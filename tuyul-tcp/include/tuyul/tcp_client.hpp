#ifndef TUYUL_TCP_CLIENT_HPP
#define TUYUL_TCP_CLIENT_HPP

#include <tuyul/error_code.hpp>
#include <tuyul/tcp_options.hpp>
#include <functional>
#include <string_view>
#include <string>
#include <memory>

namespace tuyul {

class TcpClient {
public:
    // Callbacks for loose coupling and zero-copy packet ingestion
    using DataCallback = std::function<void(std::string_view raw_payload)>;
    using LoggerCallback = std::function<void(std::string_view log_level, std::string_view log_message)>;

    // Constructor accepting dynamic hardware tuning parameters
    explicit TcpClient(TcpOptions options = TcpOptions{});
    
    // RAII Destructor ensuring graceful closure of client socket connections
    ~TcpClient();

    // Enforce strict resource ownership rules by deleting copy semantics
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    // Support clean pointer move semantics transfers
    TcpClient(TcpClient&&) noexcept;
    TcpClient& operator=(TcpClient&&) noexcept;

    /**
     * @brief Hooks the host application custom logger framework channel.
     */
    void set_logger(LoggerCallback logger);

    /**
     * @brief Registers the trap event callback to intercept incoming zero-copy payloads.
     */
    void on_data(DataCallback callback);

    /**
     * @brief Establishes an asynchronous connection link to the target host and port destination.
     * @return ErrorCode status detailing connection success or structural system breakdown.
     */
    ErrorCode connect_to(const std::string& host, int port);

    /**
     * @brief Transmits a payload data buffer over the network wire synchronously.
     * @return ErrorCode detailing the write sequence operation status.
     */
    ErrorCode send_data(std::string_view payload);

    /**
     * @brief Halts the active client reactor loop and gracefully disconnects the socket descriptor.
     */
    void disconnect();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tuyul

#endif // TUYUL_TCP_CLIENT_HPP
