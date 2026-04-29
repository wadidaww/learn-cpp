// boost_mpi.cpp
// Demonstrates Boost.MPI for distributed-memory HPC programming.
//
// Compile:
//   mpic++ -std=c++17 boost_mpi.cpp -lboost_mpi -lboost_serialization -o boost_mpi
//
// Run with N processes:
//   mpirun -n 4 ./boost_mpi
//
// Concepts shown:
//   - mpi::environment / mpi::communicator
//   - Point-to-point: send / recv
//   - Collective: broadcast, reduce (sum), gather
//   - Non-blocking: isend / irecv with wait

#include <boost/mpi.hpp>
#include <boost/mpi/collectives.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <iostream>
#include <numeric>
#include <vector>

namespace mpi = boost::mpi;

// ---------------------------------------------------------------------------
// Example 1: Point-to-point — rank 0 sends a greeting to every other rank
// ---------------------------------------------------------------------------
void point_to_point(mpi::communicator& world) {
    if (world.rank() == 0) {
        for (int dest = 1; dest < world.size(); ++dest) {
            std::string msg = "Hello from rank 0 to rank " + std::to_string(dest);
            world.send(dest, /*tag=*/0, msg);
        }
    } else {
        std::string msg;
        world.recv(0, /*tag=*/0, msg);
        std::cout << "[p2p] rank " << world.rank() << " received: " << msg << "\n";
    }
    world.barrier();
}

// ---------------------------------------------------------------------------
// Example 2: Broadcast — rank 0 shares a value with all ranks
// ---------------------------------------------------------------------------
void broadcast_example(mpi::communicator& world) {
    int value = (world.rank() == 0) ? 42 : 0;
    mpi::broadcast(world, value, /*root=*/0);
    std::cout << "[broadcast] rank " << world.rank()
              << " has value=" << value << "\n";
    world.barrier();
}

// ---------------------------------------------------------------------------
// Example 3: Reduce — each rank contributes its rank number; root collects the sum
// ---------------------------------------------------------------------------
void reduce_example(mpi::communicator& world) {
    int local_val = world.rank();
    int global_sum = 0;
    mpi::reduce(world, local_val, global_sum, std::plus<int>(), /*root=*/0);
    if (world.rank() == 0) {
        int expected = world.size() * (world.size() - 1) / 2;
        std::cout << "[reduce] sum of ranks = " << global_sum
                  << "  (expected " << expected << ")\n";
    }
    world.barrier();
}

// ---------------------------------------------------------------------------
// Example 4: Gather — each rank contributes a vector; root collects all
// ---------------------------------------------------------------------------
void gather_example(mpi::communicator& world) {
    // Each rank produces a small vector of ints unique to that rank.
    std::vector<int> local = {world.rank() * 10, world.rank() * 10 + 1};

    std::vector<std::vector<int>> all_data;
    mpi::gather(world, local, all_data, /*root=*/0);

    if (world.rank() == 0) {
        std::cout << "[gather] root received:\n";
        for (int r = 0; r < world.size(); ++r) {
            std::cout << "  rank " << r << ":";
            for (int v : all_data[r]) std::cout << " " << v;
            std::cout << "\n";
        }
    }
    world.barrier();
}

// ---------------------------------------------------------------------------
// Example 5: Non-blocking send/recv (isend / irecv)
//
// Each rank sends to its right neighbour and receives from its left neighbour
// in a ring topology — classic HPC halo exchange pattern.
// ---------------------------------------------------------------------------
void nonblocking_ring(mpi::communicator& world) {
    int rank = world.rank();
    int size = world.size();
    int send_val = rank;
    int recv_val = -1;

    int right = (rank + 1) % size;
    int left  = (rank - 1 + size) % size;

    mpi::request send_req = world.isend(right, /*tag=*/1, send_val);
    mpi::request recv_req = world.irecv(left,  /*tag=*/1, recv_val);

    mpi::wait_all(&send_req, &send_req + 1); // could overlap with computation here
    mpi::wait_all(&recv_req, &recv_req + 1);

    std::cout << "[ring] rank " << rank << " sent " << send_val
              << " → right,  received " << recv_val << " ← left\n";
    world.barrier();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    mpi::environment  env(argc, argv);
    mpi::communicator world;

    if (world.rank() == 0) std::cout << "=== Point-to-point ===\n";
    point_to_point(world);

    if (world.rank() == 0) std::cout << "\n=== Broadcast ===\n";
    broadcast_example(world);

    if (world.rank() == 0) std::cout << "\n=== Reduce ===\n";
    reduce_example(world);

    if (world.rank() == 0) std::cout << "\n=== Gather ===\n";
    gather_example(world);

    if (world.rank() == 0) std::cout << "\n=== Non-blocking ring exchange ===\n";
    nonblocking_ring(world);

    return 0;
}
