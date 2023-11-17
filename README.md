# LibNetSync_Distributed_Library_Management_System

## Personal Details

- Full Name: Yaxing Li
- Student ID: 123456789

## Project Overview

This project implements a library server system with a main server (`serverM`), three backend servers (`serverS`, `serverL`, `serverH`), and a client. It covers the startup sequence, login and authentication, request forwarding, inventory checks, and replies to the client. An extra credit part for inventory management is also included.

## Code Files

- `serverM.cpp` and `serverM.h`: Main server implementation handling TCP and UDP connections, authentication, and request routing.
- `serverS.cpp` and `serverS.h`: Backend server for science books. Handles inventory management, book status updates, and responds to requests from the main server.
- `serverL.cpp` and `serverL.h`: Backend server for literature books with functionalities similar to `serverS`.
- `serverH.cpp` and `serverH.h`: Backend server for history books with functionalities similar to `serverS`.
- `client.cpp` and `client.h`: Client implementation that interacts with the user for login, book requests, and displays responses.

## Message Format

- Usernames and passwords are encrypted with an offset of 5 and case-sensitive.
- Book codes are prefixed with a letter indicating the server ('S', 'L', 'H').
- Responses from servers regarding book availability follow a predefined format.

## Idiosyncrasies

- The system assumes reliable network connections.
- The backend servers read their respective `.txt` files once at startup.
- If a client requests a book multiple times, the inventory updates correctly without re-reading the database file.

## Reused Code

No code from external sources was used in this project. All implementations were created based on the assignment specifications and guided by the Beej’s Guide to Network Programming.

## Phase Descriptions

### Phase 1: Boot-up

- Servers start in a specific order and read their respective `.txt` files.
- Backend servers send their book statuses to the main server via UDP.

### Phase 2: Login and confirmation

- Client encrypts and sends login credentials to the main server via TCP.
- Main server checks encrypted credentials against its database.

### Phase 3: Forwarding request to Backend Servers

- Client sends book code to the main server.
- Main server determines the appropriate backend server based on the book code prefix and forwards the request via UDP.

### Phase 4: Reply

- Backend servers check the inventory and respond to the main server.
- Main server forwards the response to the client via TCP.

### Extra Credit: Inventory Management

- Implemented inventory management accessible by library staff with credentials (Username: Admin, Password: Admin).

