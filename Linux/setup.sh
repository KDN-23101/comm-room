#!/bin/bash
echo "========================================================"
echo " Setting up Secure C++ Chat Room (Linux)"
echo "========================================================"
echo ""
echo "1. Installing required packages (g++, make, libssl-dev)..."
sudo apt-get update
sudo apt-get install -y g++ make libssl-dev

echo ""
echo "2. Generating self-signed SSL certificates..."
openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes -subj "/C=US/ST=State/L=City/O=Antigravity/CN=localhost" 2>/dev/null

echo ""
echo "========================================================"
echo " Setup Complete!"
echo "========================================================"
echo "You can now compile the project by running:"
echo "make server client"
echo ""
