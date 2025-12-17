#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>

using namespace std;

int main() {
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

    listen(server_socket, 5);
    cout << "Server listening on port " << server_port << "...\n";

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
        cout << "Connected: " << client_ip << endl;

        bool allowed = false;
        for (auto& ip : allowed_ips)
            if (ip == client_ip) allowed = true;

        if (!allowed) {
            cout << "IP not allowed, closing.\n";
            close(client_socket);
            continue;
        }

        char info_buf[1024] = {0};
        recv(client_socket, info_buf, sizeof(info_buf), 0);

        string info(info_buf);

        if (info.rfind("UPLOAD", 0) == 0) {
            size_t p1 = info.find(',');
            size_t p2 = info.find(',', p1 + 1);

            string filename = info.substr(p1 + 1, p2 - p1 - 1);
            long file_size = stol(info.substr(p2 + 1));

            cout << "Uploading: " << filename << " (" << file_size << " bytes)\n";

            string ready = "OK";
            send(client_socket, ready.c_str(), ready.size(), 0);

            ofstream outfile("received_" + filename, ios::binary);
            long received = 0;
            char buf[1024];

            while (received < file_size) {
                int bytes = recv(client_socket, buf, sizeof(buf), 0);
                if (bytes <= 0) break;

                outfile.write(buf, bytes);
                received += bytes;

                string ack = "ACK";
                send(client_socket, ack.c_str(), ack.size(), 0);
            }

            outfile.close();
            cout << "Upload finished.\n";
        }

        else if (info.rfind("DOWNLOAD", 0) == 0) {
            size_t p = info.find(',');
            string filename = info.substr(p + 1);

            struct stat st;
            if (stat(filename.c_str(), &st) != 0) {
                string no = "NOFILE";
                send(client_socket, no.c_str(), no.size(), 0);
                close(client_socket);
                continue;
            }

            long file_size = st.st_size;
            string header = "OK," + to_string(file_size);
            send(client_socket, header.c_str(), header.size(), 0);

            ifstream file(filename, ios::binary);
            char buffer[1024];

            while (!file.eof()) {
                file.read(buffer, sizeof(buffer));
                int n = file.gcount();
                if (n > 0) send(client_socket, buffer, n, 0);
            }

            file.close();
            cout << "Download sent: " << filename << endl;
        }

        close(client_socket);
    }

    close(server_socket);
    return 0;
}
