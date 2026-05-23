# tuyul-net

`tuyul-net` is a lightweight, low-latency, and high-performance asynchronous networking engine written from scratch in **Modern C++ (C++17)**. 

The main goal here is to build a rock-solid networking foundation that compiles with absolutely *zero external dependencies*. Inspired from `boost` but stripped down to be ultra-modular and easy to grasp.

---

## System Design

The infrastructure relies on **Event-Driven Reactor Pattern**. To get maximum performance on Linux, directly hook into the kernel's native **`epoll`** mechanism, combined with a strict zero-copy buffer strategy prevent hog RAM or CPU cycles.

```mermaid
flowchart LR
    %% Theme Color Settings (Muted Tones)
    classDef appStyle fill:#EBF5FB,stroke:#2E86C1,stroke-width:2px,color:#1B4F72;
    classDef engineStyle fill:#F4ECF7,stroke:#7D3C98,stroke-width:2px,color:#4A235A;
    classDef roadmapStyle fill:#F9EBEA,stroke:#A93226,stroke-width:1px,stroke-dasharray: 4 4,color:#78281F;
    classDef osStyle fill:#EAECEE,stroke:#5D6D7E,stroke-width:2px,color:#2C3E50;

    %% --- LAYER 1: APP DEMO ---
    subgraph AppLayer [" tuyul-example (App Examples) "]
        app[" Examples app"]
    end
    class app appStyle;

    %% --- LAYER 2: ENGINE CORE ---
    subgraph EngineLayer [" tuyul-net (Core Framework) "]
        subgraph Roadmap [" Application Layer (Future Roadmap) "]
            http["http_server"]
            ws["websocket"]
            mqtt["mqtt"]
            sftp["sftp"]
            grpc["gRPC"]
        end

        subgraph Transport [" Transport Layer "]
            tcp["tuyul-tcp"]
            udp["tuyul-udp"]
        end
    end
    class http,ws,mqtt,sftp,grpc roadmapStyle;
    class tcp,udp engineStyle;

    %% --- LAYER 3: OS KERNEL ---
    subgraph OsLayer [" Operating System Layer "]
        kernel["Linux Kernel<br>(Epoll & Native Sockets)"]
    end
    class kernel osStyle;

    %% --- DATA FLOW & CONNECTIONS ---
    app -->|Uses High-Level API| http
    app -->|Uses Bare TCP| tcp

    http -->|Processes Buffers| tcp
    ws  -->|Upgrades Conn| http
    mqtt-->|Decodes PDU| tcp
    sftp-->|Handles Files| tcp
    grpc-->|Serializes HTTP/2| http

    tcp -->|epoll_wait| kernel
    udp -->|Datagrams| kernel
```
---
## Technical Documentation

Dive deep into the architectural specifications and API references for each component:
*   [TCP Core Engine Interface](docs/tuyul-tcp.md)
*   [UDP Core Datagram Interface](docs/tuyul-udp.md) (Coming Soon)
*   [HTTP/1.1 & HTTP/2 Layer Specification](docs/tuyul-http.md) (Coming Soon)
*   [MQTT Broker Protocol Specification](docs/tuyul-mqtt.md) (Coming Soon)
---