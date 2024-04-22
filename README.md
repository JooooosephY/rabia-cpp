# Rabia-CPP
[![GitHub license which is BSD-2-Clause license](https://img.shields.io/github/license/resetius/miniraft-cpp)](https://github.com/resetius/miniraft-cpp)

This is an implementation of the Rabia consensus algorithm in C++20. Final project for NYU CS2621, spring 2024.
It is inspired by and basesd on [Miniraft-cpp](https://github.com/resetius/miniraft-cpp.git). Similarly, this implementation leverages the [coroio library](https://github.com/resetius/coroio) for efficient asynchronous I/O operations. 

The remainder of file follows directly from its basis, Miniraft-cpp.

## Components
- `raft.h` / `raft.cpp`: Implementation of the core Raft algorithm.
- `messages.h` / `messages.cpp`: Message definitions for node communication.
- `timesource.h`: Time-related functionalities for Raft algorithm timings.
- `server.h` / `server.cpp`: Server-side logic for handling client requests and node communication.
- `client.cpp`: Client-side implementation for cluster interaction.

## Getting Started

### Prerequisites
- C++20 compatible compiler
- CMake for building the project
- [Cmocka](https://cmocka.org/) for unit testing

### Building the Project
1. Clone the repository: 
   ```
   git clone https://github.com/resetius/miniraft-cpp
   ```
2. Initialize and update the submodule:
   ```
   git submodule init
   git submodule update
   ```
3. Navigate to the project directory: 
   ```
   cd miniraft-cpp
   ```
4. Build the project using CMake: 
   ```
   cmake .
   make
   ```

### Running the Application

This is a simple application designed to demonstrate log replication in the Raft consensus algorithm. 

To start the application, launch the servers with the following commands:
```
./server --id 1 --node 127.0.0.1:8001:1 --node 127.0.0.1:8002:2 --node 127.0.0.1:8003:3
./server --id 2 --node 127.0.0.1:8001:1 --node 127.0.0.1:8002:2 --node 127.0.0.1:8003:3
./server --id 3 --node 127.0.0.1:8001:1 --node 127.0.0.1:8002:2 --node 127.0.0.1:8003:3
```

To interact with the system, run the client as follows:
```
./client --node 127.0.0.1:8001:1
```
The client expects an input string to be added to the distributed log. If the input string starts with an underscore (`_`), it should be followed by a number (e.g., `_ 3`). In this case, the client will attempt to read the log entry at the specified number.


### Distributed Key-Value Store Example

Additionally, there's an example implementing a distributed key-value (KV) store. 

#### Starting KV Store Servers

To start the KV store servers, use:
```
./kv --server --id 1 --node 127.0.0.1:8001:1 --node 127.0.0.1:8002:2 --node 127.0.0.1:8003:3
./kv --server --id 2 --node 127.0.0.1:8001:1 --node 127.0.0.1:8002:2 --node 127.0.0.1:8003:3
./kv --server --id 3 --node 127.0.0.1:8001:1 --node 127.0.0.1:8002:2 --node 127.0.0.1:8003:3
```

#### Running the KV Client

To run the KV client, use:
```
./kv --client --node 127.0.0.1:8001:1
```
The KV client expects commands as input:
1. `set <key> <value>` - Adds or updates a value in the KV store.
2. `get <key>` - Retrieves a value by its key.
3. `list` - Displays all key/value pairs in the store.
4. `del <key>` - Deletes a key from the store.
