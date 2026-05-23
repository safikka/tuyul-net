# tuyul-engine

`tuyul-engine` is a lightweight, low-latency, and high-performance asynchronous networking engine written from scratch in **Modern C++ (C++17)**. 

The main goal here is to build a rock-solid networking foundation that compiles with absolutely *zero external dependencies*. Inspired from `boost` but stripped down to be ultra-modular and easy to grasp.

---

## 🗺️ System Design

The infrastructure relies on **Event-Driven Reactor Pattern**. To get maximum performance on Linux, directly hook into the kernel's native **`epoll`** mechanism, combined with a strict zero-copy buffer strategy prevent hog RAM or CPU cycles.

```plantuml
@startuml
skinparam BackgroundColor #FFFFFF
skinparam Handwritten false
skinparam ComponentStyle rectangle

package "tuyul-example (Living Examples Catalog)" {
    [Your App / Example Files] as app
}

package "tuyul-engine (Core Framework)" {
    package "Application Layer (Future Roadmap)" {
        [http_server] as http
        [websocket] as ws
        [mqtt] as mqtt
        [sftp] as sftp
        [gRPC] as grpc
    }
    
    package "Transport Layer (Current Focus)" {
        [tuyul-tcp] as tcp
        [tuyul-udp] as udp
    }
}

package "Operating System Layer" {
    [Linux Kernel (Epoll Engine & Native Sockets)] as kernel
}

' Data Flow & Relationships
app --> http : "Uses High-Level API"
app --> tcp : "Uses Bare TCP (C++17 Callbacks & ErrorCodes)"

http --> tcp : "Processes Raw Buffers"
ws --> http : "Upgrades Connection"
mqtt --> tcp : "Decodes Binary PDUs"
sftp --> tcp : "Handles Secure Streams"
grpc --> tcp : "Serializes Protobuf via HTTP/2"

tcp --> kernel : "I/O Multiplexing (epoll_wait)"
udp --> kernel : "Connectionless Datagrams (UDP)"

@enduml
```
---