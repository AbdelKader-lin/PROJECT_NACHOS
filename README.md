# NachOS Operating System Project

Educational operating-systems project based on **NachOS (Not Another Completely Heuristic Operating System)**, completed as part of the Operating Systems coursework at Grenoble INP - Ensimag.

This project extends the NachOS codebase with kernel and user-space functionality covering **system calls, user threads, process management, memory management, synchronization, filesystem operations, and networking**.

## What this project demonstrates

This repository showcases low-level systems programming in C/C++, including kernel/user interactions and operating-system mechanisms.

Key areas covered include:

- **System calls** and transitions between user programs and the NachOS kernel
- **User-level threads**: creation, exit, join, and synchronization
- **Process management**: `fork`, `exec`, `join`, and process exit
- **Dynamic memory management** through `sbrk`
- **Synchronized console I/O** for characters, strings, and integers
- **Semaphores and synchronization primitives**
- **Filesystem operations** and directory handling
- **Networking tests** and file transfer between NachOS instances

## Repository structure

```text
.
├── code/
│   ├── filesys/      # Filesystem implementation and related logic
│   ├── machine/      # Simulated hardware support provided by NachOS
│   ├── network/      # NachOS networking support
│   ├── test/         # User-space and kernel test programs
│   ├── threads/      # Threading and synchronization code
│   ├── userprog/     # User programs, syscalls, address spaces, processes
│   └── README.md     # Detailed test commands
├── docker/           # Docker environment for building/running the project
├── COPYRIGHT
└── README.md
```

## Implemented and tested functionality

The project includes tests for synchronized character, integer and string I/O; user-thread creation and join; address-space and page management; memory-allocation behavior; producer/consumer synchronization; filesystem and directory operations; `fork`/`exec`; a simple shell; and networking/file transfer between two NachOS instances.

## Running the tests

Detailed commands are documented in [`code/README.md`](code/README.md).

Typical workflow:

```bash
# Format the NachOS filesystem
./build/nachos-final -f

# Copy a compiled user program into the NachOS filesystem
./build/nachos-final -cp ./build/halt halt

# Execute it
./build/nachos-final -x halt
```

## Technologies

- C / C++
- NachOS
- Linux
- Make
- Docker
- Operating-system concepts: processes, threads, synchronization, virtual memory, filesystems, networking

## Academic context

This is an educational operating-systems project. The repository contains the upstream NachOS codebase together with extensions completed during the course. The purpose of this repository is to showcase the systems-programming work carried out on top of NachOS rather than to present NachOS itself as original work.
