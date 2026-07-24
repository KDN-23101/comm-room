@echo off
echo ========================================================
echo  Setting up Secure C++ Chat Room (Windows)
echo ========================================================
echo.
echo Make sure you have MSYS2 or MinGW installed!
echo.
echo 1. If you are using MSYS2, please run this script inside the MSYS2 UCRT64 or MINGW64 terminal.
echo To install the required compilers and OpenSSL libraries, run this command:
echo pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-openssl
echo.
echo 2. Generating self-signed SSL certificates for the server...
openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes -subj "/C=US/ST=State/L=City/O=Antigravity/CN=localhost"
echo.
echo If openssl is not recognized, please make sure your MinGW/MSYS2 bin folder is in your PATH environment variable.
echo.
echo ========================================================
echo  Setup Complete!
echo ========================================================
echo You can now compile the Windows project by running:
echo mingw32-make server_win client_win
echo (or just run the 'g++' commands manually if make is not installed)
echo.
pause
