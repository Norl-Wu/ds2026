#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <iostream>
#include <fstream>

using namespace std;

string rpc_upload(string filename, string filedata) {
    string saved = "upload_" + filename;
    ofstream out(saved, ios::binary);
    if (!out.is_open()) return "ERROR: Cannot write file";

    out.write(filedata.c_str(), filedata.size());
    out.close();
    return "OK";
}

string rpc_download(string filename) {
    ifstream in(filename, ios::binary);
    if (!in.is_open()) return "ERROR: NOFILE";

    string data((istreambuf_iterator<char>(in)),
                istreambuf_iterator<char>());
    return data;
}

int main() {   // <<<<<< THIS IS MAIN
    int port = 22000;
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (sockaddr*)&addr, sizeof(addr));
    listen(server_socket, 5);

    cout << "RPC Server listening on " << port << "...\n";

    while (true) {
        sockaddr_in caddr{};
        socklen_t len = sizeof(caddr);

        int client = accept(server_socket, (sockaddr*)&caddr, &len);
        if (client < 0) continue;

        char buf[500000];
        int n = recv(client, buf, sizeof(buf), 0);
        if (n <= 0) { close(client); continue; }

        string req(buf, n);

        if (req.rfind("RPC,", 0) != 0) {
            string err = "ERROR,Bad RPC format";
            send(client, err.c_str(), err.size(), 0);
            close(client);
            continue;
        }

        size_t p1 = req.find(",", 4);
        size_t p2 = req.find(",", p1 + 1);

        string func = req.substr(4, p1 - 4);
        int arg_len = stoi(req.substr(p1 + 1, p2 - p1 - 1));
        string args = req.substr(p2 + 1);

        if (func == "UPLOAD") {
            size_t sep = args.find('\n');
            string filename = args.substr(0, sep);
            string filedata = args.substr(sep + 1);

            string status = rpc_upload(filename, filedata);
            string resp = "OK," + to_string(status.size()) + "," + status;
            send(client, resp.c_str(), resp.size(), 0);
        }

        else if (func == "DOWNLOAD") {
            string filename = args;

            string data = rpc_download(filename);
            string resp = "OK," + to_string(data.size()) + "," + data;
            send(client, resp.c_str(), resp.size(), 0);
        }

        else {
            string err = "ERROR,Unknown RPC";
            send(client, err.c_str(), err.size(), 0);
        }

        close(client);
    }

    return 0;
}
