# tuyul-tcp Technical Documentation

`tuyul-tcp` is the foundational transport layer component of the `tuyul-net` ecosystem. It implements a non-blocking, asynchronous, and high-performance TCP server core built natively on top of the Linux **Kernel Epoll API** using the **Reactor Design Pattern**.

---

## ⚙️ Configuration Subsystem (`TcpOptions`)

To prevent performance rigidity, `tuyul-tcp` avoids hardcoded tuning values. Instead, it provides the `tuyul::TcpOptions` structural layout, allowing users to fine-tune socket behaviors based on server memory constraints and expected connection density.

### Configuration Specification

```cpp
namespace tuyul {
struct TcpOptions {
    int max_epoll_events{64};
    int network_buffer_size{4096};
    int watchdog_timeout_ms{5000};
};
}
```


| Parameter Name | Data Type | Default Value | Description |
| :--- | :--- | :--- | :--- |
| `max_epoll_events` | `int` | `64` | The maximum number of continuous network events fetched by the kernel per single `epoll_wait` loop iteration. |
| `network_buffer_size` | `int` | `4096` (4 KB) | The byte allocation footprint for the raw hardware network ingestion buffer. Higher values optimize large packet streams. |
| `watchdog_timeout_ms` | `int` | `5000` (5s) | The active poll timeout duration. It allows the core thread loop to periodically wake up to check for shutdown flags, preventing deadlocks. |

---

## 🛑 Strongly-Typed Error Subsystem (`ErrorCode`)

`tuyul-tcp` eliminates the runtime overhead of C++ exceptions and magic numbers by utilizing a strongly-typed `enum class` error subsystem. 

### Diagnostics Mapping Table


| Constant Name | Underlying Value | Operational Diagnostic Trigger |
| :--- | :--- | :--- |
| `SUCCESS` | `0` | The network transaction or lifecycle boot phase completed without anomalies. |
| `SOCKET_CREATION_FAILED` | `1` | Failed to allocate an active socket descriptor from the host operating system kernel. |
| `SET_OPTION_FAILED` | `2` | System failed to configure mandatory socket options (e.g., setting `SO_REUSEADDR` or O_NONBLOCK). |
| `BIND_FAILED` | `3` | Interface collision detected. The requested local port is busy, locked, or restricted by the OS. |
| `LISTEN_FAILED` | `4` | Failed to transition the socket state machine into the listening backlog queue phase. |
| `EPOLL_CREATION_FAILED`| `5` | Critical operational error initializing the native subsystem instance of Linux epoll. |
| `ACCEPT_FAILED` | `6` | Non-fatal network glitch encountered while pulling a new connection out of the backlog. |
| `READ_FAILED` | `7` | Soft or hard hardware read fault encountered during mid-stream packet processing. |
| `CLIENT_DISCONNECTED`  | `8` | Client successfully initiated a graceful socket closure process from their end. |
| `TIMEOUT` | `9` | Connection reaped after idling beyond the allocated runtime threshold window. |

---

## 📖 Core Public API Reference

All architectural components are fully decoupled and enclosed within the `tuyul` namespace to ensure zero global scope pollution.

### 1. Class Construction & Resource Ownership
```cpp
#include <tuyul/tcp_server.hpp>
#include <tuyul/tcp_options.hpp>

// Explicitly bindings a port with default configuration profiles
tuyul::TcpServer server(8080);

// Fine-tuning the reactor engine using customized TcpOptions profiles
tuyul::TcpOptions my_config;
my_config.network_buffer_size = 8192; // 8 KB
tuyul::TcpServer custom_server(8080, my_config);
```
*Note: Copy semantics are explicitly deleted (`= delete`) to ensure unique ownership of host file descriptors. Move semantics are natively supported.*

### 2. Operational Logger Injection Callback
`tuyul-tcp` follows the polite library standard—it remains **completely silent** and never pollutes stdout/stderr unless a custom logger callback is hooked.
```cpp
server.set_logger([](std::string_view log_level, std::string_view log_message) {
    // Pipe the data into standard console logs, file descriptors, or an external engine
    std::cout << "[" << log_level << "] CoreNotice: " << log_message << std::endl;
});
```

### 3. Asynchronous Zero-Copy Data Ingestion
Payload streams are intercepted and passed upstairs via standard C++17 `std::string_view`. This minimizes heap fragmentation by giving your application direct look-ahead access to the raw network buffer.
```cpp
server.on_data([](int client_fd, std::string_view raw_payload) {
    // Parse your application layers (HTTP, MQTT, etc.) directly on top of this zero-copy view
    std::cout << "Intercepted " << raw_payload.length() << " bytes from client descriptor: " << client_fd;
});
```

### 4. Lifecycle Controls & Graceful Shuts
```cpp
// Triggers the synchronous event loop block (Returns ErrorCode)
tuyul::ErrorCode status = server.start();

// Request a graceful lifecycle halt from any external app worker thread context
server.stop();
```

---

## Thread-Safety & Graceful Interrupt Compliance

*   **OS Signal Immunity**: `tuyul-tcp` deliberately avoids registering native signal handlers (e.g., `<csignal>`). Senders of `SIGINT` (Ctrl+C) must be intercepted at the application tier (`main.cpp`), which should subsequently invoke `server.stop()`.
*   **Asynchronous Teardown Assurance**: Thanks to the internal timeout constraints enforced within the `epoll_wait` loop, invoking `server.stop()` from any concurrent context guarantees a responsive loop break and safe RAII-backed descriptor cleanup within a few milliseconds.
