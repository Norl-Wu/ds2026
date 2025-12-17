#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

using namespace std;

int sockfd;

string rpc_call(string func, string args) {
    string req = "RPC," + func + "," + to_string(args.size()) + "," + args;
    send(sockfd, req.c_str(), req.size(), 0);

    char buf[500000];
    int n = recv(sockfd, buf, sizeof(buf), 0);
    return string(buf, n);
}

void upload_rpc() {
    string filename;
    cout << "Enter filename: ";
    cin >> filename;

    ifstream in(filename, ios::binary);
    if (!in.is_open()) {
        cout << "File cannot open\n";
        return;
    }

    string data((istreambuf_iterator<char>(in)),
                istreambuf_iterator<char>());

    string args = filename + "\n" + data;
    string resp = rpc_call("UPLOAD", args);

    cout << "Server: " << resp << endl;
}

void download_rpc() {
    string filename;
    cout << "Enter filename: ";
    cin >> filename;

    string resp = rpc_call("DOWNLOAD", filename);

    size_t p1 = resp.find(',');
    size_t p2 = resp.find(',', p1 + 1);
    string data = resp.substr(p2 + 1);

    ofstream out("downloaded_" + filename, ios::binary);
    out.write(data.c_str(), data.size());

    cout << "Downloaded\n";
}

int main() {  // <<<<< THIS IS MAIN
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(22000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sockfd, (sockaddr*)&addr, sizeof(addr));

    cout << "1: Upload\n2: Download\nChoice: ";
    int c;
    cin >> c;

    if (c == 1) upload_rpc();
    if (c == 2) download_rpc();

    close(sockfd);
    return 0;
}
