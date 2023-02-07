#include <iostream>
#include <fstream>
#include <WinSock2.h>
#include <string>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int BUFSIZE = 1024;

int main()
{
    int PORT;
    string SERVER_ADDRESS;
    cout << "Enter operation and file name" << endl;

    string operation;
    cin >> operation;
    string fileName;
    getline(cin, fileName);
    fileName = fileName.substr(1);

    cout << "Enter Server IP Address and Port Number: ";
    cin >> SERVER_ADDRESS >> PORT;

    string sendmessage = operation + " " + fileName;

    WSADATA wsaData;
    SOCKET sock, serverSock;
    sockaddr_in serverAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed: " << WSAGetLastError() << endl;
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cerr << "socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.S_un.S_addr = inet_addr(SERVER_ADDRESS.c_str());

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "server connection failed: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    int result = send(sock, sendmessage.c_str(), sendmessage.length(), 0);
    if (result == SOCKET_ERROR)
    {
        cerr << "send failed with error: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    const char* nfile = fileName.c_str();
    if (operation == "upload") {
        ifstream file(nfile, ios::binary);
        if (!file.is_open())
        {
            cerr << "Error: Could not open file " << nfile << endl;
            return 1;
        }
        cout << "Sending file " << nfile << "..." << endl;

        char buf[BUFSIZE];
        int bytesRead, bytesSent;
        while (!file.eof())
        {
            file.read(buf, BUFSIZE);
            bytesRead = file.gcount();
            bytesSent = send(sock, buf, bytesRead, 0);
            if (bytesSent != bytesRead)
            {
                cerr << "Error: Could not send all data." << endl;
                file.close();
                closesocket(sock);
                WSACleanup();
                return 1;
            }
        }

        cout << "File sent successfully" << endl;

        file.close();
        closesocket(sock);
        WSACleanup();
    }

    else if (operation == "get") {
        const char* newFileName = "newDownloadTestFile.pptx";

        ofstream outFile(newFileName, ios::binary);
        if (!outFile.is_open())
        {
            cerr << "Error: Could not create file " << newFileName << endl;
            closesocket(sock);
            WSACleanup();
            return 1;
        }

        cout << "Receiving file " << fileName << "..." << endl;

        char buf[BUFSIZE];
        int bytesReceived;

        do {
            bytesReceived = recv(sock, buf, BUFSIZE, 0);
            if (bytesReceived > 0) {
                outFile.write(buf, bytesReceived);
            }
            else if (bytesReceived == 0) {
                cout << "Connection closed" << endl;
            }
            else {
                cerr << "recv failed: " << WSAGetLastError() << endl;
            }
        } while (bytesReceived > 0);

        outFile.close();
        closesocket(sock);
        WSACleanup();

        cout << "File received successfully." << endl;
    }

    else {
        cerr << "Invalid command" << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    return 0;
}