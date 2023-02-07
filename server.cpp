#include <iostream>
#include <fstream>
#include <WinSock2.h>
#include <sstream>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int BUFSIZE = 1024;

int main()
{
    //uncomment this block to print your build directory to the console, this is where you put the pptx files
   /* TCHAR Buffer[MAX_PATH];
    DWORD dwRet;
    dwRet = GetCurrentDirectory(MAX_PATH, Buffer);
    wcout << Buffer << endl;*/

    cout << "ftpclient (Enter port number): ";
    int PORT;
    cin >> PORT;

    cout << "Setting up server listen to port: " << PORT << endl;

    WSADATA wsaData;
    SOCKET sock, clientSock;
    sockaddr_in serverAddr, clientAddr;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSA startup failed: " << WSAGetLastError() << endl;
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == INVALID_SOCKET) {
        cerr << "socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "bind failed: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    if (listen(sock, 1) == SOCKET_ERROR) {
        cerr << "listen failed: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "Listen successful" << endl;
    cout << "Waiting for client connection and message" << endl;

    int clientAddrLen = sizeof(clientAddr);
    clientSock = accept(sock, (sockaddr*)&clientAddr, &clientAddrLen);

    if (clientSock == INVALID_SOCKET) {
        cerr << "accept failed/client socket might be invalid: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char clientMessage[512];

    int result;
    result = recv(clientSock, clientMessage, sizeof(clientMessage), 0);
    if (result > 0) {
        clientMessage[result] = 0;
        cout << "Received message: " << clientMessage << endl;
    }

    else {
        cerr << "No valid message received from client" << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    stringstream ss(clientMessage);
    string operation, fileName;

    getline(ss, operation, ' ');
    getline(ss, fileName);

    if (operation == "upload") {
        const char* newFileName = "newUploadTestFile.pptx";

        ofstream outFile(newFileName, ios::binary);
        if (!outFile.is_open())
        {
            cerr << "Error: Could not create file " << newFileName << endl;
            return 1;
        }

        cout << "Receiving file " << fileName << "..." << endl;

        char buf[BUFSIZE];
        int bytesReceived;
        while ((bytesReceived = recv(clientSock, buf, BUFSIZE, 0)) > 0)
        {
            outFile.write(buf, bytesReceived);
        }

        outFile.close();
        closesocket(clientSock);
        closesocket(sock);
        WSACleanup();

        cout << "File received successfully." << endl;
    }

    else if (operation == "get") {
        ifstream file(fileName, ios::binary);
        if (!file.is_open()) {
            cerr << "Error: Could not open file " << fileName << endl;
            return 1;
        }
        cout << "Sending file " << fileName << "..." << endl;

        char buf[BUFSIZE];
        int bytesRead, bytesSent;
        while (!file.eof()) {
            file.read(buf, BUFSIZE);
            bytesRead = file.gcount();

            bytesSent = send(clientSock, buf, bytesRead, 0);
            if (bytesSent == SOCKET_ERROR) {
                cerr << "send failed: " << WSAGetLastError() << endl;
                file.close();
                closesocket(clientSock);
                closesocket(sock);
                WSACleanup(); 
                return 1;
            }

            if (bytesSent != bytesRead) {
                cerr << "Error: Could not send all data." << endl;
                file.close();
                closesocket(clientSock);
                closesocket(sock);
                WSACleanup();
                return 1;
            }
        }

        cout << "File sent successfully" << endl;
        file.close();
        closesocket(clientSock);
        closesocket(sock);
        WSACleanup();
    }

    else {
        cerr << "Invalid command from client" << endl;
        closesocket(clientSock);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    return 0;
}