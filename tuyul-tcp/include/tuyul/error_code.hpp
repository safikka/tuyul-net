#ifndef TUYUL_ERROR_CODE_HPP
#define TUYUL_ERROR_CODE_HPP

namespace tuyul {

/**
 * @brief Strongly-typed error codes for the tuyul-net network ecosystem.
 * Prevents runtime overhead by avoiding throwing raw C++ exceptions.
 */
enum class ErrorCode {
    SUCCESS = 0,                // Operation completed successfully without issues
    SOCKET_CREATION_FAILED,     // Failed to allocate system socket descriptor via OS
    SET_OPTION_FAILED,          // Failed to configure socket options (e.g., SO_REUSEADDR)
    BIND_FAILED,                // Port is already in use or restricted by the operating system
    LISTEN_FAILED,              // Failed to put the socket into listening state
    EPOLL_CREATION_FAILED,      // Failed to initialize the Linux epoll instance
    ACCEPT_FAILED,              // Error while accepting an incoming client connection
    READ_FAILED,                // Network read error encountered during payload retrieval
    CLIENT_DISCONNECTED,        // Client gracefully closed the connection during an operation
    TIMEOUT                     // Watchdog timer kicked in; client was idle for too long
};

} // namespace tuyul

#endif // TUYUL_ERROR_CODE_HPP
