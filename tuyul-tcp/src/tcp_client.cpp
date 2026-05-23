#include <tuyul/tcp_client.hpp>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netdb.h>
#include <thread>
#include <vector>
#include <cstring>

namespace tuyul {

/**
 * @brief Helper utility to transform file descriptors into non-blocking operational states.
 */
static bool set_fd_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

/**
 * @brief Private Implementation infrastructure mapping layout (Pimpl Pattern) for TcpClient.
 */
struct TcpClient::Impl {
    TcpOptions options;
    int client_fd{-1};
    int epoll_fd{-1};
    bool is_connected{false};
    
    std::thread worker_thread;
    LoggerCallback logger_cb{nullptr};
    DataCallback data_cb{nullptr};

    explicit Impl(TcpOptions opts) : options(opts) {}

    void log(std::string_view level, std::string_view message) {
        if (logger_cb) logger_cb(level, message);
    }

    void shutdown_pipeline() {
        is_connected = false;
        
        // Safely re-join the background epoll worker thread back into parent runtime context
        if (worker_thread.joinable()) {
            worker_thread.join();
        }

        if (epoll_fd >= 0) {
            close(epoll_fd);
            epoll_fd = -1;
        }

        if (client_fd >= 0) {
            close(client_fd);
            log("INFO", "Client communication network socket gracefully dismantled.");
            client_fd = -1;
        }
    }
};

TcpClient::TcpClient(TcpOptions options) : m_impl(std::make_unique<Impl>(options)) {}

TcpClient::~TcpClient() {
    disconnect();
}

TcpClient::TcpClient(TcpClient&&) noexcept = default;
TcpClient& TcpClient::operator=(TcpClient&&) noexcept = default;

void TcpClient::set_logger(LoggerCallback logger) {
    m_impl->logger_cb = std::move(logger);
}

void TcpClient::on_data(DataCallback callback) {
    m_impl->data_cb = std::move(callback);
}

void TcpClient::disconnect() {
    if (m_impl && m_impl->is_connected) {
        m_impl->shutdown_pipeline();
    }
}

ErrorCode TcpClient::connect_to(const std::string& host, int port) {
    if (m_impl->is_connected) {
        m_impl->log("WARN", "Client socket connection request ignored: instance already tied to a host link.");
        return ErrorCode::SUCCESS;
    }

    // Resolve host addresses using network domain layout registries
    struct addrinfo hints{}, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Force IPv4 standard structures
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) {
        m_impl->log("ERROR", "Host network domain translation address lookup failed.");
        return ErrorCode::SOCKET_CREATION_FAILED;
    }

    // Spawn master client stream channel descriptor socket
    m_impl->client_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_impl->client_fd < 0) {
        m_impl->log("ERROR", "Operating system failed to allocate raw client stream descriptor.");
        freeaddrinfo(res);
        return ErrorCode::SOCKET_CREATION_FAILED;
    }

    // Initiate raw transport synchronous link attachment connection request
    if (connect(m_impl->client_fd, res->ai_addr, res->ai_addrlen) < 0) {
        m_impl->log("ERROR", "Network handshake connection request rejected by remote host interface endpoint.");
        close(m_impl->client_fd);
        m_impl->client_fd = -1;
        freeaddrinfo(res);
        return ErrorCode::BIND_FAILED;
    }

    freeaddrinfo(res); // Flush resolved allocations safely

    // Force non-blocking paradigm post connection to allow clean epoll asynchronous lookups
    if (!set_fd_non_blocking(m_impl->client_fd)) {
        m_impl->log("ERROR", "Failed to transform connected client socket into a non-blocking execution block.");
        m_impl->shutdown_pipeline();
        return ErrorCode::SET_OPTION_FAILED;
    }

    // Initialize the Linux kernel epoll instance manager
    m_impl->epoll_fd = epoll_create1(0);
    if (m_impl->epoll_fd < 0) {
        m_impl->log("ERROR", "Linux kernel failed to allocate native client epoll core multiplexer instance.");
        m_impl->shutdown_pipeline();
        return ErrorCode::EPOLL_CREATION_FAILED;
    }

    // Bind connection socket onto epoll monitoring arrays
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET; // Leverage Edge-Triggered event architecture rules
    ev.data.fd = m_impl->client_fd;
    if (epoll_ctl(m_impl->epoll_fd, EPOLL_CTL_ADD, m_impl->client_fd, &ev) < 0) {
        m_impl->log("ERROR", "Failed to register active channel descriptor into client epoll reactor array.");
        m_impl->shutdown_pipeline();
        return ErrorCode::EPOLL_CREATION_FAILED;
    }

    m_impl->is_connected = true;
    m_impl->log("INFO", "Successfully hooked asynchronous transport link channel connection to remote server.");

    // Spawn background processing context thread to manage async inbound traffic ingestion loops
    m_impl->worker_thread = std::thread([this]() {
        std::vector<epoll_event> events(m_impl->options.max_epoll_events);
        std::vector<char> buffer(m_impl->options.network_buffer_size);

        while (m_impl->is_connected) {
            #ifdef UNIT_TEST
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (!m_impl->is_connected) break;
            continue;
            #else
            // Standard asynchronous wait blocks loop cycles
            int active_events = epoll_wait(m_impl->epoll_fd, events.data(), m_impl->options.max_epoll_events, m_impl->options.watchdog_timeout_ms);
            if (active_events < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (active_events == 0) continue; // Check for connection state lifecycle changes upon timeout triggers

            for (int i = 0; i < active_events; ++i) {
                if (events[i].events & EPOLLIN) {
                    bool server_dropped = false;
                    while (true) {
                        ssize_t bytes_fetched = read(m_impl->client_fd, buffer.data(), buffer.size());
                        if (bytes_fetched > 0) {
                            if (m_impl->data_cb) {
                                m_impl->data_cb(std::string_view(buffer.data(), bytes_fetched));
                            }
                        } else if (bytes_fetched == 0) {
                            m_impl->log("INFO", "Remote server terminated communication pipeline channel gracefully.");
                            server_dropped = true;
                            break;
                        } else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) break; // Buffer pool completely exhausted
                            m_impl->log("WARN", "Encountered processing fault while ingesting incoming server stream packet arrays.");
                            server_dropped = true;
                            break;
                        }
                    }
                    if (server_dropped) {
                        m_impl->is_connected = false;
                        break;
                    }
                }
            }
            #endif
        }
    });

    return ErrorCode::SUCCESS;
}

ErrorCode TcpClient::send_data(std::string_view payload) {
    if (!m_impl->is_connected || m_impl->client_fd < 0) {
        m_impl->log("WARN", "Data transmission request rejected: socket descriptor channel is closed.");
        return ErrorCode::READ_FAILED; // Reusing error structures for missing links
    }

    size_t bytes_left = payload.length();
    const char* data_ptr = payload.data();

    // Loop until the entire data chunk payload successfully exits onto the kernel hardware queue
    while (bytes_left > 0) {
        ssize_t bytes_sent = write(m_impl->client_fd, data_ptr, bytes_left);
        if (bytes_sent > 0) {
            bytes_left -= bytes_sent;
            data_ptr += bytes_sent;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // OS hardware stream pipelines are temporarily busy, yield briefly and retry
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            m_impl->log("ERROR", "Fatal transaction fault encountered while dispatching packet bytes down the link channel.");
            return ErrorCode::READ_FAILED;
        }
    }

    return ErrorCode::SUCCESS;
}

} // namespace tuyul
