// boost_compute.cpp
// Demonstrates Boost.Compute for GPU / OpenCL accelerated computing.
//
// Boost.Compute wraps OpenCL and provides STL-like algorithms that run
// on GPUs or other OpenCL devices. It is header-only on top of OpenCL.
//
// Compile (requires OpenCL runtime and headers):
//   g++ -std=c++17 boost_compute.cpp -lOpenCL -o boost_compute
//
// Concepts shown:
//   - Device enumeration
//   - Copying data to / from the device
//   - compute::transform  (element-wise operation on device)
//   - compute::sort       (parallel sort on device)
//   - compute::reduce     (parallel reduction / sum)
//   - Custom kernel via BOOST_COMPUTE_FUNCTION macro

#include <boost/compute.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/algorithm/sort.hpp>
#include <boost/compute/algorithm/reduce.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/functional/math.hpp>
#include <iostream>
#include <numeric>
#include <vector>
#include <random>

namespace compute = boost::compute;

// ---------------------------------------------------------------------------
// Helper: print the first 'n' elements of a device vector
// ---------------------------------------------------------------------------
template<typename T>
void print_device_vec(const compute::vector<T>& dv, int n, const char* label) {
    std::vector<T> host(n);
    compute::copy(dv.begin(), dv.begin() + n, host.begin(), dv.get_buffer().get_context().get_command_queue());
    std::cout << label << ":";
    for (auto v : host) std::cout << " " << v;
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
// Example 1: Device info — enumerate available OpenCL devices
// ---------------------------------------------------------------------------
void device_info() {
    std::cout << "[devices] available OpenCL devices:\n";
    for (const auto& device : compute::system::devices()) {
        std::cout << "  - " << device.name()
                  << "  (type: " << device.type_name() << ")\n";
    }
}

// ---------------------------------------------------------------------------
// Example 2: transform — square each element on the GPU
// ---------------------------------------------------------------------------
void transform_example(compute::command_queue& queue) {
    const int N = 16;
    std::vector<float> host_in(N);
    std::iota(host_in.begin(), host_in.end(), 1.0f); // 1,2,...,16

    compute::vector<float> d_in(N, queue.get_context());
    compute::vector<float> d_out(N, queue.get_context());

    // Copy host → device
    compute::copy(host_in.begin(), host_in.end(), d_in.begin(), queue);

    // Square each element: d_out[i] = d_in[i] * d_in[i]
    compute::transform(d_in.begin(), d_in.end(), d_out.begin(),
                       compute::multiplies<float>() /* not a self-multiply —
                       use a lambda-style function below instead */,
                       queue);

    // For a true x*x we define a custom unary function:
    BOOST_COMPUTE_FUNCTION(float, square, (float x), { return x * x; });

    compute::transform(d_in.begin(), d_in.end(), d_out.begin(), square, queue);
    queue.finish();

    std::vector<float> host_out(N);
    compute::copy(d_out.begin(), d_out.end(), host_out.begin(), queue);

    std::cout << "[transform] first 8 squares:";
    for (int i = 0; i < 8; ++i) std::cout << " " << host_out[i];
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
// Example 3: sort — parallel sort on the device
// ---------------------------------------------------------------------------
void sort_example(compute::command_queue& queue) {
    const int N = 20;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 100);

    std::vector<int> host(N);
    std::generate(host.begin(), host.end(), [&] { return dist(rng); });

    compute::vector<int> d_vec(N, queue.get_context());
    compute::copy(host.begin(), host.end(), d_vec.begin(), queue);

    std::cout << "[sort] before:";
    for (int v : host) std::cout << " " << v;
    std::cout << "\n";

    compute::sort(d_vec.begin(), d_vec.end(), queue);
    queue.finish();

    compute::copy(d_vec.begin(), d_vec.end(), host.begin(), queue);
    std::cout << "[sort] after: ";
    for (int v : host) std::cout << " " << v;
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
// Example 4: reduce — parallel sum
// ---------------------------------------------------------------------------
void reduce_example(compute::command_queue& queue) {
    const int N = 1024;
    std::vector<float> host(N, 1.0f); // all ones → sum == N

    compute::vector<float> d_vec(N, queue.get_context());
    compute::copy(host.begin(), host.end(), d_vec.begin(), queue);

    float result = 0.0f;
    compute::reduce(d_vec.begin(), d_vec.end(), &result, queue);
    queue.finish();

    std::cout << "[reduce] sum of " << N << " ones = " << result
              << "  (expected " << N << ")\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    device_info();

    // Pick the default (usually most capable) device.
    compute::device device = compute::system::default_device();
    std::cout << "\nUsing device: " << device.name() << "\n\n";

    compute::context       ctx(device);
    compute::command_queue queue(ctx, device);

    std::cout << "=== transform (GPU square) ===\n";
    transform_example(queue);

    std::cout << "\n=== sort (GPU parallel sort) ===\n";
    sort_example(queue);

    std::cout << "\n=== reduce (GPU parallel sum) ===\n";
    reduce_example(queue);

    return 0;
}
