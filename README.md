# distribute-file-server
## Features 
- Developed a working UDP-based distributed file server in C language. The On-Disk file system follows the design of the UNIX file system.
- Provides a client library to users including write/read/create/delete/lookup a file in the system, display file system information, shutdown the server. Users are allowed to link the library to their own code and programmatically operate the file system.
- The server achieves idempotency to provide fault tolerance when the server crashes. Optimized the filesystem for ~30% faster data access and lower latency by implementing LRU cache.
