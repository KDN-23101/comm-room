#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

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
        cerr << "Unable to create SSL context" << endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

DWORD WINAPI receive_messages(LPVOID lpParam) {
    SSL *ssl = (SSL*)lpParam;
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
    return 0;
}

string discover_servers() {
    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket == INVALID_SOCKET) return "";

    char broadcast = 1;
    setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    DWORD timeout = 2000;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

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
    int server_len = sizeof(server_addr);

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recvfrom(udp_socket, buffer, sizeof(buffer)-1, 0, (struct sockaddr*)&server_addr, &server_len);
        if (bytes <= 0) break; // timeout reached

        string reply(buffer);
        if (reply.rfind("CHAT_SERVER_HERE:", 0) == 0) {
            string room_name = reply.substr(17);
            string ip(inet_ntoa(server_addr.sin_addr));
            
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
    closesocket(udp_socket);

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
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        return 1;
    }

    init_openssl();
    SSL_CTX *ctx = create_context();

    string server_ip = discover_servers();

    string username;
    cout << "Enter your username: ";
    getline(cin, username);

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        cerr << "Error creating socket." << endl;
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        cerr << "Invalid address." << endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Connection to server failed." << endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_socket);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        closesocket(client_socket);
        WSACleanup();
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
            closesocket(client_socket);
            WSACleanup();
            return 1;
        }
        cout << "Authentication successful!" << endl;
        
        // Send requested username
        SSL_write(ssl, username.c_str(), username.length());
    }

    cout << "You are now in the secure chat room!\n" << endl;

    CreateThread(NULL, 0, receive_messages, (LPVOID)ssl, 0, NULL);

    string message;
    while (true) {
        getline(cin, message);
        if (message == "exit" || message == "quit") {
            break;
        }

        if (SSL_write(ssl, message.c_str(), message.length()) <= 0) {
            cerr << "Send failed." << endl;
            break;
        }
    }

    SSL_free(ssl);
    closesocket(client_socket);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    WSACleanup();
    return 0;
}
