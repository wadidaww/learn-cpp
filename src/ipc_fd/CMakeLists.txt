cmake_minimum_required(VERSION 3.10)

# ── FD Passing (SCM_RIGHTS) ────────────────────────────────────────────────
add_executable(ipc_fd_sender   fd_passing/sender.cpp)
target_compile_options(ipc_fd_sender   PRIVATE -Wall -Wextra -O2)

add_executable(ipc_fd_receiver fd_passing/receiver.cpp)
target_compile_options(ipc_fd_receiver PRIVATE -Wall -Wextra -O2)

# ── select()-based IPC ─────────────────────────────────────────────────────
add_executable(ipc_select select_ipc/select_example.cpp)
target_compile_options(ipc_select PRIVATE -Wall -Wextra -O2)

# ── epoll()-based IPC ──────────────────────────────────────────────────────
add_executable(ipc_epoll epoll_ipc/epoll_example.cpp)
target_compile_options(ipc_epoll PRIVATE -Wall -Wextra -O2)
