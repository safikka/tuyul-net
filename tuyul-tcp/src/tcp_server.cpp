#include <tuyul/tcp_server.hpp>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <thread>
#include <chrono>

namespace tuyul {

// Maximum events epoll can handle per single iteration loop
constexpr int MAX_EPOLL_EVENTS = 64;
// Shared internal buffer allocation chunk footprint
constexpr int NETWORK_BUFFER_SIZE = 4096;

/**
 * @brief Helper utility to enforce non-blocking operational rules on OS descriptors.
 */
static bool make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

/**
 * @brief Private Implementation structural layout (Pimpl Pattern).
 * Encapsulates raw Linux OS socket primitives away from public API boundaries.
 */
struct TcpServer::Impl {
    int port;
    TcpOptions options;
    int server_fd{-1};
    int epoll_fd{-1};
    bool is_running{false};
    
    LoggerCallback logger_cb{nullptr};
    DataCallback data_cb{nullptr};

    Impl(int p, TcpOptions opts) : port(p), options(opts) {}

    void log(std::string_view level, std::string_view message) {
        if (logger_cb) {
            logger_cb(level, message);
        }
    }

    void cleanup() {
        if (epoll_fd >= 0) {
            close(epoll_fd);
            epoll_fd = -1;
        }
        if (server_fd >= 0) {
            close(server_fd);
            log("INFO", "Network server socket successfully closed.");
            server_fd = -1;
        }
        is_running = false;
    }
};

// Constructor: Initializes the structural Pimpl bridge layout allocation
TcpServer::TcpServer(int port, TcpOptions options) 
    : m_impl(std::make_unique<Impl>(port, options)) {}

// Destructor: Triggers fallback resource sweeping via RAII rules
TcpServer::~TcpServer() {
    stop();
}

// Move Semantics Enforcements
TcpServer::TcpServer(TcpServer&&) noexcept = default;
TcpServer& TcpServer::operator=(TcpServer&&) noexcept = default;

void TcpServer::set_logger(LoggerCallback logger) {
    m_impl->logger_cb = std::move(logger);
}

void TcpServer::on_data(DataCallback callback) {
    m_impl->data_cb = std::move(callback);
}

void TcpServer::stop() {
    if (m_impl && m_impl->is_running) {
        m_impl->cleanup();
    }
}

ErrorCode TcpServer::start() {
    // 1. Spawning master transport socket descriptor (IPv4, TCP stream)
    m_impl->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_impl->server_fd < 0) {
        m_impl->log("ERROR", "Failed to allocate master system socket descriptor.");
        return ErrorCode::SOCKET_CREATION_FAILED;
    }

