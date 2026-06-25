#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

class Socket {
public:
    explicit Socket(int fd = -1) : fd_(fd) {}
    ~Socket() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            if (fd_ != -1) {
                ::close(fd_);
            }
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const { return fd_; }
    int release() {
        const int fd = fd_;
        fd_ = -1;
        return fd;
    }

private:
    int fd_;
};

Socket tcp_socket() {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        throw std::runtime_error("socket() failed: " + std::string(std::strerror(errno)));
    }
    return Socket(fd);
}

void set_reuse_addr(int fd) {
    int one = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
    }
}

void set_tcp_no_delay(int fd) {
    int one = 1;
    if (::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) == -1) {
        throw std::runtime_error("setsockopt(TCP_NODELAY) failed");
    }
}

void set_nonblocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("fcntl(O_NONBLOCK) failed");
    }
}

sockaddr_in loopback(std::uint16_t port) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return address;
}

std::uint16_t bound_port(int fd) {
    sockaddr_in address{};
    socklen_t length = sizeof(address);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &length) == -1) {
        throw std::runtime_error("getsockname() failed");
    }
    return ntohs(address.sin_port);
}

Socket listen_on_loopback() {
    Socket listener = tcp_socket();
    set_reuse_addr(listener.get());

    const auto address = loopback(0);
    if (::bind(listener.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1) {
        throw std::runtime_error("bind() failed: " + std::string(std::strerror(errno)));
    }
    if (::listen(listener.get(), SOMAXCONN) == -1) {
        throw std::runtime_error("listen() failed");
    }
    return listener;
}

void send_all(int fd, std::string_view message) {
    while (!message.empty()) {
        const ssize_t sent = ::send(fd, message.data(), message.size(), 0);
        if (sent == -1) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("send() failed: " + std::string(std::strerror(errno)));
        }
        message.remove_prefix(static_cast<std::size_t>(sent));
    }
}

std::string recv_some(int fd) {
    std::array<char, 64> buffer{};
    const ssize_t received = ::recv(fd, buffer.data(), buffer.size(), 0);
    if (received == -1) {
        throw std::runtime_error("recv() failed: " + std::string(std::strerror(errno)));
    }
    return {buffer.data(), static_cast<std::size_t>(received)};
}

