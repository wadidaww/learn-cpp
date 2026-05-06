// boost_compute.cpp — Advanced
//
// GPU-accelerated image normalisation pipeline:
//   raw pixels → [normalise] → [histogram] → [equalize LUT] → output pixels
//
// Facilities combined:
//   compute::device / context / command_queue — device setup and execution
//   compute::vector<T>               — device-side container (manages CL buffer)
//   compute::copy / copy_async       — host↔device transfers
//   compute::transform               — element-wise map kernel
//   compute::fill                    — zero a device buffer in one call
//   compute::inclusive_scan          — parallel prefix sum on the device (CDF)
//   compute::gather                  — indexed lookup (apply equalisation LUT)
//   BOOST_COMPUTE_FUNCTION           — define an inline OpenCL device function
//   compute::event + CL profiling    — GPU-side kernel timing
//   command_queue(enable_profiling)  — enable CL_QUEUE_PROFILING_ENABLE
//
// Algorithm (histogram equalisation):
//   1. Normalise  :  d_norm[i]  = d_raw[i] / 255.0f
//   2. Histogram  :  count pixels in each of BINS buckets (CPU side)
//   3. CDF        :  inclusive_scan(d_hist) → d_cdf   (device)
//   4. Build LUT  :  lut[b] = round((cdf[b] - cdf_min) / (N-1) * 255)  (CPU)
//   5. Bin pixels :  d_bins[i] = (int)(d_norm[i] * 256)               (device)
//   6. Apply LUT  :  d_out[i]  = d_lut[d_bins[i]]    via gather       (device)
//
// Compile (requires OpenCL runtime and Boost.Compute headers):
//   g++ -std=c++17 boost_compute.cpp -lOpenCL -o boost_compute

#include <boost/compute.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/algorithm/fill.hpp>
#include <boost/compute/algorithm/gather.hpp>
#include <boost/compute/algorithm/inclusive_scan.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/container/vector.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

namespace compute = boost::compute;

// ============================================================================
//  generate_synthetic_image
//  Gaussian-distributed intensities clustered around 80 (dark image) so the
//  equalised output should spread evenly across [0, 255].
// ============================================================================
static std::vector<std::uint8_t>
generate_synthetic_image(int width, int height) {
    std::vector<std::uint8_t> img(width * height);
    std::mt19937 rng(42);
    std::normal_distribution<float> nd(80.0f, 30.0f);
    for (auto& px : img) {
        float v = nd(rng);
        px = static_cast<std::uint8_t>(
                 std::clamp(static_cast<int>(std::round(v)), 0, 255));
    }
    return img;
}

// ============================================================================
//  gpu_elapsed_ms  — read start/end timestamps from a CL profiling event
// ============================================================================
static double gpu_elapsed_ms(const compute::event& ev) {
    cl_ulong t0 = ev.get_profiling_info<CL_PROFILING_COMMAND_START>();
    cl_ulong t1 = ev.get_profiling_info<CL_PROFILING_COMMAND_END>();
    return static_cast<double>(t1 - t0) * 1e-6;
}

