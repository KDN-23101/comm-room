# comm-room
Terminal Chat Room

# Secure Terminal Chat Room
A lightweight, robust, and highly secure terminal-based chat room application written entirely in C++ from scratch. It features cross-platform support, local network server discovery, and enterprise-grade OpenSSL encryption.
## Features
- **Local Network Discovery:** Servers broadcast their presence over UDP. Clients can automatically scan the local network and discover active chat rooms without needing to manually type IP addresses.
- **TLS/SSL Encryption:** All TCP chat traffic is encrypted using OpenSSL, preventing eavesdropping and packet sniffing.
- **Room Passwords:** Servers require a secure password to join, verified via a cryptographic handshake upon connection.
- **Secure Unique Identities:** To prevent username spoofing, the server acts as the absolute authority on identity. It generates a Discord-style unique 4-digit code (e.g., `kemmy#8291`) for every user when they join, and securely prepends it to all their messages.
- **DoS Protection & Rate Limiting:** The server actively monitors client message frequency using high-resolution timers. Clients that attempt to spam the room are automatically kicked.
- **Cross-Platform:** Includes native socket implementations for both Linux (POSIX) and Windows (Winsock2).
## Working Principle
### 1. Networking Architecture
The application uses a hybrid networking model:
- **UDP (User Datagram Protocol):** Used exclusively for server discovery. The server listens on port `8081`. When a client boots up, it sends a broadcast packet (`255.255.255.255`). Any active servers on the local network reply with their Custom Room Name and IP address.
- **TCP (Transmission Control Protocol):** Used for the actual chat room communication on port `8080`. TCP ensures that messages are delivered reliably and in the correct order.
### 2. Security Handshake
1. **Connection:** Client establishes a raw TCP connection.
2. **TLS Wrap:** Both client and server immediately wrap the socket using `SSL_connect` and `SSL_accept`.
3. **Password Auth:** Server sends `AUTH_REQUEST`. Client responds with the user's input password. Server validates it and sends `AUTH_SUCCESS` or drops the connection.
4. **Identity Binding:** Client sends their requested username. Server generates a `#1234` tag, binds it to the connection, and logs the IP.
## Setup Instructions
Depending on your operating system, follow the specific setup guide below:
- [Linux Setup Guide](LINUX_SETUP.md)
- [Windows Setup Guide](WINDOWS_SETUP.md)
