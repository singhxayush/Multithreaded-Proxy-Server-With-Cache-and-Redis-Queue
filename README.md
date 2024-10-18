# Multi-Threaded Proxy-Server with LRU Cache and Redis-Queue

## Index

1. [Overview](#Project-Overview)
2. [Working](#working)
3. [Example](#Example)
4. [Setup and Run](#Setup-and-Run)
5. [Directory Structure](#directory-structure)

## Project Overview

1. This project implements a Multithreaded Proxy Server with Caching and Redis Queue. It designed to handle multiple HTTP **GET** requests simultaneously.
2. It features a multithreaded architecture to handle multiple incoming clients simultaneously and uses an LRU caching mechanism to minimize unnecessary requests to the remote server.
3. The project also integrates a Redis queue to manage requests during peak loads to keep things running smoothly.
4. Supports HTTP/1.x and HTTP/2.0

## Working

<!-- ![Project Diagram](images/diag.png) -->

![Project Diagram](images/diag.png)

## Example

![Project Diagram](images/flow.png)

## Setup and Run

#### Prerequisites:

- OS: macOS, Linux
- C compiler (GCC, Clang)
- lcurl (library in C)

#### Setup

**Clone the Repository**:

```bash
git clone https://github.com/singhxayush/Multithreaded-Proxy-Server-With-Cache-and-Redis-Queue.git
cd Multithreaded-Proxy-Server-With-Cache-and-Redis-Queue
```

#### Building the Project:

1. **Compile:** Open a terminal, navigate to the project directory, and run the following command:

   ```bash
   make
   ```

   This will compile the source code, link the object files, and create the proxy executable in the `bin` directory.

**Running the Proxy Server:**

2. **Default Port:** To run the proxy server on the default port (8080), execute:

   ```bash
   make run
   ```

3. **Custom Port:** To run the proxy server on a specific port (e.g., 7000), use:

   ```bash
   make run PORT=7000
   ```

**Cleaning Up:**

4. **Remove Compiled Files:** To delete all compiled files and the `build` directory, run:

   ```bash
   make clean
   ```

**Creating an Archive:**

5. **Create a Tarball:** To create a compressed archive of the project source code, Makefile, and README, execute:

   ```bash
   make tar
   ```

   This will generate a file named `ass1.tgz`.

**Important Notes:**

- The `make` command automatically determines the necessary steps based on the Makefile.
- You can customize the build process by modifying the Makefile variables (e.g., `PORT`, `CFLAGS`).
- Refer to the Makefile comments for detailed explanations of each target and variable.

Certainly, I'll add the running part in a similar Markdown syntax. Here's how it would look:

#### Running the Proxy Server:

1. **Default Port:** To run the proxy server on the default port (8080), execute:

   ```bash
   make run
   ```

2. **Custom Port:** To run the proxy server on a specific port (e.g., 7000), use:
   ```bash
   make run PORT=7000
   ```

#### Using the Proxy Server:

3. **Via Command Line:** You can use curl to send requests through the proxy:

   ```bash
   curl -X GET http://localhost:8080/https://www.cs.princeton.edu
   ```

4. **Via Web Browser:** Enter the following URL format in your browser:

   ```
   http://localhost:8080/https://www.cs.princeton.edu
   ```

   Replace `localhost:8080` with the appropriate host and port if you're not using the default, and replace the example URL with your desired destination.

## Directory Structure

    .
    ├── bin/
    ├── build/
    │
    ├── include/
    │   ├── http1.x_handler.h
    │   ├── redis_queue.h
    │   ╰── style.h
    │
    ├── src/
    │   ├── main.c
    │   ├── redis_queue.c
    │   ╰── server.c
    │
    ├── tests/
    │   ╰── test_thread_pool.c
    │
    ├── images/
    │
    ├── Todo.txt
    ├── Makefile
    ╰── README.md

### Description

- **bin/**: Directory for compiled binary files.
- **build/**: Directory for intermediate build files and object files.
- **include/**: Directory for header files.
  - **redis_queue.h**: Header file for Redis queue functions.
  - **server.h**: Header file for server-related functions.
  - **thread_pool.h**: Header file for thread pool functions.
- **src/**: Directory for source files.
  - **redis_queue.c**: Implementation of Redis queue functions.
  - **server.c**: Implementation of server functions.
  - **thread_pool.c**: Implementation of thread pool functions.
- **tests/**: Directory for test files.
  - **some_test.c**:
- **Makefile**: Makefile for building the project.
- **Todo.txt**: A list of features yet to implement.
