**Contents**
- [1. Introduction](#sec1)
- [2. Architecture](#sec2)
  - [2.1. Event-Based Architecture (EBA)](#sec21)
  - [2.2. Cross-Platform Abstraction Layers](#sec22)
  - [2.3. Modularity and Extensibility](#sec23)
- [3. Components](#sec3)
- [4. Build and Run](#sec4)
- [5. TODOs](#sec5)

**PREFACE**\
I created this repository as a reference for my job applications.
The code given in this repository is:
- to introduce my experience with TCP/IP,
- to present my background with Event-Based Architecture (**EBA**).

# 1. Introduction <a id='sec1'></a>
This repository contains a working, cross-platform and event-driven TCP communication system implemented in modern C++20:
- Event-Based Architecture (**EBA**) and the **Reactor Pattern**
- **Cross-platform** network programming (Linux and Windows)
- Modular, extensible and flexible system design
- **Concurrency models:** single-threaded (ST) and handler-parallel (HP)
- **Select-based** and **poll-based** event loops (``epoll``/``kqueue``/``IOCP`` is currently in **TODO** list)
- Clean separation of concerns: event-loop, handlers, queue, thread pool, sockets
- Production-level error handling and platform abstraction layers

Followings are the components of the event-based system:
- A fully functional TCP server (select-based and poll-based)
- A fully functional TCP client
- A complete event-driven reactor built from scratch
- An extensible handler system supporting read, write, transform, redirect and accept operations

The code is written in modern C++20 with a focus on:
- correctness
- cross-platform support
- modularity
- extensibility
- clarity of architectural design

# 2. Architecture <a id='sec2'></a>

## 2.1 Event-Based Architecture (EBA) <a id='sec21'></a>
The entire system follows a strict **event-driven design**.
Handlers return ``Reactor_Event`` objects, which instruct the event-loop to:
- register new file descriptors
- unregister file descriptors
- attach new handlers
- switch read/write modes
- close connections
- schedule follow-up operations

Every handler is isolated, stateless and operates on the single responsibility principle, making the design extremely flexible.

## 2.2 Cross-Platform Abstraction Layers <a id='sec22'></a>
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

## 2.3 Modularity and Extensibility <a id='sec23'></a>
The system is designed around clean modular components:
- ``IEvent_Loop``
- ``IEvent_Handler``
- ``Reactor_Event``
- ``Concurrent_Queue`` (see [lock-free](https://github.com/BarisAlbayrakIEEE/lock_free.git))
- ``Thread_Pool`` (see [lock-free](https://github.com/BarisAlbayrakIEEE/lock_free.git))
- ``Socket`` (RAII wrapper)

Adding new event loops (epoll/kqueue/IOCP) requires implementing only a small specialized version of ``Event_Loop``.
Adding new handlers (logging, broadcasting, protocol parsers) requires implementing a class derived from ``IEvent_Handler``.

# 3. Components <a id='sec3'></a>
- Cross-platform TCP server and client
- Select-based event loop (Low/ST, Low/HP)
- Poll-based event loop (Mid/ST and Mid/HP)
- Accept, read, write, redirect, transform handlers
- Full string-forward and string-transform pipelines
- Safe RAII socket wrapper
- Reactor pattern with handler-chaining
- Non-blocking stdin support (where available)
- Fully working binaries for both Linux and Windows
- Unified ``poll``/``WSAPoll`` abstraction

# 4. Build and Run <a id='sec4'></a>
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

# 5. TODOs <a id='sec5'></a>
- Tests (gtest)
- Benchmarks (Google Benchmark)
- High-performance server backends (``epoll``/``kqueue``/``IOCP``) will be added to enable **O(1)** event-scaling and true high-performance networking
