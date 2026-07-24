# Windows Setup Guide
This guide will walk you through compiling and running the Secure C++ Chat Room on a Windows environment. Because Windows does not come with C++ compilers or OpenSSL by default, we highly recommend using **MSYS2** to install the necessary MinGW-w64 toolchains.
## Prerequisites
1. Download and install [MSYS2](https://www.msys2.org/).
2. Open the **MSYS2 UCRT64** or **MSYS2 MINGW64** terminal from your Windows Start Menu.
### Installing Dependencies
Inside the MSYS2 terminal, run the following command to install the C++ compiler (`g++`), the `make` utility, and the OpenSSL libraries:
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-openssl
```
### Automated Helper Script
You can double-click the `setup.cmd` file in Windows Explorer for a quick reminder of the commands and to automatically generate your SSL certificates!
### Manual Certificate Generation
The server requires a certificate and private key to encrypt traffic. In your terminal (inside the project folder), run:
```bash
openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes -subj "/C=US/ST=State/L=City/O=Antigravity/CN=localhost"
```
## Compiling
Once the dependencies are installed and your certificates are generated, open your terminal in the project folder and use the provided `Makefile` to compile the Windows executables:
```bash
mingw32-make winserver winclient
```
*(Note: Depending on your MSYS2 environment, the make command might just be `make`)*
This will output two executable files: `winserver.exe` and `winclient.exe`.
## Running the Application
Double-click the executables in Windows Explorer, or run them from your terminal!
**To host a chat room:**
```bash
./winserver.exe
```
You will be prompted to enter a custom name for your chat room and a secure password.
**To join a chat room:**
```bash
./winclient.exe
```
The client will automatically scan your local network for active servers and display a list for you to choose from!
