#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <random>
#include <iomanip>
#include <sstream>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8080
#define UDP_PORT 8081
#define MAX_LEN 2048

using namespace std;

char banner[] = {
"███████╗███████╗██████╗ ██╗   ██╗███████╗██████╗ \n"
"██╔════╝██╔════╝██╔══██╗██║   ██║██╔════╝██╔══██╗\n"
"███████╗█████╗  ██████╔╝██║   ██║█████╗  ██████╔╝\n"
"╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██╔══╝  ██╔══██╗\n"
"███████║███████╗██║  ██║ ╚████╔╝ ███████╗██║  ██║\n"
"╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝\n"
"                                                 \n"
};

vector<SSL*> clients;
mutex clients_mtx;
string g_room_name;
string g_room_password;

string generate_unique_id() {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 9999);
    
    stringstream ss;
    ss << setw(4) << setfill('0') << dis(gen);
    return ss.str();
}

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX *create_context() {
    const SSL_METHOD *method = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void broadcast_message(const string& message, SSL* sender_ssl) {
    lock_guard<mutex> lock(clients_mtx);
    for (SSL* client_ssl : clients) {
        if (client_ssl != sender_ssl) {
            SSL_write(client_ssl, message.c_str(), message.length());
        }
    }
}

void handle_client(SSL* ssl) {
    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        int fd = SSL_get_fd(ssl);
        SSL_free(ssl);
        close(fd);
        return;
    }

    // Authentication
    string auth_req = "AUTH_REQUEST";
    SSL_write(ssl, auth_req.c_str(), auth_req.length());

    char auth_buf[256];
    memset(auth_buf, 0, sizeof(auth_buf));
    int auth_bytes = SSL_read(ssl, auth_buf, sizeof(auth_buf) - 1);
    
    if (auth_bytes <= 0 || string(auth_buf) != g_room_password) {
        string reject_msg = "AUTH_FAILED";
        SSL_write(ssl, reject_msg.c_str(), reject_msg.length());
        int fd = SSL_get_fd(ssl);
        SSL_free(ssl);
        close(fd);
        cout << "Client rejected (incorrect password)." << endl;
        return;
    }

    string accept_msg = "AUTH_SUCCESS";
    SSL_write(ssl, accept_msg.c_str(), accept_msg.length());

    // Receive requested username
    memset(auth_buf, 0, sizeof(auth_buf));
    int user_bytes = SSL_read(ssl, auth_buf, sizeof(auth_buf) - 1);
    if (user_bytes <= 0) {
        int fd = SSL_get_fd(ssl);
        SSL_free(ssl);
        close(fd);
        return;
    }
    
    string base_username(auth_buf);
    string full_identity = base_username + "#" + generate_unique_id();

    // Get IP address
    int fd = SSL_get_fd(ssl);
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(fd, (struct sockaddr *)&addr, &addr_size);
    char clientip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, clientip, INET_ADDRSTRLEN);
    string ip_str(clientip);

    // Add to active clients
    {
        lock_guard<mutex> lock(clients_mtx);
        clients.push_back(ssl);
    }
    
    cout << "Client joined from " << ip_str << " as " << full_identity << endl;

    char buffer[MAX_LEN];
    auto last_msg_time = chrono::steady_clock::now();
    int burst_count = 0;

    while (true) {
        memset(buffer, 0, MAX_LEN);
        int bytes_received = SSL_read(ssl, buffer, MAX_LEN);

        if (bytes_received <= 0) {
            cout << "Client " << full_identity << " (" << ip_str << ") disconnected." << endl;
            break;
        }

        // Rate limiting logic
        auto now = chrono::steady_clock::now();
        auto diff = chrono::duration_cast<chrono::milliseconds>(now - last_msg_time).count();
        if (diff < 500) {
            burst_count++;
        } else {
            burst_count = 0;
        }
        last_msg_time = now;

        if (burst_count > 3) {
            cout << "Client " << full_identity << " (" << ip_str << ") kicked for spamming." << endl;
            string kick_msg = "[SERVER]: You have been kicked for spamming.";
            SSL_write(ssl, kick_msg.c_str(), kick_msg.length());
            break;
        }

        string raw_message(buffer);
        string formatted_message = "[" + full_identity + "]: " + raw_message;
        cout << "Broadcasting: " << formatted_message << endl;
        broadcast_message(formatted_message, ssl);
    }

    lock_guard<mutex> lock(clients_mtx);
    clients.erase(remove(clients.begin(), clients.end(), ssl), clients.end());
    // fd was fetched above during setup
    SSL_free(ssl);
    close(fd);
}

void discovery_listener(string room_name) {
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) return;

    int opt = 1;
    setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(UDP_PORT);

    if (bind(udp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(udp_socket);
        return;
    }

    char buffer[256];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0,
                                      (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_received > 0) {
            string msg(buffer);
            if (msg == "DISCOVER_CHAT_SERVER") {
                string reply = "CHAT_SERVER_HERE:" + room_name;
                sendto(udp_socket, reply.c_str(), reply.length(), 0,
                       (struct sockaddr*)&client_addr, client_len);
            }
        }
    }
}

int main() {
    cout << banner;
    cout << "Enter a name for your chat room: ";
    getline(cin, g_room_name);
    
    cout << "Enter a secure password for the room: ";
    getline(cin, g_room_password);

    init_openssl();
    SSL_CTX *ctx = create_context();
    configure_context(ctx);

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "Error creating socket." << endl;
        return 1;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error binding to port " << PORT << "." << endl;
        return 1;
    }

    if (listen(server_socket, 10) < 0) {
        cerr << "Error listening on socket." << endl;
        return 1;
    }

    cout << "Secure Server listening on port " << PORT << "..." << endl;

    thread discovery_thread(discovery_listener, g_room_name);
    discovery_thread.detach();

    while (true) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) continue;

        cout << "New raw TCP connection accepted. Initiating TLS handshake..." << endl;
        
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_socket);

        thread t(handle_client, ssl);
        t.detach();
    }

    close(server_socket);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    return 0;
}
