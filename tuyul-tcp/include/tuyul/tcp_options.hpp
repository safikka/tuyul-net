#ifndef TUYUL_TCP_OPTIONS_HPP
#define TUYUL_TCP_OPTIONS_HPP

namespace tuyul {

/**
 * @brief Configuration tuning parameters for the TcpServer infrastructure.
 */
struct TcpOptions {
    // Maximum networking event cues processed per single epoll iteration cycle
    int max_epoll_events{64};

    // Allocation footprint size (in bytes) for the raw network ingestion buffer
    int network_buffer_size{4096};

    // Watchdog trigger duration (in milliseconds) to reap idle hanging sessions
    int watchdog_timeout_ms{5000};
};

} // namespace tuyul

#endif // TUYUL_TCP_OPTIONS_HPP
