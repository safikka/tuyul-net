# tuyul-tcp Technical Documentation

`tuyul-tcp` is the foundational transport layer component of the `tuyul-net` ecosystem. It delivers a non-blocking, asynchronous, and high-performance TCP implementation—providing both a robust Server Core and an active Client Core built natively on top of the Linux **Kernel Epoll API** using the **Reactor Design Pattern**.

---

## ⚙️ Configuration Subsystem (`TcpOptions`)

To prevent performance rigidity, `tuyul-tcp` avoids hardcoded tuning values. Instead, it shares a single unified structure `tuyul::TcpOptions` for both servers and clients, allowing users to fine-tune network socket behaviors easily.

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
| `watchdog_timeout_ms` | `int` | `5000` (5s) | The active poll timeout duration. It allows the internal background threads to periodically wake up to check for operational flags, cleanly preventing thread lock deadlocks. |

---

## 🛑 Strongly-Typed Error Subsystem (`ErrorCode`)

`tuyul-tcp` eliminates the runtime overhead of raw C++ exceptions and messy magic numbers by enforcing a strongly-typed `enum class` diagnostics subsystem across all operations.

### Diagnostics Mapping Table



| Constant Name | Underlying Value | Operational Diagnostic Trigger |
| :--- | :--- | :--- |
| `SUCCESS` | `0` | The network transaction, connection link, or lifecycle boot phase completed perfectly. |
| `SOCKET_CREATION_FAILED` | `1` | Failed to allocate an active socket descriptor from the host operating system kernel. |
| `SET_OPTION_FAILED` | `2` | System failed to configure mandatory socket options (e.g., setting `SO_REUSEADDR` or transitioning to non-blocking states). |
| `BIND_FAILED` | `3` | Interface collision or handshake rejection. The port/address is busy, locked, or restricted by the host. |
| `LISTEN_FAILED` | `4` | Failed to transition the socket state machine into the listening backlog manager phase. |
| `EPOLL_CREATION_FAILED`| `5` | Critical operational error initializing the native subsystem instance of the Linux kernel epoll core. |
| `ACCEPT_FAILED` | `6` | Non-fatal network glitch encountered while pulling a new connection descriptor out of the backlog. |
| `READ_FAILED` | `7` | Soft or hard operational fault encountered while executing payload write/send or read data streams. |
| `CLIENT_DISCONNECTED`  | `8` | Klien or remote interface successfully initiated a graceful socket closure process from their end. |
| `TIMEOUT` | `9` | Session reaped or loop cycled after idling beyond the allocated poll threshold window. |

---

## 📖 Public API Reference: `TcpServer` Engine

The server core acts as a passive asynchronous network reactor that binds to a local interface port and orchestrates high-density client event loops.

### 1. Instance Spawning & Resource Ownership
```cpp
#include <tuyul/tcp_server.hpp>
#include <tuyul/tcp_options.hpp>

// Explicitly binding a port with default configuration profiles
tuyul::TcpServer server(8080);

// Fine-tuning the reactor engine using customized TcpOptions parameters
tuyul::TcpOptions server_config;
server_config.network_buffer_size = 8192; // 8 KB data ingestion steps
tuyul::TcpServer custom_server(8080, server_config);
```
*Note: Copy semantics are explicitly deleted (`= delete`) to prevent unsafe duplication of active socket descriptors. Modern C++ move semantics are fully supported.*

### 2. Operational Logger & Callback Attachments
`tuyul-tcp` follows the "polite library" standard—it remains **completely silent** unless you explicitly inject a logging callback to route internal state events.
```cpp
// Inject an app-level logger callback to capture runtime milestones
server.set_logger([](std::string_view log_level, std::string_view log_message) {
    std::cout << "[" << log_level << "] ServerNotice: " << log_message << std::endl;
});

// Register the data trap callback using zero-copy modern C++17 string views
server.on_data([](int client_fd, std::string_view raw_payload) {
    // This exact checkpoint is where future application layer parsers (HTTP, MQTT) hook down!
    std::cout << "Intercepted " << raw_payload.length() << " bytes from descriptor: " << client_fd << "\n";
});
```

### 3. Lifecycle Execution Controls
```cpp
// Launches the synchronous blocking event loop (Returns ErrorCode)
tuyul::ErrorCode status = server.start();

// Request a graceful teardown from any external application thread context
server.stop();
```

---

## 📖 Public API Reference: `TcpClient` Engine

The client core acts as an active asynchronous transport connector that establishes dedicated outgoing pipelines to remote endpoints, running its epoll loops entirely within isolated internal background threads.

### 1. Initialization & Configuration Wiring
```cpp
#include <tuyul/tcp_client.hpp>
#include <tuyul/tcp_options.hpp>

// Instantiate a client target using standard runtime defaults
tuyul::TcpClient client;

// Customizing specific client ingestion capacities using the shared TcpOptions structure
tuyul::TcpOptions client_config;
client_config.network_buffer_size = 16384; // 16 KB optimization for heavy replies
tuyul::TcpClient custom_client(client_config);
```

### 2. Logging & Response Callback Routines
```cpp
// Attach a localized logging pipeline
client.set_logger([](std::string_view level, std::string_view message) {
    std::cout << "[" << level << "] ClientNotice: " << message << "\n";
});

// Intercept server replies reactively with zero heap memory allocation overhead
client.on_data([](std::string_view raw_response) {
    std::cout << "Received server reply payload data stream:\n" << raw_response << "\n";
});
```

### 3. Connection Lifecycles & Active Data Transmissions
```cpp
// Establish an asynchronous non-blocking connection link to a remote server
tuyul::ErrorCode status = client.connect_to("127.0.0.1", 8080);

if (status == tuyul::ErrorCode::SUCCESS) {
    // Dispatch data blocks down the active wire synchronously
    client.send_data("Hello from the Tuyul-Net client layer pipeline!");
}

// Tear down active network links and cleanly terminate internal thread worker routines
client.disconnect();
```

---

## 🔒 Thread-Safety & Graceful Interrupt Compliance

*   **OS Signal Immunity**: `tuyul-tcp` deliberately avoids touching native OS signal handlers (e.g., `<csignal>`). Senders of `SIGINT` (Ctrl+C) must be intercepted gracefully at the application tier (`main.cpp`), which should subsequently invoke `server.stop()` or `client.disconnect()`.
*   **Asynchronous Teardown Assurance**: Thanks to the persistent timeout constraints enforced within our `epoll_wait` engines, calling `stop()` or `disconnect()` from any concurrent thread context guarantees a highly responsive loop break and flawless RAII-backed descriptor cleanup within a few milliseconds.
