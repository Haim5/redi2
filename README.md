# Redi2 - A High-Performance In-Memory Key-Value Store

Redi2 is a lightweight, high-performance in-memory key-value store written in C. It is designed to handle thousands of concurrent connections efficiently using a non-blocking I/O model with `epoll` on Linux.

It implements a subset of the Redis Serialization Protocol (RESP), making it compatible with standard Redis clients like `redis-cli`.

## Features (MVP)

*   **Non-blocking I/O:** Uses `epoll` for highly scalable event-driven networking.
*   **Concurrent Storage:** A thread-safe hash table with fine-grained locking (lock striping) to minimize contention.
*   **RESP Compatible:** Understands a subset of the Redis protocol.
*   **Core Commands:** Implements `PING`, `SET`, `GET`, and `DEL`.

## Requirements

*   A Linux-based operating system (for `epoll`).
*   `gcc` compiler.
*   `make` build tool.
*   `redis-cli` for testing (can be installed via `redis-tools`).

## Build Instructions

To compile the project, navigate to the root directory and run `make`:

```sh
make
```

This will create the executable at `bin/redi2`.

## Usage

### 1. Start the Server

Run the compiled executable to start the server. It will listen on the default port `6379`.

```sh
./bin/redi2
```

To run on a different port, provide it as an argument:

```sh
./bin/redi2 9999
```

### 2. Connect with a Client

With the server running, open a new terminal and connect using `redis-cli`:

```sh
redis-cli -p 6379
```

### Example Session

```
127.0.0.1:6379> PING
PONG
127.0.0.1:6379> SET mykey "Hello World"
OK
127.0.0.1:6379> GET mykey
"Hello World"
127.0.0.1:6379> DEL mykey
:1
127.0.0.1:6379> GET mykey
(nil)
```