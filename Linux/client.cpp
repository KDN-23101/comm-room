#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8080
#define UDP_PORT 8081
#define MAX_LEN 2048

using namespace std;

char banner[] = {
    " ██████╗ ██████╗ ███╗   ███╗███╗   ███╗    ██████╗  ██████╗  ██████╗ ███╗   ███╗\n"
    "██╔════╝██╔═══██╗████╗ ████║████╗ ████║    ██╔══██╗██╔═══██╗██╔═══██╗████╗ ████║\n"
    "██║     ██║   ██║██╔████╔██║██╔████╔██║    ██████╔╝██║   ██║██║   ██║██╔████╔██║\n"
    "██║     ██║   ██║██║╚██╔╝██║██║╚██╔╝██║    ██╔══██╗██║   ██║██║   ██║██║╚██╔╝██║\n"
    "╚██████╗╚██████╔╝██║ ╚═╝ ██║██║ ╚═╝ ██║    ██║  ██║╚██████╔╝╚██████╔╝██║ ╚═╝ ██║\n"
    " ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝     ╚═╝    ╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚═╝     ╚═╝\n"
    "                                                                                \n"
};

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX *create_context() {
    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void receive_messages(SSL* ssl) {
    char buffer[MAX_LEN];
    while (true) {
        memset(buffer, 0, MAX_LEN);
        int bytes_received = SSL_read(ssl, buffer, MAX_LEN);
        if (bytes_received <= 0) {
            cout << "\nDisconnected from server." << endl;
            exit(0);
        }
        cout << buffer << endl;
    }
}

string discover_servers() {
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) return "";

    int opt = 1;
    setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in broadcast_addr;
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(UDP_PORT);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

    string msg = "DISCOVER_CHAT_SERVER";
    sendto(udp_socket, msg.c_str(), msg.length(), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

    cout << "Searching for servers on local network..." << endl;

    vector<pair<string, string>> discovered_servers;
    char buffer[256];
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recvfrom(udp_socket, buffer, sizeof(buffer)-1, 0, (struct sockaddr*)&server_addr, &server_len);
        if (bytes <= 0) break; // timeout reached

        string reply(buffer);
        if (reply.rfind("CHAT_SERVER_HERE:", 0) == 0) {
            string room_name = reply.substr(17);
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(server_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
            string ip(ip_str);
            
            bool found = false;
            for (const auto& server : discovered_servers) {
                if (server.first == ip) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                discovered_servers.push_back({ip, room_name});
            }
        }
    }
    close(udp_socket);

    if (discovered_servers.empty()) {
        cout << "No servers found." << endl;
        string ip;
        cout << "Enter server IP address manually: ";
        getline(cin, ip);
        return ip;
    }

    cout << "\nDiscovered Secure Servers:\n";
    for (size_t i = 0; i < discovered_servers.size(); ++i) {
        cout << "[" << i + 1 << "] " << discovered_servers[i].second << " (" << discovered_servers[i].first << ")" << endl;
    }
    cout << "[0] Enter IP manually\n";
    
    while (true) {
        cout << "\nSelect a server (0-" << discovered_servers.size() << "): ";
        string choice_str;
        getline(cin, choice_str);
        if (choice_str.empty()) continue;
        
        try {
            int choice = stoi(choice_str);
            if (choice == 0) {
                string ip;
                cout << "Enter server IP address manually: ";
                getline(cin, ip);
                return ip;
            } else if (choice > 0 && choice <= (int)discovered_servers.size()) {
                return discovered_servers[choice - 1].first;
            }
        } catch (...) {}
    }
}

int main() {
    cout << banner;
    init_openssl();
    SSL_CTX *ctx = create_context();

    int client_socket;
    struct sockaddr_in server_addr;

    string server_ip = discover_servers();

    string username;
    cout << "Enter your username: ";
    getline(cin, username);

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        cerr << "Error creating socket." << endl;
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        cerr << "Invalid address." << endl;
        return 1;
    }

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Connection to server failed." << endl;
        return 1;
    }
    
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_socket);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    cout << "TLS Connection established securely." << endl;

    // Authentication handshake
    char auth_buf[256];
    memset(auth_buf, 0, sizeof(auth_buf));
    int auth_bytes = SSL_read(ssl, auth_buf, sizeof(auth_buf) - 1);
    
    if (auth_bytes > 0 && string(auth_buf) == "AUTH_REQUEST") {
        string password;
        cout << "Enter the room password: ";
        getline(cin, password);
        SSL_write(ssl, password.c_str(), password.length());

        memset(auth_buf, 0, sizeof(auth_buf));
        SSL_read(ssl, auth_buf, sizeof(auth_buf) - 1);
        if (string(auth_buf) != "AUTH_SUCCESS") {
            cout << "Authentication failed. Disconnecting." << endl;
            SSL_free(ssl);
            close(client_socket);
            return 1;
        }
        cout << "Authentication successful!" << endl;
        
        // Send requested username
        SSL_write(ssl, username.c_str(), username.length());
    }

    cout << "You are now in the chat room!\n" << endl;

    thread t(receive_messages, ssl);
    t.detach();

    string message;
    while (true) {
        getline(cin, message);
        if (message == "exit" || message == "quit") {
            break;
        }

        SSL_write(ssl, message.c_str(), message.length());
    }

    SSL_free(ssl);
    close(client_socket);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    return 0;
}
