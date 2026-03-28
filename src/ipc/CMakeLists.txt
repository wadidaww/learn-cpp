cmake_minimum_required(VERSION 3.10)

# ── Anonymous Pipes ────────────────────────────────────────────────────────
add_executable(ipc_pipe pipe/pipe_example.cpp)
target_compile_options(ipc_pipe PRIVATE -Wall -Wextra -O2)

# ── Named Pipes (FIFO) ─────────────────────────────────────────────────────
add_executable(ipc_fifo_writer named_pipe/writer.cpp)
target_compile_options(ipc_fifo_writer PRIVATE -Wall -Wextra -O2)

add_executable(ipc_fifo_reader named_pipe/reader.cpp)
target_compile_options(ipc_fifo_reader PRIVATE -Wall -Wextra -O2)

# ── Shared Memory + POSIX Semaphores ──────────────────────────────────────
add_executable(ipc_shm_writer shared_memory/writer.cpp)
target_compile_options(ipc_shm_writer PRIVATE -Wall -Wextra -O2)
target_link_libraries(ipc_shm_writer PRIVATE rt pthread)

add_executable(ipc_shm_reader shared_memory/reader.cpp)
target_compile_options(ipc_shm_reader PRIVATE -Wall -Wextra -O2)
target_link_libraries(ipc_shm_reader PRIVATE rt pthread)

# ── POSIX Message Queues ───────────────────────────────────────────────────
add_executable(ipc_mq_sender message_queue/sender.cpp)
target_compile_options(ipc_mq_sender PRIVATE -Wall -Wextra -O2)
target_link_libraries(ipc_mq_sender PRIVATE rt)

add_executable(ipc_mq_receiver message_queue/receiver.cpp)
target_compile_options(ipc_mq_receiver PRIVATE -Wall -Wextra -O2)
target_link_libraries(ipc_mq_receiver PRIVATE rt)

# ── Unix Domain Sockets ────────────────────────────────────────────────────
add_executable(ipc_uds_server unix_socket/server.cpp)
target_compile_options(ipc_uds_server PRIVATE -Wall -Wextra -O2)

add_executable(ipc_uds_client unix_socket/client.cpp)
target_compile_options(ipc_uds_client PRIVATE -Wall -Wextra -O2)