    // 2. Adjust option settings to allow rapid port reclamation on reboots
    int opt = 1;
    if (setsockopt(m_impl->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        m_impl->log("ERROR", "Failed to configure SO_REUSEADDR socket option.");
        m_impl->cleanup();
        return ErrorCode::SET_OPTION_FAILED;
    }

    // Enforce non-blocking paradigm on the listening master descriptor
    if (!make_socket_non_blocking(m_impl->server_fd)) {
        m_impl->log("ERROR", "Failed to transition master socket into non-blocking context.");
        m_impl->cleanup();
        return ErrorCode::SET_OPTION_FAILED;
    }

    // 3. Bind socket interface configuration structure
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all network local interfaces
    address.sin_port = htons(m_impl->port);

    if (bind(m_impl->server_fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
        m_impl->log("ERROR", "Port binding configuration failed. Interface might be busy.");
        m_impl->cleanup();
        return ErrorCode::BIND_FAILED;
    }

    // 4. Transform socket state into listening backlog manager
    if (listen(m_impl->server_fd, SOMAXCONN) < 0) {
        m_impl->log("ERROR", "Failed to transition server socket into operational listening phase.");
        m_impl->cleanup();
        return ErrorCode::LISTEN_FAILED;
    }

    // 5. Initialize the Linux kernel epoll multiplexer engine
    m_impl->epoll_fd = epoll_create1(0);
    if (m_impl->epoll_fd < 0) {
        m_impl->log("ERROR", "Failed to spawn native Linux epoll orchestration instance.");
        m_impl->cleanup();
        return ErrorCode::EPOLL_CREATION_FAILED;
    }

    // Add server socket into epoll monitoring registry array
    epoll_event ev{};
    ev.events = EPOLLIN; // Monitor incoming read/connection triggers
    ev.data.fd = m_impl->server_fd;
    if (epoll_ctl(m_impl->epoll_fd, EPOLL_CTL_ADD, m_impl->server_fd, &ev) < 0) {
        m_impl->log("ERROR", "Failed to link server socket event framework into epoll loop.");
        m_impl->cleanup();
        return ErrorCode::EPOLL_CREATION_FAILED;
    }

    m_impl->is_running = true;
    m_impl->log("INFO", "Tuyul Net network reactor core started successfully.");

    std::vector<epoll_event> events(m_impl->options.max_epoll_events);
    std::vector<char> buffer(m_impl->options.network_buffer_size);

    // 6. Primary Orchestration Asynchronous Reactor Event Loop
    while (m_impl->is_running) {

        int num_events = 0;

        #ifdef UNIT_TEST
        // If we are testing, do not touch the real Linux kernel epoll_wait.
        // Instantly return 0 or simulate a graceful shutdown break.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (!m_impl->is_running) {
            break; 
        }
        num_events = 0; // Simulate no incoming packets during dummy cycles
        #else
        // Lock the thread indefinitely waiting for pure hardware network signals
        num_events = epoll_wait(m_impl->epoll_fd, events.data(), m_impl->options.max_epoll_events, m_impl->options.watchdog_timeout_ms);
        #endif

        if (num_events < 0) {
            // Intercept signals or software interrupts safely, crash if critical error
            if (errno == EINTR) continue;
            m_impl->log("ERROR", "Critical failure encountered during epoll_wait event cycles.");
            break;
        }

        for (int i = 0; i < num_events; ++i) {
            int current_fd = events[i].data.fd;

            if (current_fd == m_impl->server_fd) {
                // SIKLUS A: Menerima Koneksi Klien Baru
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(m_impl->server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
                
                if (client_fd < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        m_impl->log("WARN", "Encountered anomalous failure during accept processing cycle.");
                    }
                    continue;
                }

                if (!make_socket_non_blocking(client_fd)) {
                    close(client_fd);
                    continue;
                }

                // Register client socket descriptor to epoll loop for reading payloads
                epoll_event client_ev{};
                client_ev.events = EPOLLIN | EPOLLET; // Edge-Triggered paradigm for performance
                client_ev.data.fd = client_fd;
                if (epoll_ctl(m_impl->epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) < 0) {
                    m_impl->log("WARN", "Failed to register new client socket into epoll reactor.");
                    close(client_fd);
                } else {
                    m_impl->log("INFO", "New client connection accepted and hooked into reactor loop.");
                }

            } else {
                // SIKLUS B: Membaca Paket Data Masuk Dari Klien Lama
                bool connection_closed = false;
                
                // Edge-Triggered requires reading until the descriptor buffer space is totally empty
                while (true) {
                    ssize_t bytes_read = read(current_fd, buffer.data(), buffer.size());
                    
                    if (bytes_read > 0) {
                        // Pass data package upstairs using standard zero-copy std::string_view mechanics
                        if (m_impl->data_cb) {
                            m_impl->data_cb(current_fd, std::string_view(buffer.data(), bytes_read));
                        }
                    } else if (bytes_read == 0) {
                        // Client gracefully severed the pipeline link link natively
                        m_impl->log("INFO", "Client disconnected gracefully from the socket stream.");
                        connection_closed = true;
                        break;
                    } else {
                        // Safe break when OS buffer space is fully exhausted
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        m_impl->log("WARN", "Error encountered while reading network packet streams from descriptor.");
                        connection_closed = true;
                        break;
                    }
                }

                if (connection_closed) {
                    // Deregister connection from monitoring scope space and release file descriptor safely
                    epoll_ctl(m_impl->epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
                    close(current_fd);
                }
            }
        }
    }

    m_impl->cleanup();
    return ErrorCode::SUCCESS;
}

} // namespace tuyul
