// boost_mpi.cpp — Advanced
//
// Distributed 1-D heat equation solver demonstrating production-grade
// Boost.MPI patterns:
//
//   mpi::environment / communicator  — initialisation and global context
//   non-blocking halo exchange       — isend/irecv overlap communication
//     (isend/irecv)                    with inner-cell computation (latency
//                                      hiding, the canonical HPC pattern)
//   mpi::all_reduce                  — global convergence check each step
//   mpi::scatter / gather            — distribute initial conditions from
//                                      rank 0; collect final field to rank 0
//   boost::serialization             — custom Diagnostics struct sent with
//                                      a single mpi::gather call
//   mpi::timer                       — wall-clock timing on each rank
//   communicator::split()            — create a sub-communicator of even/odd
//                                      ranks to demonstrate communicator groups
//
// PDE:   u_t = α u_xx,  x ∈ [0,1],  t ∈ [0, T]
// BCs:   u(0,t) = u(1,t) = 0
// IC:    u(x,0) = sin(π x)
// Exact: u(x,t) = exp(−π² α t) sin(π x)
//
// Compile:
//   mpic++ -std=c++17 boost_mpi.cpp \
//          -lboost_mpi -lboost_serialization -o boost_mpi
// Run:
//   mpirun -n 4 ./boost_mpi

#include <boost/mpi.hpp>
#include <boost/mpi/collectives.hpp>
#include <boost/mpi/timer.hpp>
#include <boost/serialization/vector.hpp>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace mpi = boost::mpi;

// ============================================================================
//  Physical / numerical constants
// ============================================================================
static constexpr double ALPHA    = 0.25;
static constexpr int    N_GLOBAL = 200;            // interior grid points
static constexpr double DX       = 1.0 / (N_GLOBAL + 1);
static constexpr double DT       = 0.4 * DX * DX / ALPHA;  // CFL condition
static constexpr double R        = ALPHA * DT / (DX * DX); // ≤ 0.5 for stability
static constexpr int    MAX_STEPS = 400;
static constexpr double TOL      = 1e-7;           // L∞ convergence threshold

// ============================================================================
//  Diagnostics
//  Per-rank performance and accuracy report gathered on rank 0 at the end.
//  The serialize() method makes it transparently sendable via Boost.MPI.
// ============================================================================
struct Diagnostics {
    int    rank{};
    int    steps_run{};
    double final_l2_error{};
    double wall_ms{};

    template <typename Archive>
    void serialize(Archive& ar, unsigned /*version*/) {
        ar & rank & steps_run & final_l2_error & wall_ms;
    }
};

// ============================================================================
//  l2_error_local
//  Computes the squared L2 error over this rank's interior cells.
// ============================================================================
static double l2_error_local(const std::vector<double>& u,
                             int lo, double t) {
    double decay = std::exp(-M_PI * M_PI * ALPHA * t);
    double err2  = 0.0;
    // u[0] and u[local_n+1] are ghost cells; owned interior is u[1..local_n].
    for (int i = 1; i < static_cast<int>(u.size()) - 1; ++i) {
        double x     = (lo + i) * DX;   // global coordinate
        double exact = decay * std::sin(M_PI * x);
        double diff  = u[i] - exact;
        err2 += diff * diff;
    }
    return err2;
}