std::string blocking_client_round_trip(std::uint16_t port, std::string_view message) {
    Socket client = tcp_socket();
    set_tcp_no_delay(client.get());

    const auto address = loopback(port);
    if (::connect(client.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1) {
        throw std::runtime_error("connect() failed: " + std::string(std::strerror(errno)));
    }

    send_all(client.get(), message);
    return recv_some(client.get());
}

void blocking_echo_server_once(Socket listener) {
    Socket client(::accept(listener.get(), nullptr, nullptr));
    if (client.get() == -1) {
        throw std::runtime_error("accept() failed");
    }
    set_tcp_no_delay(client.get());

    const auto request = recv_some(client.get());
    send_all(client.get(), request);
}

void run_blocking_echo_example() {
    Socket listener = listen_on_loopback();
    const auto port = bound_port(listener.get());

    std::thread server([listener = std::move(listener)]() mutable { blocking_echo_server_once(std::move(listener)); });
    const auto reply = blocking_client_round_trip(port, "blocking tcp echo");
    server.join();

    std::cout << "blocking sockets: '" << reply << "'\n";
}

std::string nonblocking_client_round_trip(std::uint16_t port, std::string_view message) {
    Socket client = tcp_socket();
    set_tcp_no_delay(client.get());
    set_nonblocking(client.get());

    const auto address = loopback(port);
    if (::connect(client.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1 &&
        errno != EINPROGRESS) {
        throw std::runtime_error("nonblocking connect() failed");
    }

    Socket epoll_fd(::epoll_create1(EPOLL_CLOEXEC));
    if (epoll_fd.get() == -1) {
        throw std::runtime_error("epoll_create1() failed");
    }

    epoll_event event{};
    event.events = EPOLLOUT;
    event.data.fd = client.get();
    if (::epoll_ctl(epoll_fd.get(), EPOLL_CTL_ADD, client.get(), &event) == -1) {
        throw std::runtime_error("epoll_ctl(ADD) failed");
    }

    epoll_event ready{};
    const int connect_ready = ::epoll_wait(epoll_fd.get(), &ready, 1, 1000);
    if (connect_ready == -1) {
        throw std::runtime_error("epoll_wait() failed waiting for connect");
    }
    if (connect_ready == 0) {
        throw std::runtime_error("epoll_wait() timed out waiting for connect");
    }

    int error{};
    socklen_t error_length = sizeof(error);
    if (::getsockopt(client.get(), SOL_SOCKET, SO_ERROR, &error, &error_length) == -1 || error != 0) {
        throw std::runtime_error("nonblocking connect did not complete");
    }

    send_all(client.get(), message);

    event.events = EPOLLIN;
    if (::epoll_ctl(epoll_fd.get(), EPOLL_CTL_MOD, client.get(), &event) == -1) {
        throw std::runtime_error("epoll_ctl(MOD) failed");
    }
    const int reply_ready = ::epoll_wait(epoll_fd.get(), &ready, 1, 1000);
    if (reply_ready == -1) {
        throw std::runtime_error("epoll_wait() failed waiting for reply");
    }
    if (reply_ready == 0) {
        throw std::runtime_error("epoll_wait() timed out waiting for reply");
    }

    return recv_some(client.get());
}

void run_nonblocking_client_example() {
    Socket listener = listen_on_loopback();
    const auto port = bound_port(listener.get());

    std::thread server([listener = std::move(listener)]() mutable { blocking_echo_server_once(std::move(listener)); });
    const auto reply = nonblocking_client_round_trip(port, "nonblocking tcp echo");
    server.join();

    std::cout << "nonblocking client: '" << reply << "'\n";
}

void epoll_echo_server(Socket listener, int expected_clients) {
    set_nonblocking(listener.get());

    Socket epoll_fd(::epoll_create1(EPOLL_CLOEXEC));
    if (epoll_fd.get() == -1) {
        throw std::runtime_error("epoll_create1() failed");
    }

    epoll_event listen_event{};
    listen_event.events = EPOLLIN;
    listen_event.data.fd = listener.get();
    if (::epoll_ctl(epoll_fd.get(), EPOLL_CTL_ADD, listener.get(), &listen_event) == -1) {
        throw std::runtime_error("epoll_ctl(listener) failed");
    }

    std::vector<Socket> clients;
    int replies_sent = 0;

    while (replies_sent < expected_clients) {
        std::array<epoll_event, 8> events{};
        const int ready_count = ::epoll_wait(epoll_fd.get(), events.data(), events.size(), 1000);
        if (ready_count <= 0) {
            throw std::runtime_error("epoll_wait() failed or timed out");
        }

        for (int i = 0; i < ready_count; ++i) {
            const int fd = events[i].data.fd;
            if (fd == listener.get()) {
                for (;;) {
                    Socket client(::accept(listener.get(), nullptr, nullptr));
                    if (client.get() == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        throw std::runtime_error("accept() failed in epoll server");
                    }
                    set_tcp_no_delay(client.get());
                    set_nonblocking(client.get());

                    epoll_event client_event{};
                    client_event.events = EPOLLIN;
                    client_event.data.fd = client.get();
                    if (::epoll_ctl(epoll_fd.get(), EPOLL_CTL_ADD, client.get(), &client_event) == -1) {
                        throw std::runtime_error("epoll_ctl(client) failed");
                    }
                    clients.push_back(std::move(client));
                }
                continue;
            }

            const auto request = recv_some(fd);
            if (!request.empty()) {
                send_all(fd, request);
                ++replies_sent;
            }
        }
    }
}

void run_epoll_server_example() {
    Socket listener = listen_on_loopback();
    const auto port = bound_port(listener.get());

    std::thread server([listener = std::move(listener)]() mutable { epoll_echo_server(std::move(listener), 2); });
    const auto first = blocking_client_round_trip(port, "epoll client one");
    const auto second = blocking_client_round_trip(port, "epoll client two");
    server.join();

    std::cout << "epoll server: '" << first << "', '" << second << "'\n";
}

} // namespace

int main() {
    run_blocking_echo_example();
    run_nonblocking_client_example();
    run_epoll_server_example();
}
