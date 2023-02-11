#include <iostream>
#include <WinSock2.h>
#include <fstream>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main()
{
    //uncomment this block to print your build directory to the console, this is where you put the pptx files
   /* TCHAR Buffer[MAX_PATH];
    DWORD dwRet;
    dwRet = GetCurrentDirectory(MAX_PATH, Buffer);
    wcout << Buffer << endl;*/

    const int BUFSIZE = 1024;
    int port;
    SOCKET sock, clientSock;
    sockaddr_in serverSocketAddress, clientSocketAddress;
    char clientMessage[512];
    string operation, fileName;
    WSADATA wsaData;

    cout << "ftpserver (Enter port number): ";
    
    cin >> port;

    cout << "Setting up server listen to port: " << port << endl;

    serverSocketAddress.sin_family = AF_INET;
    serverSocketAddress.sin_port = htons(port);
    serverSocketAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

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

    if (bind(sock, (sockaddr*)&serverSocketAddress, sizeof(serverSocketAddress)) == SOCKET_ERROR) {
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

    int clientSocketAddrLen = sizeof(clientSocketAddress);
    clientSock = accept(sock, (sockaddr*)&clientSocketAddress, &clientSocketAddrLen);

    if (clientSock == INVALID_SOCKET) {
        cerr << "accept failed/client socket might be invalid: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    int result = recv(clientSock, clientMessage, sizeof(clientMessage), 0);

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

    getline(ss, operation, ' ');
    getline(ss, fileName);

    if (operation == "upload") {
        const char* newFileName = "newUploadTestFile.pptx";
        ofstream file(newFileName, ios::binary);

        if (!file.is_open())
        {
            cerr << "Error: Could not create file " << newFileName << endl;
            closesocket(sock);
            WSACleanup();
            return 1;
        }

        cout << "Receiving file " << fileName  << endl;

        int bytesRec;
        char buffer[BUFSIZE];

        do {
            bytesRec = recv(clientSock, buffer, BUFSIZE, 0);
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
        cout << "Sending file " << fileName << endl;

        char buffer[BUFSIZE];
        int bytesR, bytesS;

        while (!file.eof()) {
            file.read(buffer, BUFSIZE);
            bytesR = file.gcount();

            bytesS = send(clientSock, buffer, bytesR, 0);

            if (bytesS == SOCKET_ERROR) {
                cerr << "send failed: " << WSAGetLastError() << endl;
                file.close();
                closesocket(clientSock);
                closesocket(sock);
                WSACleanup(); 
                return 1;
            }

            if (bytesS != bytesR) {
                cerr << "Error: partial send, could not send everything." << endl;
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