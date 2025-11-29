# C Project – GarageDucati: Motorcycle Configuration System

This repository contains the implementation of **GarageDucati**, a client-server program in **C** for configuring Ducati motorcycles and managing user interactions.  
The project was developed as part of the **Operating Systems** course at the University of Enna “Kore”.

---

## Repository Structure

```
main_repository/
│
├── client.c
├── LICENSE
├── README.md
└── server.c
```

---

## Project Objective

GarageDucati allows users to:

1. **Create a Ducati motorcycle configuration**  
   - Choose model, color, wheel diameter, seat, exhaust, clutch, frame, tires, footpegs, and stand  
   - Each option has 5 or 10 possible choices  
   - Save configuration to a text file with timestamp, serial code, total price, and usage instructions  

2. **Load a Ducati configuration**  
   - Input serial code to retrieve previously saved configuration  
   - Display selected options and total price  

3. **Contact a GarageDucati operator via chat**  
   - Connect with one of 20 randomly assigned operators (10 men, 10 women)  
   - Close the chat with the keyword “Chiudi”  

4. **Immediate program exit**  

The project implements **inter-process communication (IPC)**, signals, shared memory, threads, semaphores, and mutexes.

---

## System Calls & IPC

The application makes extensive use of system calls and synchronization mechanisms:

### Processes
- `getpid()`, `getppid()`, `fork()`, `exit()`, `wait()`, `waitpid()`, `execl()`

### Signals
- `raise()`, `kill()`, `signal()`, `alarm()`  
- Signals used include: `SIGUSR1`, `SIGUSR2`, `SIGQUIT`, `SIGALRM`, `SIGINT`

### Pipes & FIFOs
- `pipe()`, `write()`, `read()`

### Shared Memory
- `shmget()`, `shmat()`, `shmdt()`, `ftok()`

### Message Queues
- `msgget()`, `msgsnd()`, `msgrcv()`, `msgctl()`

### Threads
- `pthread_create()`, `pthread_exit()`, `pthread_join()`, `pthread_mutex_lock()`, `pthread_mutex_unlock()`, `pthread_mutex_destroy()`, `pthread_cond_wait()`, `pthread_cond_signal()`, `pthread_cond_init()`, `pthread_cond_destroy()`, `pthread_sigmask()`, `pthread_kill()`

### Semaphores
- **System-V:** `semget()`, `semop()`, `semctl()`  
- **POSIX named:** `sem_open()`, `sem_wait()`, `sem_post()`, `sem_close()`, `sem_unlink()`

---

## Features Overview

- **Client**
  - Menu management via threads
  - Configuration creation and saving
  - Loading configuration using serial code
  - Chat with operators for assistance
  - Signal handlers for connection, menu, exit, and semaphore events

- **Server**
  - Handles client requests and manages configurations
  - Manages shared memory and message queues
  - Synchronizes threads and semaphores
  - Sends configuration data back to clients

---

## Execution

### Compile

From the root folder:
```bash
gcc -o client client.c -lpthread
gcc -o server server.c -lpthread
```

1. Start the server:
```bash
./server
```

2. Start the client in a separate terminal:
```bash
./client
```

3. Follow on-screen prompts to configure a motorcycle, load a configuration, contact an operator, or exit.

---

## Notes

- Configurations are stored in a folder named **“Ducati GarageMoto”**
- The client and server communicate via **shared memory, message queues, and signals**
- Threads and semaphores ensure **safe concurrent access** and menu management
- Each operation is logged with serial codes and timestamps
