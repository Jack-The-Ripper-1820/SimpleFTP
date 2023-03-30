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

int process(SOCKET clientSock, int clientNo) {
    auto threadId = this_thread::get_id();
    stringstream tmpss;
    tmpss << threadId;
    string id = tmpss.str();

    string clientNoString = to_string(clientNo);

    char clientMessage[512];
    string operation, fileName;

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

    if (send(clientSock, clientNoString.c_str(), clientNoString.length(), 0) == SOCKET_ERROR)
    {
        cerr << "sending client id failed with error: " << WSAGetLastError() << endl;
        closesocket(clientSock);
        return 1;
    }

    if (operation == "upload") {
        cout << "receiving upload from client with client id: " << clientNoString << endl;
        string newFileName_str = "newUploadTestFile" + clientNoString + ".pptx";
        const char* newFileName = (const char*)newFileName_str.c_str();

        ofstream file(newFileName, ios::binary);

        if (!file.is_open())
        {
            cerr << "Error: Could not create file " << newFileName << endl;
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
                std::cout << "Connection closed or file not present" << endl;
            }
            else {
                cerr << "recv failed: " << WSAGetLastError() << endl;
            }
        } while (bytesRec > 0);

        file.close();
        closesocket(clientSock);

        cout << "File transaction closed" << endl;
        
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
                return 1;
            }

            if (bytesS != bytesR) {
                cerr << "Error: partial send, could not send everything." << endl;
                file.close();
                closesocket(clientSock);
                return 1;
            }
        }

        std::cout << "File sent successfully" << endl;
        file.close();
        closesocket(clientSock);
    }

    else {
        cerr << "Invalid command from client" << endl;
        closesocket(clientSock);
        return 1;
    }

    return 0;
}

