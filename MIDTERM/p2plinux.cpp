#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>     

#define PORT 8080
#define BUFFER_SIZE 4096

using namespace std;
namespace fs = std::filesystem;

struct FileMetadata {
    char filename[256];
    long filesize;
    bool fileFound;
};

string getFileList() {
    string list = "";
    int count = 0;
    try {
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file()) {
                string fname = entry.path().filename().string();
                if (fname != "p2p_linux" && fname != "p2p_linux.cpp") { 
                    list += "- " + fname + "\n";
                    count++;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        return "Error scanning directory.\n";
    }

    if (count == 0) return "No files available on Server.\n";
    return list;
}


void startServer() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    cout << "[Server] Running on Linux Port " << PORT << "..." << endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        cout << "\n[Info] Client connected." << endl;

        string fileList = getFileList();
        send(new_socket, fileList.c_str(), fileList.length() + 1, 0);

        char requestedFile[256] = {0};
        int valread = read(new_socket, requestedFile, 256);
        if (valread <= 0) {
            close(new_socket);
            continue;
        }

        cout << "[Request] Client wants: " << requestedFile << endl;

        ifstream inFile(requestedFile, ios::binary);
        FileMetadata metadata;
        strcpy(metadata.filename, requestedFile);

        if (inFile) {
            inFile.seekg(0, ios::end);
            metadata.filesize = inFile.tellg();
            inFile.seekg(0, ios::beg);
            metadata.fileFound = true;
        } else {
            metadata.filesize = 0;
            metadata.fileFound = false;
        }

        send(new_socket, &metadata, sizeof(metadata), 0);

        if (metadata.fileFound) {
            char buffer[BUFFER_SIZE];
            while (inFile.read(buffer, BUFFER_SIZE)) {
                send(new_socket, buffer, inFile.gcount(), 0);
            }
            if (inFile.gcount() > 0) {
                send(new_socket, buffer, inFile.gcount(), 0);
            }
            inFile.close();
            cout << "[Success] File sent: " << requestedFile << endl;
        } else {
            cout << "[Error] File not found." << endl;
        }

        close(new_socket);
    }
    close(server_fd);
}

void startClient() {
    string ipAddress, filename;
    
    cout << "Enter Server IP: ";
    cin >> ipAddress;

    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "\n Socket creation error \n";
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ipAddress.c_str(), &serv_addr.sin_addr) <= 0) {
        cout << "\nInvalid address/ Address not supported \n";
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "\nConnection Failed \n";
        return;
    }

    char listBuffer[4096] = {0};
    cout << "\n--- FILES ON SERVER ---\n";
    read(sock, listBuffer, 4096);
    cout << listBuffer;
    cout << "-----------------------\n";

    cin.ignore(); 
    cout << "Enter filename to download: ";
    getline(cin, filename);

    send(sock, filename.c_str(), filename.length() + 1, 0);

    FileMetadata metadata;
    read(sock, &metadata, sizeof(metadata));

    if (!metadata.fileFound) {
        cout << "[Server] File not found!" << endl;
        close(sock);
        return;
    }

    cout << "[Downloading] " << metadata.filename << " (" << metadata.filesize << " bytes)..." << endl;

    string outputName = string("downloaded_") + metadata.filename;
    ofstream outFile(outputName, ios::binary);
    
    char buffer[BUFFER_SIZE];
    long totalReceived = 0;
    while (totalReceived < metadata.filesize) {
        int bytesReceived = read(sock, buffer, BUFFER_SIZE);
        if (bytesReceived <= 0) break;
        outFile.write(buffer, bytesReceived);
        totalReceived += bytesReceived;
    }

    cout << "[Success] Saved as: " << outputName << endl;
    outFile.close();
    close(sock);
}

int main() {
    int choice;
    cout << "=== LINUX P2P FILE DOWNLOADER ===" << endl;
    cout << "1. Server (Share files)" << endl;
    cout << "2. Client (Download files)" << endl;
    cout << "Select: ";
    cin >> choice;

    if (choice == 1) startServer();
    else if (choice == 2) startClient();

    return 0;
}