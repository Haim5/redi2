# Research: High-Performance In-Memory Key-Value Store Design

## 1. System Components Breakdown

Building a robust key-value store requires a modular architecture where networking, parsing, and storage logic are decoupled.

### A. Network Listener (The Acceptor)
*   **Role:** Responsible for binding to a TCP port and accepting incoming connections.
*   **Design:** In a blocking model, this loops infinitely calling `accept()`. In a non-blocking (NIO/epoll) model, this registers the server socket with an event selector.
*   **Critical Detail:** Must handle "Backlog" (the queue of pending connections) correctly to avoid rejecting clients during bursty traffic.

### B. I/O Handler & Protocol Parser
*   **Role:** Reads raw bytes from the socket and converts them into structured commands.
*   **Challenge:** TCP is a stream, not a packet protocol. You may receive partial commands or multiple commands in a single read.
*   **Solution:** A buffering mechanism is required. The parser must check if a full command frame is available before processing.
    *   *Line-based protocols* (e.g., `SET key value\r\n`) are easy to debug but require scanning for delimiters.
    *   *Length-prefixed protocols* (e.g., Redis RESP) allow for faster parsing by reading the length header first, then reading the exact payload size.

### C. Command Dispatcher / Execution Engine
*   **Role:** Takes a parsed command object (e.g., `CommandType.SET`, `key`, `value`) and routes it to the appropriate logic.
*   **Design:** Often implemented as a Strategy Pattern or a Command Pattern. This layer enforces syntax validation (e.g., ensuring `SET` has exactly two arguments).

### D. Storage Engine (The Core)
*   **Role:** Manages the in-memory data structures.
*   **Design:** Wraps the underlying hash map or tree structures. It abstracts away the complexity of locking, memory management, and eviction, exposing simple API methods like `put`, `get`, and `delete`.

---

## 2. Concurrency Models: Choices and Trade-offs

The concurrency model dictates how the server scales with the number of connected clients.

### Option A: Thread-Per-Client (Blocking I/O)
*   **Description:** Every new connection spawns a dedicated thread.
*   **Pros:** Simple to implement; linear logic flow.
*   **Cons:** Does not scale. 10,000 clients = 10,000 threads. Context switching overhead becomes the bottleneck. High memory consumption per thread stack.

### Option B: Thread Pool (Blocking I/O)
*   **Description:** A fixed pool of threads handles requests.
*   **Pros:** Limits resource usage.
*   **Cons:** If a client is slow (or the network is slow), a thread remains blocked waiting for I/O, making it unavailable for other clients.

### Option C: Non-Blocking I/O (Event Loop / Reactor Pattern) - **Recommended**
*   **Description:** A single thread (or small pool) uses OS-level multiplexing (`epoll`, `kqueue`, Java NIO `Selector`) to monitor thousands of sockets. Threads only wake up when data is ready to be read.
*   **Pros:** Extremely high scalability; low memory footprint. Used by Redis, Node.js, and Netty.
*   **Cons:** Higher implementation complexity. The event loop must *never* block (no heavy computation or sleep).
*   **Hybrid Approach:** Use an Event Loop for I/O and a separate Worker Thread Pool for heavy command execution (though for simple KV operations, single-threaded execution is often faster due to lack of context switching).

---

## 3. Data Structures & Storage

### Primary Storage: Hash Map
*   **Requirement:** O(1) average time complexity for `GET` and `SET`.
*   **Implementation:**
    *   **Global Lock:** A standard `HashMap` protected by a `ReentrantReadWriteLock`. Simple, but write operations become a bottleneck under high concurrency.
    *   **Lock Striping (Sharding):** Divide the key space into N segments (e.g., 16 or 32). Hash the key to find the segment, and lock only that segment. This significantly reduces contention (similar to Java's `ConcurrentHashMap`).

### Expiration Management: Priority Queue or Wheel
*   **Requirement:** Efficiently remove keys after a Time-To-Live (TTL).
*   **Structure:** A Min-Heap (Priority Queue) storing objects sorted by expiration timestamp.
*   **Trade-off:** Checking the heap on every operation is expensive.
*   **Alternative:** **Lazy Expiration** (check TTL only when the key is accessed) combined with **Active Sampling** (periodically test random keys and delete expired ones—the Redis approach).

### Eviction Policy: LRU (Least Recently Used)
*   **Requirement:** When memory is full, remove the least useful data.
*   **Structure:** A Hash Map combined with a Doubly Linked List.
    *   *Access:* Move node to the head of the list.
    *   *Eviction:* Remove node from the tail.
*   **Approximation:** Maintaining a strict linked list requires pointer updates on *every* read, which requires write-locks even for read operations. An *approximated LRU* (randomly sampling 5 keys and evicting the oldest) is often preferred for performance.

---

## 4. Performance Considerations

### Lock Contention
*   **The Problem:** If multiple threads try to acquire the same lock, they queue up, destroying throughput.
*   **Mitigation:** Use lock striping (sharding) or lock-free data structures (CAS operations). For read-heavy workloads, `ReadWriteLocks` allow multiple simultaneous readers.

### Throughput vs. Latency
*   **Throughput:** Total requests per second. Optimized by batching and pipelining.
*   **Latency:** Time taken for a single request. Optimized by minimizing queuing and context switching.
*   **GC Pauses (If using Java/Go):** Large heaps lead to long Garbage Collection pauses. Object pooling (reusing command objects and buffers) can reduce allocation pressure.

### Networking Bottlenecks
*   **Nagle’s Algorithm:** TCP by default buffers small packets to reduce overhead. This ruins latency for a KV store. **Must enable `TCP_NODELAY`** to disable Nagle’s algorithm.
*   **Buffer Sizing:** Allocating a new buffer for every read is slow. Reusing direct byte buffers is critical for high performance.

### Benchmarking
*   **Tools:** `redis-benchmark` or `memtier_benchmark`.
*   **Metrics:** Requests Per Second (RPS), P99 Latency (the latency below which 99% of requests fall).

---

## 5. Typical Pitfalls

1.  **The "Thundering Herd":** When many threads wake up simultaneously to handle an event but only one can proceed, causing CPU spikes.
2.  **Partial Reads/Writes:** Assuming `socket.read()` returns the full message. It often returns only part of it, requiring stateful parsing.
3.  **Deadlocks:** If implementing complex commands (e.g., `MSET` moving keys between shards), acquiring locks in inconsistent orders will cause deadlocks. Always acquire locks in a deterministic order (e.g., by hash value).
4.  **Unbounded Growth:** Failing to set a max memory limit or max connection limit will eventually crash the server (OOM).

---

## 6. Optional Extensions

### A. TTL (Time To Live)
*   **Pros:** Essential for caching use cases (session storage).
*   **Cons:** Adds complexity to the storage engine (need to store metadata alongside value).

### B. Persistence (Snapshots/AOF)
*   **Pros:** Data survives restarts.
*   **Cons:** Disk I/O is slow. Must be done asynchronously (e.g., `fork()` a child process to write the snapshot) to avoid blocking the main server loop.

### C. Pub/Sub
*   **Pros:** Enables real-time messaging patterns.
*   **Cons:** Requires a different internal structure (subscriptions list) separate from the key-value store.

### D. Metrics & Introspection
*   **Pros:** Critical for production (monitoring memory usage, hit/miss ratio, connected clients).
*   **Cons:** Minimal overhead if implemented using atomic counters.
