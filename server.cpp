#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm> 

#define PORT 8080


std::vector<SOCKET> clientSockets;
std::mutex clientMutex;
std::vector<char> messagesFromOtherClient;





void receiveMessages(SOCKET clientSocket)
{
    char buffer[1024];
    int bytesReceived;

    while (true)
    {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        
     
        if (bytesReceived <= 0) {
            std::cout << "Client " << clientSocket << " baglantisi koptu." << std::endl;
            
      
            std::lock_guard<std::mutex> lock(clientMutex);
            clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
            
            
            closesocket(clientSocket);
            break; 
        }
        
        buffer[bytesReceived] = '\0';

        std::cout << "Message from client " << clientSocket << ": " << buffer << std::endl;
        messagesFromOtherClient.insert(messagesFromOtherClient.end(), buffer, buffer + bytesReceived);
        std::lock_guard<std::mutex> lock(clientMutex);
        for (SOCKET sock : clientSockets) {
            if (sock != clientSocket) {
                
                std::string msgWithID = std::to_string(clientSocket) + ": " + std::string(buffer, bytesReceived);
                send(sock, msgWithID.c_str(), msgWithID.length(), 0);
            }
        }
        

    }
}


void sendMessagesToAll()
{
    std::string message;
    while (true) {
        std::cout << "Tum client'lara gonderilecek mesaj: ";
        std::getline(std::cin, message);
        message = "Server: " + message;

        if (message == "exit") {
            
            break;
        }

        std::lock_guard<std::mutex> lock(clientMutex);
        if (clientSockets.empty()) {
            std::cout << "(Hic bagli client yok)" << std::endl;
            continue;
        }

        
        for (SOCKET clientSocket : clientSockets) {
            send(clientSocket, message.c_str(), message.length(), 0);
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

   
    std::thread sendToAllThread(sendMessagesToAll);
    sendToAllThread.detach(); 
   

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
            continue;
        }
        
        std::cout << "Yeni client baglandi: " << clientSocket << std::endl;

  


   
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            clientSockets.push_back(clientSocket);
        }
       
        
        
        std::thread receiveThread(receiveMessages, clientSocket);
        receiveThread.detach();
        
      
    }

    std::cout << "Sunucu kapatiliyor." << std::endl;
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}