# Linux Setup Guide
This guide will walk you through compiling and running the Secure C++ Chat Room on a Linux environment (Ubuntu/Debian).
## Prerequisites
To compile the C++ source code with TLS encryption, you need the GNU compiler (`g++`), the `make` utility, and the OpenSSL development headers.
### Automated Setup
We have provided an automated setup script. Simply open your terminal in the project directory and run:
```bash
chmod +x setup.sh
./setup.sh
```
*Note: You may be prompted for your sudo password to install the required packages.*
### Manual Setup
If you prefer to install things manually:
1. **Install Dependencies:**
   ```bash
   sudo apt-get update
   sudo apt-get install -y g++ make libssl-dev
   ```
2. **Generate SSL Certificates:**
   The server requires a certificate and private key to encrypt the traffic. Generate them in the project folder:
   ```bash
   openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes -subj "/C=US/ST=State/L=City/O=Antigravity/CN=localhost"
   ```
## Compiling
Once the dependencies are installed and the certificates are generated, you can compile the code using the provided `Makefile`:
```bash
make server client
```
This will output two executable files: `server` and `client`.
## Running the Application
**To host a chat room:**
```bash
./server
```
You will be prompted to enter a custom name for your chat room and a secure password.

**To join a chat room:**
```bash
./client
```
The client will automatically scan your local network for active servers and display a list for you to choose from!
