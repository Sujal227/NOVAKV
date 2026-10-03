# NovaKV

NovaKV is a small Redis-style, in-memory key-value database written in C++17.

It supports:

- TCP client/server communication
- Remote LAN clients
- Multiple concurrent clients through a thread pool
- Thread-safe key-value operations
- Disk persistence
- TTL / key expiration
- Case-insensitive commands (`SET`, `Set`, `sEt`, etc.)
- C++ GoogleTest unit tests

## Project structure

```text
NovaKV/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── client.hpp
│   ├── kv_store.hpp
│   ├── persistence.hpp
│   ├── server.hpp
│   └── thread_pool.hpp
├── src/
│   ├── client.cpp
│   ├── client_main.cpp
│   ├── kv_store.cpp
│   ├── main.cpp
│   ├── persistence.cpp
│   ├── server.cpp
│   └── thread_pool.cpp
└── tests/
    ├── kv_store_test.cpp
    └── thread_pool_test.cpp
```

## Build on Windows

From the project directory:

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

If CMake was already configured:

```powershell
cmake --build build
```

## Run tests

```powershell
ctest --test-dir build --output-on-failure
```

## Start the server

Default port:

```powershell
.\build\novakv.exe
```

Custom port:

```powershell
.\build\novakv.exe 6379
```

The server binds to `0.0.0.0`, so it accepts connections arriving through the machine's network interfaces, not only localhost.

## Connect locally

```powershell
.\build\novakv_client.exe 127.0.0.1 6379
```

## Connect from another computer on the same LAN

On the server machine, run:

```powershell
ipconfig
```

Find the server's IPv4 address, for example:

```text
192.168.1.25
```

On the other computer:

```powershell
.\build\novakv_client.exe 192.168.1.25 6379
```

### Automatic reconnect

If the server is stopped while the client is running, the client keeps the same process alive. When you enter the next command, it detects the broken TCP connection and retries the connection every 2 seconds until the server is available again. The command is then retried after reconnection.

For example:

```text
> GET name
Connection lost. Waiting for NovaKV server...
Attempting to reconnect...
Reconnected to NovaKV
VALUE Sujal
```

The client still exits normally when you enter `EXIT`.

Windows Firewall must allow inbound TCP traffic on the selected port. For a controlled LAN test, create an inbound rule for the NovaKV executable or TCP port 6379.

## Commands

Commands are case-insensitive:

```text
SET key value
Set key value
sEt key value
```

Supported commands:

```text
SET key value
SET key value ttl
GET key
EXISTS key
REMOVE key
SIZE
TTL key
PING
EXIT
```

Examples:

```text
> SET name Sujal
OK

> get name
VALUE Sujal

> sEt session abc 10
OK

> ttl session
TTL 9

> EXISTS session
EXISTS 1

> REMOVE session
REMOVED 1

> PING
PONG

> SIZE
SIZE 0
```

TTL return semantics:

- `TTL key` returns remaining seconds for a temporary key.
- `TTL key` returns `-1` for a permanent key.
- `TTL key` returns `-2` when the key does not exist or has expired.

## Architecture

```text
                 TCP
+-------------+       +----------------------+
| NovaKV      | ----> | NovaKV Server        |
| Client      |       |                      |
+-------------+       |  ThreadPool          |
                      |      |               |
                      |      v               |
                      |  KeyValueStore       |
                      |      |               |
                      |      v               |
                      |  Persistence         |
                      +----------------------+
```

The server accepts TCP connections and submits each client connection to the thread pool. The key-value store serializes concurrent access with a mutex. Persistent disk writes are additionally serialized so concurrent client operations cannot race while writing the database file.

## Current scope

NovaKV is a learning/project implementation rather than a production database. The protocol is newline-delimited text and values currently use a single whitespace-delimited token in the `SET key value` command.