// ============================================================================
//  main
// ============================================================================
int main(int argc, char* argv[]) {
    mpi::environment  env(argc, argv);
    mpi::communicator world;

    const int rank   = world.rank();
    const int nranks = world.size();

    // ---- Domain decomposition -------------------------------------------
    // Each rank owns a contiguous slab of interior grid points.
    // Remainder points are distributed one-by-one to the first (extra) ranks.
    const int base  = N_GLOBAL / nranks;
    const int extra = N_GLOBAL % nranks;
    const int lo    = rank * base + std::min(rank, extra);  // first owned index (1-based)
    const int hi    = lo + base + (rank < extra ? 1 : 0);  // exclusive
    const int local_n = hi - lo;

    // Local field: ghost_left | owned_0 … owned_{local_n-1} | ghost_right
    std::vector<double> u(local_n + 2, 0.0);
    std::vector<double> u_new(local_n + 2, 0.0);

    // ---- Rank 0 builds the global IC and scatters it --------------------
    // Each rank receives exactly local_n values.
    {
        std::vector<int> counts(nranks), displs(nranks);
        for (int r = 0; r < nranks; ++r) {
            int r_lo = r * base + std::min(r, extra);
            int r_hi = r_lo + base + (r < extra ? 1 : 0);
            counts[r] = r_hi - r_lo;
            displs[r] = (r == 0) ? 0 : displs[r-1] + counts[r-1];
        }

        std::vector<double> global_ic;
        if (rank == 0) {
            global_ic.resize(N_GLOBAL);
            for (int i = 0; i < N_GLOBAL; ++i)
                global_ic[i] = std::sin(M_PI * (i + 1) * DX);
        }

        // scatterv: rank 0 distributes unequal slices.
        mpi::scatterv(world, global_ic, counts, displs,
                      u.data() + 1, local_n, 0);
    }

    // Identify neighbours (wrap-around BCs use Dirichlet zero → no send needed
    // at physical boundaries).
    const int left  = (rank > 0)          ? rank - 1 : MPI_PROC_NULL;
    const int right = (rank < nranks - 1) ? rank + 1 : MPI_PROC_NULL;

    // ---- Time integration ------------------------------------------------
    mpi::timer wall_clock;
    int  step       = 0;
    bool converged  = false;

    while (step < MAX_STEPS && !converged) {
        // -- Non-blocking halo exchange: post sends/recvs, compute interior --
        mpi::request reqs[4];
        int nreq = 0;

        if (left  != MPI_PROC_NULL) {
            reqs[nreq++] = world.isend(left,  0, u[1]);        // send left ghost
            reqs[nreq++] = world.irecv(left,  1, u[0]);        // recv left ghost
        }
        if (right != MPI_PROC_NULL) {
            reqs[nreq++] = world.isend(right, 1, u[local_n]);  // send right ghost
            reqs[nreq++] = world.irecv(right, 0, u[local_n+1]);// recv right ghost
        }

        // While communication is in flight, update the INNER cells (no ghost
        // dependency).  This overlaps computation and communication — the
        // classical latency-hiding pattern in stencil codes.
        for (int i = 2; i < local_n; ++i)
            u_new[i] = u[i] + R * (u[i+1] - 2*u[i] + u[i-1]);

        // Wait for halos, then update the two boundary cells.
        mpi::wait_all(reqs, reqs + nreq);

        u_new[1]       = u[1]       + R * (u[2]       - 2*u[1]       + u[0]);
        u_new[local_n] = u[local_n] + R * (u[local_n+1]-2*u[local_n] + u[local_n-1]);

        std::swap(u, u_new);

        // -- Convergence check every 20 steps via all_reduce -----------------
        if (step % 20 == 0) {
            double t        = (step + 1) * DT;
            double local_e2 = l2_error_local(u, lo, t);
            double global_e2 = 0.0;
            mpi::all_reduce(world, local_e2, global_e2, std::plus<double>());
            double l2 = std::sqrt(global_e2 * DX);

            if (rank == 0)
                std::cout << "step=" << std::setw(4) << step
                          << "  L2_err=" << std::scientific
                          << std::setprecision(4) << l2 << "\n";

            converged = (l2 < TOL);
        }
        ++step;
    }

    const double wall_ms = wall_clock.elapsed() * 1000.0;

    // ---- Gather diagnostics on rank 0 ------------------------------------
    double t_final  = step * DT;
    double local_e2 = l2_error_local(u, lo, t_final);
    double global_e2 = 0.0;
    mpi::all_reduce(world, local_e2, global_e2, std::plus<double>());

    Diagnostics diag{rank, step,
                     std::sqrt(global_e2 * DX), wall_ms};

    std::vector<Diagnostics> all_diag;
    mpi::gather(world, diag, all_diag, 0);

    if (rank == 0) {
        std::cout << "\n--- Per-rank diagnostics ---\n";
        for (auto& d : all_diag)
            std::cout << "  rank=" << d.rank
                      << "  steps=" << d.steps_run
                      << "  L2_final=" << d.final_l2_error
                      << "  wall=" << d.wall_ms << " ms\n";
        std::cout << (converged ? "Converged.\n" : "Max steps reached.\n");
    }

    // ---- Gather final field on rank 0 and verify -------------------------
    {
        std::vector<int> counts(nranks), displs(nranks);
        for (int r = 0; r < nranks; ++r) {
            int r_lo = r * base + std::min(r, extra);
            int r_hi = r_lo + base + (r < extra ? 1 : 0);
            counts[r] = r_hi - r_lo;
            displs[r] = (r == 0) ? 0 : displs[r-1] + counts[r-1];
        }
        // Gather only the owned interior (no ghost cells).
        std::vector<double> owned(u.begin() + 1, u.begin() + 1 + local_n);
        std::vector<double> global_field;
        mpi::gatherv(world, owned, global_field, counts, displs, 0);

        if (rank == 0) {
            double t = step * DT;
            double decay = std::exp(-M_PI * M_PI * ALPHA * t);
            double lmax = 0.0;
            for (int i = 0; i < N_GLOBAL; ++i) {
                double x = (i + 1) * DX;
                lmax = std::max(lmax, std::abs(global_field[i]
                                               - decay * std::sin(M_PI * x)));
            }
            std::cout << "Final L∞ error = " << lmax << "\n";
        }
    }

    // ---- communicator::split demo ----------------------------------------
    // Split into two sub-communicators: even and odd ranks.
    // A real application might use this to build hierarchical collective trees
    // (intra-node shared memory vs. inter-node MPI).
    int colour = rank % 2;
    mpi::communicator sub = world.split(colour);
    if (sub.rank() == 0)
        std::cout << "[split] sub-comm colour=" << colour
                  << "  size=" << sub.size()
                  << "  root rank in world=" << rank << "\n";

    return 0;
}
