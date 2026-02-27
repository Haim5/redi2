# Project Plan: High-Performance In-Memory Key-Value Store
## 1. Project Objective
To design and implement a robust, high-throughput, in-memory key-value store in C. The system will utilize non-blocking I/O (epoll/kqueue) to handle thousands of concurrent connections efficiently and employ fine-grained locking (or lock striping) to maximize parallel execution on multi-core processors. The primary goal is to achieve low-latency operations for core commands (`GET`, `SET`, `DEL`) while managing memory effectively through eviction policies, adhering to the Redis Serialization Protocol (RESP) for client compatibility.

## 2. High-Level Components

1.  **Network Layer (Event Loop):** Handles TCP connections, manages the Event Loop (using `epoll` on Linux or `kqueue` on BSD/macOS), and reads/writes raw bytes to sockets.
2.  **Protocol Parser:** Decodes raw byte streams into structured commands (RESP format) and encodes responses back to bytes. Handles partial frames and buffering.
3.  **Command Dispatcher:** Routes parsed commands to the appropriate execution handler.
4.  **Storage Engine:** The core data structure holding the key-value pairs. Handles concurrency control, atomic operations, and data retrieval.
5.  **Expiration & Eviction Manager:** Background or active processes to remove stale data (TTL) and enforce memory limits (LRU).

## 3. Prioritized Feature List

### Version 1: MVP (Minimum Viable Product)
*   **Networking:** Non-blocking TCP server using `epoll`.
*   **Protocol:** Basic RESP parsing (Simple Strings, Bulk Strings, Arrays).
*   **Storage:** Hash Table with fine-grained locking (using `pthread_mutex_t` per bucket).
*   **Commands:** `SET`, `GET`, `DEL`, `PING`.
*   **Client Support:** Compatible with standard `redis-cli`.

### Version 2: Enhancements
*   **TTL Support:** `EXPIRE` command and passive/active expiration logic.
*   **Eviction:** Max-memory limit with Approximate LRU eviction.
*   **Pipelining:** Ability to process batch commands without waiting for individual responses.
*   **Performance:** Optimize hash function (e.g., SipHash). Explore jemalloc integration.
*   **Metrics:** Internal stats (RPS, memory usage, connected clients) via `INFO` command.

### Future Improvements (Optional)
*   **Persistence:** Snapshotting (RDB style) to disk.
*   **Advanced Structures:** Lists, Sets, Hashes.
*   **Pub/Sub:** `SUBSCRIBE`, `PUBLISH`.

## 4. Implementation Roadmap

| Phase | Task | Est. Effort | Dependencies |
| :--- | :--- | :--- | :--- |
| **1** | **Project Setup & Skeleton**<br>Define project structure, `Makefile`, and logging macros. | 4 Hours | None |
| **2** | **Protocol Parser (RESP)**<br>Implement decoder for RESP types. Unit tests for partial/multiple frames. | 12 Hours | Phase 1 |
| **3** | **Storage Engine (Core)**<br>Implement Hash Table struct and API. Add basic `GET`/`SET` with locking. | 6 Hours | Phase 1 |
| **4** | **Network Layer (Event Loop)**<br>Implement `epoll` loop, `accept()` handling, and non-blocking `read()`. Integrate Parser.  Handle `EINTR` and `EAGAIN`. | 16 Hours | Phase 2, 3 |
| **5** | **Command Dispatcher**<br>Connect Network layer to Storage Engine. Implement Command pattern. | 8 Hours | Phase 4 |
| **6** | **MVP Integration Testing**<br>Verify `redis-cli` connectivity and basic operations. | 6 Hours | Phase 5 |
| **7** | **TTL & Eviction (V2)**<br>Add expiration logic (lazy + active) and LRU sampling. | 12 Hours | Phase 6 |
| **8** | **Benchmarking & Optimization**<br>Run `redis-benchmark`, profile, tune buffer sizes. | 10 Hours | Phase 7 |

**Total Estimated Effort:** ~74 Hours

## 5. Component Specifications

### A. Protocol Parser
*   **Role:** State machine converting raw buffer -> `Command` struct.
*   **Input:** `uint8_t *buffer` (from socket).
*   **Output:** `Command` struct (Type + Arguments) or status code indicating incomplete frame.
*   **Dependencies:** None.
*   **Details:**  The `Command` struct should include a dynamically allocated array of arguments. Error codes should be well-defined (enum).

### B. Network Layer
*   **Role:** Manages lifecycle of connections.
*   **Input:** TCP Packets.
*   **Output:** Writes formatted response buffers to socket.
*   **Dependencies:** Protocol Parser, Command Dispatcher.

### C. Storage Engine
*   **Role:** Manages in-memory data.
*   **Input:** Keys (`char *`), Values (`void *` or struct).
*   **Output:** Data or Success/Failure status.
*   **Dependencies:** None (Pure logic).

## 6. Testing Strategy

*   **Unit Testing:**
    *   **Parser:** Test boundary conditions (empty buffers, split packets, large payloads).
    *   **Storage:** Test concurrency (pthreads writing to same key), TTL expiration accuracy.
*   **Integration Testing:**
    *   Spin up server, connect via `Socket` client, send commands, assert responses.
    *   Test with multiple concurrent clients.
    *   Use `redis-cli` for manual verification.
*   **Fuzz Testing:** Send random garbage bytes to ensure server doesn't crash.

## 7. Benchmarking Plan

*   **Tools:** `redis-benchmark` (standard tool), `memtier_benchmark`.
*   **Metrics:**
    *   **Throughput:** Requests per second (target: >50k RPS on single core).
    *   **Latency:** P99 latency (target: <1ms).
    *   **Memory:** RSS usage under load.
*   **Scenarios:**
    *   100% GET (Read heavy).
    *   50% GET / 50% SET (Mixed).
    *   Pipeline depth 1 vs 50.
*   **Record system-level metrics:** CPU utilization, context switches, network I/O.

## 8. Risks, Unknowns, & Assumptions

*   **Risk:** **Memory Management.** Manual memory management in C can lead to leaks or fragmentation.
    *   *Mitigation:* Use Valgrind/ASan during testing. Consider a slab allocator for fixed-size objects.
*   **Risk:** **Non-blocking I/O Complexity.** Handling partial reads correctly is error-prone.
    *   *Mitigation:* Rigorous unit testing of the Parser with fragmented inputs.
*   **Assumption:** The dataset fits in RAM.
    *   *Validate:* Monitor memory usage closely during benchmarking.
    *   *Mitigation:* Implement max-memory eviction policies early.

## 9. Directory Structure
```text
/
├── src
│   ├── main.c              // Entry point
│   ├── server.c            // Networking logic (epoll/kqueue)
│   ├── protocol.c          // RESP Parsing logic
│   ├── hashtable.c         // Storage engine
│   └── commands.c          // Command implementations
├── include
│   ├── server.h
│   ├── protocol.h
│   ├── hashtable.h
│   └── commands.h
├── tests
│   └── test_main.c         // Unit tests
├── Makefile
└── README.md
```

Notes:
the code should be in c, not java
the code should be linux-suitabledocument