int main() {
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

    std::cout << "Listen successful" << endl;
    std::cout << "Listening to clients, timeout 60 seconds" << endl;

    int clientNo = 1;
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);

    timeval timeout;
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;

    while (connections > 0) {
        // Wait for either a client connection or a timeout event
        fd_set tmp_fds = read_fds;
        int result = select(0, &tmp_fds, NULL, NULL, &timeout);
        if (result == SOCKET_ERROR) {
            cerr << "select failed: " << WSAGetLastError() << endl;
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        else if (result == 0) {
            // Timeout event
            std::cout << "Server timeout" << endl;
            break;
        }
        else {
            // Client connection event
            SOCKET clientSock;
            sockaddr_in clientSocketAddress;
            int clientSocketAddrLen = sizeof(clientSocketAddress);
            char clientMessage[512];
            string operation, fileName;

            clientSock = accept(sock, (sockaddr*)&clientSocketAddress, &clientSocketAddrLen);

            if (clientSock == INVALID_SOCKET) {
                cerr << "accept failed/client socket might be invalid: " << WSAGetLastError() << endl;
                closesocket(clientSock);
                continue;
            }

            connections--;
            thread connection_thread(process, clientSock, clientNo++);
            threads.push_back(move(connection_thread));
        }
    }

    for (thread& th : threads) {
        th.join();
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}



//int main()
//{
//    // Uncomment this block to print your build directory to the console, this is where you put the pptx files
//    /* TCHAR Buffer[MAX_PATH];
//    DWORD dwRet;
//    dwRet = GetCurrentDirectory(MAX_PATH, Buffer);
//    wcout << Buffer << endl;*/
//
//    cout << "FTP SERVER \n Enter port number and number of connections: ";
//
//    cin >> port >> connections;
//
//    cout << "Setting up server listen to port: " << port << endl;
//
//    serverSocketAddress.sin_family = AF_INET;
//    serverSocketAddress.sin_port = htons(port);
//    serverSocketAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
//
//    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        cerr << "WSA startup failed: " << WSAGetLastError() << endl;
//        return 1;
//    }
//
//    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//
//    if (sock == INVALID_SOCKET) {
//        cerr << "socket failed: " << WSAGetLastError() << endl;
//        WSACleanup();
//        return 1;
//    }
//
//    int timeout = 10000; // 10 seconds in milliseconds
//
//    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int)) == SOCKET_ERROR) {
//        cerr << "Error setting socket options" << endl;
//        closesocket(sock);
//        WSACleanup();
//        return 1;
//    }
//
//    if (bind(sock, (sockaddr*)&serverSocketAddress, sizeof(serverSocketAddress)) == SOCKET_ERROR) {
//        cerr << "bind failed: " << WSAGetLastError() << endl;
//        closesocket(sock);
//        WSACleanup();
//        return 1;
//    }
//
//    if (listen(sock, 1) == SOCKET_ERROR) {
//        cerr << "listen failed: " << WSAGetLastError() << endl;
//        closesocket(sock);
//        WSACleanup();
//        return 1;
//    }
//
//    vector<thread> threads;
//
//    std::cout << "Listen successful" << endl;
//    std::cout << "Listening to clients, timeout " << timeout / 1000 << " seconds" << endl;
//
//    int clientNo = 1;
//    bool timeout_occurred = false;
//
//    while (connections > 0 && !timeout_occurred) {
//
//        SOCKET clientSock;
//        sockaddr_in clientSocketAddress;
//        int clientSocketAddrLen = sizeof(clientSocketAddress);
//        char clientMessage[512];
//        string operation, fileName;
//
//        clientSock = accept(sock, (sockaddr*)&clientSocketAddress, &clientSocketAddrLen);
//
//        if (clientSock == INVALID_SOCKET) {
//            if (WSAGetLastError() == WSAETIMEDOUT) {
//                std::cout << "Accept timed out" << std::endl;
//                timeout_occurred = true;
//                break;
//            }
//
//            cerr << "accept failed/client socket might be invalid: " << WSAGetLastError() << endl;
//            closesocket(clientSock);
//            continue;
//        }
//
//        connections--;
//        thread connection_thread(process, clientSock, clientNo++);
//        threads.push_back(move(connection_thread));
//    }
//
//    if (timeout_occurred) {
//        for (thread& th : threads) {
//            if (th.joinable()) {
//                th.join();
//            }
//        }
//        std::cout << "Server timed out. Terminating..." << endl;
//    }
//    else {
//        for (thread& th : threads) {
//            if (th.joinable()) {
//                th.join();
//            }
//        }
//        std::cout << "All connections have been served..." << endl;
//    }
//
//    return 0;
//}


//int main()
//{
//    //uncomment this block to print your build directory to the console, this is where you put the pptx files
//   /* TCHAR Buffer[MAX_PATH];
//    DWORD dwRet;
//    dwRet = GetCurrentDirectory(MAX_PATH, Buffer);
//    wcout << Buffer << endl;*/
//
//    cout << "FTP SERVER \n Enter port number and number of connections: ";
//    
//    cin >> port >> connections;
//
//    cout << "Setting up server listen to port: " << port << endl;
//
//    serverSocketAddress.sin_family = AF_INET;
//    serverSocketAddress.sin_port = htons(port);
//    serverSocketAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
//
//    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        cerr << "WSA startup failed: " << WSAGetLastError() << endl;
//        return 1;
//    }
//
//    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//
//    if (sock == INVALID_SOCKET) {
//        cerr << "socket failed: " << WSAGetLastError() << endl;
//        WSACleanup();
//        return 1;
//    }
//
//    int timeout = 10;
//
//    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int)) == SOCKET_ERROR) {
//        cerr << "Error setting socket options" << endl;
//        closesocket(sock);
//        WSACleanup();
//        return 1;
//    }
//
//    if (bind(sock, (sockaddr*)&serverSocketAddress, sizeof(serverSocketAddress)) == SOCKET_ERROR) {
//        cerr << "bind failed: " << WSAGetLastError() << endl;
//        closesocket(sock);
//        WSACleanup();
//        return 1;
//    }
//
//    if (listen(sock, 1) == SOCKET_ERROR) {
//        cerr << "listen failed: " << WSAGetLastError() << endl;
//        closesocket(sock);
//        WSACleanup();
//        return 1;
//    }
//
//    vector<thread> threads;
//
//    
//
//    std::cout << "Listen successful" << endl;
//    std::cout << "Listening to clients, timeout " << timeout << " seconds" << endl;
//
//    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
//
//    cin.get();
//
//    int clientNo = 1;
//
//    while (connections > 0) {
//
//        //cout << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time).count() << endl;
//
//        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time).count() >= timeout)
//        {
//            std::cout << "Server timeout" << endl;
//            break;
//        }
//
//        SOCKET clientSock;
//        sockaddr_in clientSocketAddress;
//        int clientSocketAddrLen = sizeof(clientSocketAddress);
//        char clientMessage[512];
//        string operation, fileName;
//
//        clientSock = accept(sock, (sockaddr*)&clientSocketAddress, &clientSocketAddrLen);
//
//        //cout << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time).count() << endl;
//
//
//        if (clientSock == INVALID_SOCKET) {
//            if (WSAGetLastError() == WSAETIMEDOUT) {
//                std::cout << "Accept timed out" << std::endl;
//                break;
//            }
//
//            cerr << "accept failed/client socket might be invalid: " << WSAGetLastError() << endl;
//            closesocket(clientSock);
//            continue;
//        }
//
//        
//        //start_time = std::chrono::steady_clock::now();
//        connections--;
//        thread connection_thread(process, clientSock, clientNo++);
//        threads.push_back(move(connection_thread));
//    }
//
//    for (thread &th : threads) {
//        th.join();
//    }
//
//    closesocket(sock);
//    WSACleanup();
//
//    return 0;
//}