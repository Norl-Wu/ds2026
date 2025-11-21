#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <iostream>
#include <fstream>

using namespace std;

int main() {
    const char* server_ip = "127.0.0.1";
    int server_port = 21003;
    string filename = "test.txt";

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket creation failed\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Connection failed\n";
        close(sock);
        return 1;
    }

    struct stat st;
    if (stat(filename.c_str(), &st) != 0) {
        cerr << "Unable to stat file\n";
        close(sock);
        return 1;
    }
    long file_size = st.st_size;

    string header = filename + "," + to_string(file_size);
    send(sock, header.c_str(), header.size(), 0);

    char ack_buf[1024] = {0};
    recv(sock, ack_buf, sizeof(ack_buf), 0);
    cout << "Server acknowledgment: " << ack_buf << endl;

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to open file\n";
        close(sock);
        return 1;
    }

    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        streamsize bytes_read = file.gcount();

        if (bytes_read > 0) {
            send(sock, buffer, bytes_read, 0);

            memset(ack_buf, 0, sizeof(ack_buf));
            recv(sock, ack_buf, sizeof(ack_buf), 0);
            cout << "Ack: " << ack_buf << endl;
        }
    }

    cout << "File '" << filename << "' sent successfully.\n";

    file.close();
    close(sock);
    return 0;
}
