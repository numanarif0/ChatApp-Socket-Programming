#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <string>
#include <thread>

#define PORT 8080


void receiveMessages(SOCKET clientSocket)
{
    char buffer[1024];
    int bytesReceived;
    while (true)
    {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        buffer[bytesReceived] = '\0';
        std::cout << "Message from client: " << buffer << std::endl;          
        if (strcmp(buffer, "exit") == 0) {
            std::cout << "Exit Client..." << std::endl; 
            break;
        }
       if(bytesReceived==-1){
        std::cout<<"Client closed";
        break;

       } 
    }   
    closesocket(clientSocket);
}

void sendMessages(SOCKET clientSocket)
{

    while (true){
           
            std::string messages;
            std::cout<<"the message you want to send to the client: "<<std::endl;
            std::getline(std::cin,messages);

            send(clientSocket,messages.c_str(),sizeof(messages),0);  
            if(messages=="exit")
            {
                break;
            }

        }
        

}





int main()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup basarisiz oldu: " << result << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation bu hata ile fail oldu: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) { 
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) { 
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Sunucu dinlemede..." << std::endl;
    

    bool shutdown = false; 

    while (!shutdown) { 
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) { 
            std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
            continue; 
        }
        
        std::cout << "Yeni client baglandi" << std::endl; 
         std::thread receiveIslem(receiveMessages,clientSocket);
         receiveIslem.detach(); 
         std::thread sendIslem(sendMessages,clientSocket);
         sendIslem.detach();    
        
    }
    
    std::cout << "Sunucu kapatiliyor." << std::endl;
    closesocket(serverSocket);
    WSACleanup(); 

    return 0;
}   