# Distributed File System

This project entails the development of a UDP-based distributed file server.

## Features

### Basic File Server
- Operates as a stand-alone UDP-based server, listening for messages and processing them as required.
- Stores data in an on-disk, fixed-sized file, known as the file system image.
- Utilizes system calls like `open()`, `read()`, `write()`, `lseek()`, `close()`, and `fsync()` to access the file system image.

### On-Disk File System
- Utilizes on-disk structures including a super block, inode bitmap, data bitmap, inode table, and data region.
- Refer to the provided `ufs.h` header for detailed specifications about the on-disk structures.

### Client Library
- Provides interfaces for file system access through the `libmfs.so` library.
- Enables functionalities such as file lookup, creation, deletion, read, write, and file system shutdown.

### Server Idempotency
- Ensures idempotency by committing all modified buffers to disk using `fsync()` after any file system state change.

## Program Specifications

### Server Invocation
- The server is invoked using the command format: `prompt> server [portnum] [file-system-image]`.
- Where:
  - `portnum`: the server's listening port number.
  - `file-system-image`: the file containing the file system image.

### Client Library
- Implements the interface specified in `mfs.h` and includes timeout handling for retries after timeouts.

### Usage
Building and Running
To compile the server:

```bash
make server
```

To run the server:

```bash
./server [portnum] [file-system-image]
```

### Client Usage
The client library must be linked with programs that interact with the file server. Usage of various file system functions is described in the provided mfs.h header.
