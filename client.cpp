#include <iostream>
#include <WinSock2.h>
#include <fstream>
#include <string>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main()
{
    const int BUFSIZE = 1024;
    const char* SERVER_ADDRESS = "127.0.0.1";
    int port;
    sockaddr_in serverSocketAddress;
    WSADATA wsaData;
    string init_string, operation, fileName, sendMessage;
    SOCKET sock, serverSock;

    cin >> init_string >> port;

    while (init_string != "ftpclient") {
        cerr << "Invalid initialization, the format is ftpclint <port>, try again" << endl;
        cin >> init_string >> port;
    }

    cout << "Input operation (upload/get) and filename with extension (filename.pptx)" << endl;

    cin >> operation;

    getline(cin, fileName);
    fileName = fileName.substr(1);

    sendMessage = operation + " " + fileName;

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

    serverSocketAddress.sin_addr.S_un.S_addr = inet_addr(SERVER_ADDRESS);
    serverSocketAddress.sin_family = AF_INET;
    serverSocketAddress.sin_port = htons(port);

    if (connect(sock, (sockaddr*)&serverSocketAddress, sizeof(serverSocketAddress)) == SOCKET_ERROR) {
        cerr << "server connection failed: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    if (send(sock, sendMessage.c_str(), sendMessage.length(), 0) == SOCKET_ERROR)
    {
        cerr << "send failed with error: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char serverMessage[512];
    string id;
    int result = recv(sock, serverMessage, sizeof(serverMessage), 0);

    cout << "receiving client id from server" << endl;

    if (result > 0) {
        serverMessage[result] = 0;
        std::cout << "Received assigned client id: " << serverMessage << endl;
    }

    else {
        cerr << "No valid message received from server" << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    stringstream ss(serverMessage);
    ss >> id;


    const char* nfile = fileName.c_str();

    if (operation == "upload") {
        ifstream file(nfile, ios::binary);

        if (!file.is_open()) {
            cerr << "Error: Could not open file " << nfile << endl;
            closesocket(sock);
            WSACleanup();
            return 1;
        }

        cout << "Sending file " << nfile << endl;

        char buffer[BUFSIZE];
        int bytesR, bytesS;

        while (!file.eof()) {
            file.read(buffer, BUFSIZE);
            bytesR = file.gcount();
            bytesS = send(sock, buffer, bytesR, 0);
            if (bytesS != bytesR)
            {
                cerr << "Error: Could not send all data." << endl;
                file.close();
                closesocket(sock);
                WSACleanup();
                return 1;
            }
        }

        cout << "File sending process finished" << endl;

        file.close();
        closesocket(sock);
        WSACleanup();
    }

    else if (operation == "get") {
        string newFileName_str = "newDownloadTestFile" + id + ".pptx";
        const char* newFileName = (const char*)newFileName_str.c_str();

        ofstream file(newFileName, ios::binary);
        if (!file.is_open())
        {
            cerr << "Error: Could not create file " << newFileName << endl;
            closesocket(sock);
            WSACleanup();
            return 1;
        }

        cout << "Receiving file " << fileName << endl;

        int bytesRec;
        char buffer[BUFSIZE];

        do {
            bytesRec = recv(sock, buffer, BUFSIZE, 0);
            if (bytesRec > 0) {
                file.write(buffer, bytesRec);
            }
            else if (bytesRec == 0) {
                cout << "Connection closed" << endl;
            }
            else {
                cerr << "recv failed: " << WSAGetLastError() << endl;
            }
        } while (bytesRec > 0);

        file.close();
        closesocket(sock);
        WSACleanup();

        cout << "File receive process finished" << endl;
    }

    else {
        cerr << "Invalid command" << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    return 0;
}