#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <cstring>


#define PORT 8080


int main()
{
    WSADATA wsaData;

    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
     if (result != 0) {
        std::cerr << "WSAStartup basarisiz oldu: " << result << std::endl;
        return 1;
    }
    SOCKET serverSocket = INVALID_SOCKET;
 

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation bu hata ile fail oldu: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    sockaddr_in serverAddr={};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    int len_serverAddr = sizeof(serverAddr);
    

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    
    listen(serverSocket, SOMAXCONN) == SOCKET_ERROR;
    std::cout << "Sunucu dinlemede..." << std::endl;

    while (true){
    int clientSocket = accept(serverSocket,nullptr,nullptr);
        char buffer[1024] = {0};
        recv(clientSocket,buffer,sizeof(buffer),0);
     /*   if(strcmp(buffer,"exit\n"))
        {
            std::cout<<  "exit" << std::endl;
            break;
        } */
        std::cout<< "Message from client:" << buffer << std::endl;
        
        }
    
closesocket(serverSocket);

return 0 ;


}

    

