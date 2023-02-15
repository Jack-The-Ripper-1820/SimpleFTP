# Simple File Transfer C++

Visual Studio - https://learn.microsoft.com/en-us/visualstudio/install/install-visual-studio?view=vs-2022

Visual Studio CMake - https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-170

(If not using Visual Studio) CMake Installation - https://cmake.org/install/

Here we are using WinSock2 library to access socket functionality and send files between the programs.

This project can transfer any pptx files between the client and the server program

It is recommended to run this project on Visual Studio although VS Code can also be used.

Both the server and the client can send and receive files in this program.

## Functions used: 

The port number to which the server listens is taken as an input from the user. 

The server then connects to the socket and starts listening to incoming requests. 

The WSAStartup() function initializes Winsock 2 DLL (Ws2_32.dll).

The socket() function creats a socket that is bound to a specific transport provider

The bind() function associates a local address with a socket.

The listen() function listens to the specified socket.

The accept() function permits an incoming connection attempt on a socket.

The closesocket() clises the connection to the specfiied socket.

The WSACleanup() function terminates use of the Winsock 2 DLL (Ws2_32.dll).

The recv() function receives data from a connected socket or a bound connectionless socket.

The connect() function establishes a connection to a specified socket.

The send() function sends data to a connected socket or a bound connection.

The WSAGetLastError() function returns the error status for the last Windows Sockets operation that failed.


## Server.cpp:

Server listens to the specified port number and after getting the request from the client, parses it and either sends the file to the client or receives the file from the client.

## Client.cpp:

Client connects to the server at the specified port number, then according to the input given by the user, either sends or receives a file to or from the server.

###NOTE - This program can send and receive all files which are compatible with ios::binary, specifically only pptx files.





