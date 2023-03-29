#include <iostream>
#include <WinSock2.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <chrono>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int BUFSIZE = 1024;
int port, opt = 1;
SOCKET sock, clientSock;
sockaddr_in serverSocketAddress;
WSADATA wsaData;
int connections = 0;

int process(SOCKET clientSock) {
    auto threadId = this_thread::get_id();
    stringstream tmpss;
    tmpss << threadId;
    string id = tmpss.str();

    char clientMessage[512];
    string operation, fileName;

    //int clientSocketAddrLen = sizeof(clientSocketAddress);
    //clientSock = accept(sock, (sockaddr*)&clientSocketAddress, &clientSocketAddrLen);

    //if (clientSock == INVALID_SOCKET) {
    //    cerr << "accept failed/client socket might be invalid: " << WSAGetLastError() << endl;
    //    /*closesocket(sock);
    //    WSACleanup();*/
    //    return 1;
    //}

    int result = recv(clientSock, clientMessage, sizeof(clientMessage), 0);

    cout << "receiving info from client" << endl;

    if (result > 0) {
        clientMessage[result] = 0;
        std::cout << "Received message: " << clientMessage << endl;
    }

    else {
        cerr << "No valid message received from client" << endl;
        closesocket(clientSock);
        return 1;
    }

    stringstream ss(clientMessage);

    getline(ss, operation, ' ');
    getline(ss, fileName);

    if (send(clientSock, id.c_str(), id.length(), 0) == SOCKET_ERROR)
    {
        cerr << "sending thread id failed with error: " << WSAGetLastError() << endl;
        closesocket(clientSock);
        return 1;
    }

    if (operation == "upload") {
        cout << "receiving upload from client with thread id: " << id << endl;
        string newFileName_str = "newUploadTestFile" + id + ".pptx";
        const char* newFileName = (const char*)newFileName_str.c_str();

        ofstream file(newFileName, ios::binary);

        if (!file.is_open())
        {
            cerr << "Error: Could not create file " << newFileName << endl;
            /*closesocket(sock);
            WSACleanup();*/
            return 1;
        }

        std::cout << "Receiving file " << fileName << endl;

        int bytesRec;
        char buffer[BUFSIZE];

        do {
            bytesRec = recv(clientSock, buffer, BUFSIZE, 0);
            if (bytesRec > 0) {
                file.write(buffer, bytesRec);
            }
            else if (bytesRec == 0) {
                std::cout << "Connection closed" << endl;
            }
            else {
                cerr << "recv failed: " << WSAGetLastError() << endl;
            }
        } while (bytesRec > 0);

        file.close();
        closesocket(clientSock);
        /*closesocket(sock);
        WSACleanup();*/

        std::cout << "File received successfully." << endl;
    }

    else if (operation == "get") {
        ifstream file(fileName, ios::binary);

        if (!file.is_open()) {
            cerr << "Error: Could not open file " << fileName << endl;
            return 1;
        }
        std::cout << "Sending file " << fileName << endl;

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
                /*closesocket(sock);
                WSACleanup();*/
                return 1;
            }

            if (bytesS != bytesR) {
                cerr << "Error: partial send, could not send everything." << endl;
                file.close();
                closesocket(clientSock);
                /*closesocket(sock);
                WSACleanup();*/
                return 1;
            }
        }

        std::cout << "File sent successfully" << endl;
        file.close();
        closesocket(clientSock);
        /*closesocket(sock);
        WSACleanup();*/
    }

    else {
        cerr << "Invalid command from client" << endl;
        closesocket(clientSock);
        /*closesocket(sock);
        WSACleanup();*/
        return 1;
    }

    return 0;
}
int main()
{
    //uncomment this block to print your build directory to the console, this is where you put the pptx files
   /* TCHAR Buffer[MAX_PATH];
    DWORD dwRet;
    dwRet = GetCurrentDirectory(MAX_PATH, Buffer);
    wcout << Buffer << endl;*/

    cout << "FTP SERVER \n Enter port number and number of connections: ";
    
    cin >> port >> connections;

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

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        cerr << "Error setting socket options" << endl;
        closesocket(sock);
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

    vector<thread> threads;

    const int timeout = 10;

    std::cout << "Listen successful" << endl;
    std::cout << "Listening to clients, timeout " << timeout << "seconds" << endl;

    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

    cin.get();

    while (connections > 0) {

        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time).count() >= timeout)
        {
            std::cout << "Server timeout" << endl;
            break;
        }

        SOCKET clientSock;
        sockaddr_in clientSocketAddress;
        int clientSocketAddrLen = sizeof(clientSocketAddress);
        char clientMessage[512];
        string operation, fileName;

        clientSock = accept(sock, (sockaddr*)&clientSocketAddress, &clientSocketAddrLen);

        if (clientSock == INVALID_SOCKET) {
            cerr << "accept failed/client socket might be invalid: " << WSAGetLastError() << endl;
            closesocket(clientSock);
            //WSACleanup();
            continue;
        }

        
        start_time = std::chrono::steady_clock::now();
        connections--;
        thread connection_thread(process, clientSock);
        threads.push_back(move(connection_thread));
    }

    for (thread &th : threads) {
        th.join();
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}