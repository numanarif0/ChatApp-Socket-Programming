# ChatApp‑Socket‑Programming

A socket-based chat application implemented in **C++** using a client–server architecture. Connect multiple clients to a server and exchange messages in real time.

---

## Table of Contents
- [About](#about)  
- [Features](#features)  
- [Getting Started](#getting-started)  
  - [Prerequisites](#prerequisites)  
  - [Installation](#installation)  
- [How to Contribute](#how-to-contribute)

---

## About

**ChatApp‑Socket‑Programming** is a practical C++ project showcasing a simple but effective chat system. The server handles multiple client connections, while clients can send and receive messages through sockets. This project demonstrates fundamental socket programming, concurrency via multithreading, and client-server communication.

---

## Features
- Multi-client chat capability  
- Server implemented using C++ sockets and `select()` or threading  
- Simple command-line interface for both server and client  
- Makefile included for easy compilation  

---

## Getting Started

### Prerequisites
- **C++ Compiler** (e.g., `g++`)  
- **Make** utility  


### Installation & Execution
1. Clone the repository:  
   ```bash
   git clone https://github.com/numanarif0/ChatApp-Socket-Programming.git
   ```
2. Navigate to project directory:  
   ```bash
   cd ChatApp-Socket-Programming
   ```
3. Build with Make:  
   ```bash
   make
   ```
4. Start the server (default port, e.g., 12345):  
   ```bash
   ./server
   ```
5. In separate terminal windows, start clients:  
   ```bash
   ./client
   ```
6. Chat away! Messages from one client will be broadcast to all connected clients.



