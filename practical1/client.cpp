#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <iostream>
#include <fstream>

using namespace std;

int sock;  // Make socket global so upload() can use it

void upload(const string& filename)
{
    struct stat st;
    if (stat(filename.c_str(), &st) != 0) {
        cerr << "Unable to stat file\n";
        return;
    }
    long file_size = st.st_size;

    // Send header: filename,file_size
    string header = "UPLOAD," + filename + "," + to_string(file_size);
    send(sock, header.c_str(), header.size(), 0);

    // Receive server ack
    char ack_buf[1024] = {0};
    recv(sock, ack_buf, sizeof(ack_buf), 0);
    cout << "Server: " << ack_buf << endl;

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to open file\n";
        return;
    }

    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        streamsize bytes_read = file.gcount();

        if (bytes_read > 0) {
            send(sock, buffer, bytes_read, 0);

            memset(ack_buf, 0, sizeof(ack_buf));
            recv(sock, ack_buf, sizeof(ack_buf), 0);
        }
    }

    cout << "Upload done.\n";
}

void download()
{
    string target;
    cout << "Enter filename to download: ";
    cin >> target;

    string header = "DOWNLOAD," + target;
    send(sock, header.c_str(), header.size(), 0);

    char buf[1024] = {0};
    recv(sock, buf, sizeof(buf), 0);

    string resp = buf;

    if (resp == "NOFILE") {
        cout << "Server: File does not exist.\n";
        return;
    }

    size_t pos = resp.find(',');
    long file_size = stol(resp.substr(pos + 1));

    cout << "Downloading " << target << " (" << file_size << " bytes)...\n";

    string out_name = "downloaded_" + target;
    ofstream outfile(out_name, ios::binary);

    long received = 0;
    while (received < file_size) {
        int bytes = recv(sock, buf, sizeof(buf), 0);
        if (bytes <= 0) break;

        outfile.write(buf, bytes);
        received += bytes;
    }

    outfile.close();
    cout << "Download complete: " << out_name << endl;
}

void choosingBehavior()
{
    int path;
    cout << "What do you want?\n1: Upload file\n2: Download file\nYour choice: ";
    cin >> path;

    if (path == 1) {
        string filename;
        cout << "Enter filename: ";
        cin >> filename;
        upload(filename);
    }
    else if (path == 2) {
        download();
    }
}

int main() {
    const char* server_ip = "127.0.0.1";
    int server_port = 21003;

    sock = socket(AF_INET, SOCK_STREAM, 0);
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

    choosingBehavior();

    close(sock);
    return 0;
}
