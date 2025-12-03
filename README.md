# socket
socket programming with C

I will prepare the README soon!

See lock_free repository for the lock-free data structures and thread pools.
Please keep in mind that lock-free repo is also under construction!








# Cross-Platform Event-Based TCP Servers and Clients (C++20)

This repository is prepared to **demonstrate my real-world software engineering skills** for job applications.
It contains a fully working, cross-platform, event-driven TCP communication system implemented in modern C++20.

The project showcases:
- High-quality software architecture
- Event-Based Architecture (EBA) and the Reactor Pattern
- Cross-platform network programming (Linux and Windows)
- Advanced C++ design: templates, concepts, move semantics, RAII, multi-threading
- Modular, extensible and flexible system design
- Concurrency models: single-threaded (ST) and handler-parallel (HP)
- Select-based and poll-based event loops (epoll/kqueue/IOCP is currently in TODO list)
- Clean separation of concerns: event-loop, handlers, queue, thread pool, sockets
- Production-level error handling and platform abstraction layers

## 1. Overview

This project implements:
- A fully functional TCP server (select-based and poll-based)
- A fully functional TCP client
- A complete event-driven reactor built from scratch
- An extensible handler system supporting read, write, transform, redirect, and accept operations

The code is written in clean, modern C++20 with a focus on:
- correctness
- cross-platform support
- modularity
- extensibility
- clarity of architectural design

## 2. Architecture

### 2.1 Event-Based Architecture (EBA)

The entire system follows a strict **event-driven design**.
Handlers return ``Reactor_Event`` objects, which instruct the event-loop to:
- register new file descriptors
- unregister file descriptors
- attach new handlers
- switch read/write modes
- close connections
- schedule follow-up operations

Every handler is isolated, stateless, and operates on the single responsibility principle, making the design extremely flexible.

### 2.2 Cross-Platform Abstraction Layers

The project provides unified wrappers for:
- ``select()`` on Linux / Windows
- ``poll()`` and ``WSAPoll()``
- socket creation, bind, listen, accept, connect
- address resolution
- platform error handling
- network interface enumeration

This allows the same event-loop implementation to run on:
- Linux
- Windows
- WSL
- macOS (select/poll)

### 2.3 Modularity and Extensibility

The system is designed around clean modular components:
- ``IEvent_Loop``
- ``IEvent_Handler``
- ``Reactor_Event``
- ``Concurrent_Queue`` (see [lock-free](https://github.com/BarisAlbayrakIEEE/lock_free.git))
- ``Thread_Pool`` (see [lock-free](https://github.com/BarisAlbayrakIEEE/lock_free.git))
- ``Socket`` (RAII wrapper)

Adding new event loops (epoll/kqueue/IOCP) requires implementing only a small specialized version of ``Event_Loop``.
Adding new handlers (logging, broadcasting, protocol parsers) requires implementing a class derived from ``IEvent_Handler``.

## 3. Implemented Features

- Cross-platform TCP server and client
- Select-based event loop (Low/ST, Low/HP)
- Poll-based event loop (Mid/ST and Mid/HP coming)
- Accept, read, write, redirect, transform handlers
- Full string-forward and string-transform pipelines
- Safe RAII socket wrapper
- Reactor pattern with handler-chaining
- Non-blocking stdin support (where available)
- Fully working binaries for both Linux and Windows

## 4. Why This Repository Matters for Recruiters

This repository is intentionally built to demonstrate my real-world engineering skills:
- Systems programming
- Cross-platform code design
- Network programming
- Concurrency
- Flexible C++ template-based architecture
- Low-level OS interaction
- Event-driven systems
- Error-safe RAII resource management
- Clean modular abstractions
- Maintainability and extensibility

It reflects the same architectural discipline I apply when designing large-scale, production-grade systems in C++.

## 5. Build and Run

The code is:
- fully compilable
- fully functional
- tested manually on Linux and Windows

To build:

```
mkdir build
cd build
cmake -DCMAKE_CXX_STANDARD=20 ..
cmake --build .
```

## 6. Roadmap

### Completed

- Cross-platform event-driven TCP server
- Select-based event loops
- Handler-parallelism via thread-pool
- Unified poll/WSAPoll abstraction
- Modular handler system
- Reactor event chaining

### TODOs

- Tests (gtest)
- Benchmarks (Google Benchmark)
- High-performance server backends (``epoll``/``kqueue``/``IOCP``) will be added to enable **O(1)** event-scaling and true high-performance networking

## 7. License

MIT License (free for commercial and personal use).

## 8. Contact

For questions or job-related inquiries, feel free to reach out.