// ============================================================================
//  main
// ============================================================================
int main() {
    // ---- Device selection -----------------------------------------------
    auto device = compute::system::default_device();
    std::cout << "Device : " << device.name() << "\n"
              << "Vendor : " << device.vendor() << "\n\n";

    compute::context ctx(device);

    // Enable CL_QUEUE_PROFILING_ENABLE so every event records timestamps.
    compute::command_queue queue(
        ctx, device,
        compute::command_queue::enable_profiling);

    // ---- Prepare input --------------------------------------------------
    const int WIDTH  = 512;
    const int HEIGHT = 512;
    const int N      = WIDTH * HEIGHT;
    const int BINS   = 256;

    auto host_raw = generate_synthetic_image(WIDTH, HEIGHT);

    // ---- Host → device -------------------------------------------------
    compute::vector<compute::uchar_> d_raw(N, ctx);
    compute::copy(host_raw.begin(), host_raw.end(), d_raw.begin(), queue);
    queue.finish();

    // ---- Step 1: Normalise  uint8 → float32 [0, 1] --------------------
    //  BOOST_COMPUTE_FUNCTION compiles a named device function from C-style
    //  syntax.  It is JIT-compiled once and cached for reuse.
    BOOST_COMPUTE_FUNCTION(float, to_float_norm, (compute::uchar_ x),
    {
        return (float)x / 255.0f;
    });

    compute::vector<float> d_norm(N, ctx);
    compute::event ev_norm = compute::transform(
        d_raw.begin(), d_raw.end(),
        d_norm.begin(),
        to_float_norm, queue);
    queue.finish();
    std::cout << std::fixed << std::setprecision(3)
              << "Step 1 normalise  : " << gpu_elapsed_ms(ev_norm) << " ms\n";

    // ---- Step 2: Histogram (CPU) ----------------------------------------
    // A production implementation would use an atomic-add OpenCL kernel;
    // we keep it on the CPU to avoid boilerplate and stay focused on Compute.
    std::vector<float> host_norm(N);
    compute::copy(d_norm.begin(), d_norm.end(), host_norm.begin(), queue);
    queue.finish();

    std::vector<float> hist(BINS, 0.0f);
    for (float v : host_norm) {
        int bin = std::clamp(static_cast<int>(v * BINS), 0, BINS - 1);
        hist[bin] += 1.0f;
    }

    // ---- Step 3: CDF — inclusive prefix sum on device ------------------
    compute::vector<float> d_hist(hist.begin(), hist.end(), queue);
    compute::vector<float> d_cdf(BINS, ctx);

    auto t0 = std::chrono::steady_clock::now();
    compute::inclusive_scan(
        d_hist.begin(), d_hist.end(), d_cdf.begin(), queue);
    queue.finish();
    auto t1 = std::chrono::steady_clock::now();
    std::cout << "Step 3 prefix sum : "
              << std::chrono::duration<double, std::milli>(t1 - t0).count()
              << " ms\n";

    // ---- Step 4: Build equalisation LUT (CPU) ---------------------------
    std::vector<float> host_cdf(BINS);
    compute::copy(d_cdf.begin(), d_cdf.end(), host_cdf.begin(), queue);
    queue.finish();

    float cdf_min = *std::min_element(host_cdf.begin(), host_cdf.end());
    std::vector<compute::uchar_> lut(BINS);
    for (int b = 0; b < BINS; ++b) {
        float eq = (host_cdf[b] - cdf_min) / static_cast<float>(N - 1) * 255.0f;
        lut[b] = static_cast<compute::uchar_>(
                     std::clamp(static_cast<int>(std::round(eq)), 0, 255));
    }

    // ---- Step 5: Compute bin index per pixel on device -----------------
    BOOST_COMPUTE_FUNCTION(int, to_bin, (float v),
    {
        int b = (int)(v * 256.0f);
        return (b < 256) ? b : 255;
    });

    compute::vector<int> d_bins(N, ctx);
    compute::event ev_bins = compute::transform(
        d_norm.begin(), d_norm.end(),
        d_bins.begin(), to_bin, queue);
    queue.finish();
    std::cout << "Step 5 bin index  : " << gpu_elapsed_ms(ev_bins) << " ms\n";

    // ---- Step 6: Apply LUT via compute::gather -------------------------
    //  gather(map_first, map_last, input_first, output_first, queue)
    //  output[i] = input[map[i]]
    //  i.e. output[i] = lut[d_bins[i]]  — the equalised pixel value.
    compute::vector<compute::uchar_> d_lut(lut.begin(), lut.end(), queue);
    compute::vector<compute::uchar_> d_out(N, ctx);

    auto t2 = std::chrono::steady_clock::now();
    compute::gather(d_bins.begin(), d_bins.end(),
                    d_lut.begin(), d_out.begin(), queue);
    queue.finish();
    auto t3 = std::chrono::steady_clock::now();
    std::cout << "Step 6 gather LUT : "
              << std::chrono::duration<double, std::milli>(t3 - t2).count()
              << " ms\n";

    // ---- Verify: output histogram should be nearly flat ----------------
    std::vector<compute::uchar_> host_out(N);
    compute::copy(d_out.begin(), d_out.end(), host_out.begin(), queue);

    std::vector<int> out_hist(BINS, 0);
    for (auto px : host_out) ++out_hist[static_cast<int>(px)];

    int min_cnt = *std::min_element(out_hist.begin(), out_hist.end());
    int max_cnt = *std::max_element(out_hist.begin(), out_hist.end());
    double mean_in  = std::accumulate(host_raw.begin(), host_raw.end(), 0LL)
                      / static_cast<double>(N);
    double mean_out = std::accumulate(host_out.begin(), host_out.end(), 0LL)
                      / static_cast<double>(N);

    std::cout << "\n--- Histogram equalisation result ---\n"
              << "  Input  mean intensity : " << mean_in  << "\n"
              << "  Output mean intensity : " << mean_out << "\n"
              << "  Output hist min bucket: " << min_cnt  << "\n"
              << "  Output hist max bucket: " << max_cnt  << "\n"
              << "  Ideal  flat count     : " << N / BINS << "\n";

    // ---- Device memory summary -----------------------------------------
    std::cout << "\n--- Device buffers (KiB) ---\n"
              << "  d_raw   : " << N    * 1 / 1024 << "\n"
              << "  d_norm  : " << N    * 4 / 1024 << "\n"
              << "  d_hist  : " << BINS * 4        << "  B\n"
              << "  d_cdf   : " << BINS * 4        << "  B\n"
              << "  d_bins  : " << N    * 4 / 1024 << "\n"
              << "  d_out   : " << N    * 1 / 1024 << "\n";

    return 0;
}
