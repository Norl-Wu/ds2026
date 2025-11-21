#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int main() {
    const char* server_ip = "0.0.0.0";
    int server_port = 21003;

    vector<string> allowed_ips = {
        "127.0.0.1",
        "192.168.48.140",
        "172.30.176.1"
    };

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "Socket creation failed\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Bind failed\n";
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, 5) < 0) {
        cerr << "Listen failed\n";
        close(server_socket);
        return 1;
    }

    cout << "Server is waiting for connection at 0.0.0.0:" << server_port << "...\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            cerr << "Accept failed\n";
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        cout << "Connected to " << client_ip << endl;

        bool allowed = false;
        for (auto& ip : allowed_ips) {
            if (ip == client_ip) { 
                allowed = true; 
                break;
            }
        }

        if (!allowed) {
            cout << "Connection from " << client_ip << " is not allowed.\n";
            close(client_socket);
            continue;
        }

        char info_buf[1024] = {0};
        recv(client_socket, info_buf, sizeof(info_buf), 0);

        string info(info_buf);
        size_t comma_pos = info.find(',');
        string filename = info.substr(0, comma_pos);
        long file_size = stol(info.substr(comma_pos + 1));

        cout << "Receiving file: " << filename << " (Size: " << file_size << " bytes)\n";

        string ready = "Ready to receive file";
        send(client_socket, ready.c_str(), ready.size(), 0);

        string out_name = "received_" + filename;
        ofstream outfile(out_name, ios::binary);
        if (!outfile.is_open()) {
            cerr << "Cannot open file to write\n";
            close(client_socket);
            continue;
        }

        long bytes_received = 0;
        char buffer[1024];

        while (bytes_received < file_size) {
            int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes <= 0) break;

            outfile.write(buffer, bytes);
            bytes_received += bytes;

            string ack = "ACK";
            send(client_socket, ack.c_str(), ack.size(), 0);
        }

        outfile.close();

        cout << "File '" << filename << "' received successfully and saved.\n";
        close(client_socket);
    }

    close(server_socket);
    return 0;
}
