# Operating System Simulation Project

## Overview
This project is an Operating System simulation implemented in C, developed as part of an Operating Systems university course. The project demonstrates core concepts of operating systems including CPU scheduling, memory management, and process execution, mimicking a functional multi-core OS kernel. 

It provides an in-depth, hands-on implementation of how operating systems handle process execution, manage memory through advanced paging algorithms (including simulated 64-bit address spaces), and maintain robust process queues.

## Key Features
- **Memory Management (64-bit System & Paging)**: Implemented virtual memory management tailored for a 64-bit environment (`mm64.c`, `mm-vm.c`). It includes multi-level paging algorithms, handling page faults, virtual-to-physical address mappings, and page replacement mechanisms (e.g., swapping pages between RAM and SWAP).
- **CPU Scheduling**: Contains robust process schedulers, prominently a Multi-Level Queue (MLQ) scheduling algorithm (`sched.c`) to prioritize and handle process execution across multiple simulated CPUs. Handles OS process transition states (Ready, Running, Finished).
- **Queue Management**: Efficient implementations of process queues (Run Queue, Ready Queue, MLQ Queues) utilizing thread-safe operations (`queue.c`).
- **Concurrency & Synchronization**: Extensively uses multithreading via POSIX threads (`pthread`) to simulate concurrent execution on multiple CPUs. Proper synchronization mechanisms using mutex locks (`pthread_mutex_t`) are applied to prevent race conditions on shared kernel data structures.
- **Syscall Handling**: Simulation of various OS system calls.

## Technologies Used
- **Language**: C
- **Environment**: Ubuntu / Linux OS
- **Build System**: Make (`Makefile`)
- **Libraries**: Standard C libraries, POSIX `pthread` for concurrency

## Ubuntu / Linux Proficiency
This project serves as practical proof of proficiency with the Ubuntu OS and the Linux software development ecosystem:
- **Native Linux Development**: Developed and compiled entirely on a Linux environment using the GNU Compiler Collection (`gcc`).
- **Build Automation**: Uses a custom `Makefile` for automated dependency management, object linking, and build execution (`make all`, `make clean`).
- **Bash Scripting**: Incorporates bash shell scripting (`syscalltbl.sh`) to dynamically process system call lists and adjust source configurations.
- **POSIX API Usage**: Leverages POSIX thread APIs (`pthread`) native to Linux environments for managing multi-core concurrency and process synchronization.
- **System Tooling**: Strong demonstration of Linux terminal operations, debugging memory management in C, and understanding file I/O operations.

## Running the Simulation

1. Ensure you have GCC and Make installed on your Ubuntu/Linux system:
   ```bash
   sudo apt update
   sudo apt-install build-essential
   ```

2. Build the project using `make`:
   ```bash
   make all
   ```

3. Run the OS simulation with a specified input configuration file (from the `input/` directory):
   ```bash
   ./os os_1_mlq_paging
   ```
   *(Note: The main executable automatically appends the `input/` path to the provided argument).*

4. Clean the compiled objects and executables when done:
   ```bash
   make clean
   ```

## Repository Structure
- `src/`: Source code files for OS modules (CPU, memory, scheduler, loader, timer).
- `include/`: Header files defining system constants, memory structures, and function prototypes.
- `input/`: Configuration files and simulated process instructions for testing various OS scheduling and paging scenarios.
- `output/`: Expected outputs for the test cases, used to verify the correctness of the OS algorithms.

---
