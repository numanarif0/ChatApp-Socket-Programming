#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <sstream>
#include <algorithm>

#define PORT 8080

std::map<SOCKET, std::string> socketToUser;
std::map<std::string, SOCKET> userToSocket;
std::map<std::string, SOCKET> userToPmSocket;
std::mutex clientMutex;

void broadcastUserList() {
    std::string userListStr = "USERLIST|";
    std::lock_guard<std::mutex> lock(clientMutex);
    
    if (socketToUser.empty()) {
        userListStr += ","; 
    } else {
        for (auto const& [sock, name] : socketToUser) {
            userListStr += name + ",";
        }
    }

    for (auto const& [sock, name] : socketToUser) {
        send(sock, userListStr.c_str(), userListStr.length(), 0);
    }
}

void handleClient(SOCKET clientSocket) {
    char buffer[2048];
    int bytesReceived;
    std::string username; 


    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }

    std::string firstMsg(buffer, bytesReceived);
    size_t delim = firstMsg.find('|');
    if (delim == std::string::npos) {
        closesocket(clientSocket);
        return;
    }

    std::string type = firstMsg.substr(0, delim);
    username = firstMsg.substr(delim + 1);

    if (type == "LOGIN") {
       
        std::cout << "Lobiye baglandi: " << username << " (" << clientSocket << ")" << std::endl;
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            socketToUser[clientSocket] = username;
        }
        broadcastUserList(); 
    } else if (type == "INIT_PM") {
        
         std::cout << "PM kanali acti: " << username << " (" << clientSocket << ")" << std::endl;
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            userToPmSocket[username] = clientSocket;
        }
    } else {
        closesocket(clientSocket);
        return;
    }


    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        std::string message(buffer, bytesReceived);
        
        if (message.rfind("PM|", 0) == 0) {
            std::stringstream ss(message.substr(3)); 
            std::string recipient, msg_content;
            
            std::getline(ss, recipient, '|');
            std::getline(ss, msg_content);

            std::cout << "'" << username << "' -> '" << recipient << "': " << msg_content << std::endl;

            SOCKET recipientSocket = INVALID_SOCKET;
            {
                std::lock_guard<std::mutex> lock(clientMutex);
                if (userToPmSocket.count(recipient)) {
                    recipientSocket = userToPmSocket[recipient];
                }
            }
            
            if (recipientSocket != INVALID_SOCKET) {
               
                std::string forward_msg = "FROM|" + username + "|" + msg_content;
                send(recipientSocket, forward_msg.c_str(), forward_msg.length(), 0);
            }
        }
    }

    
    std::cout << username << " (" << clientSocket << ") baglantisi koptu." << std::endl;
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        if (socketToUser.count(clientSocket)) {
            
            socketToUser.erase(clientSocket);
            broadcastUserList(); 
        }
        if (userToPmSocket[username] == clientSocket){
        
            userToPmSocket.erase(username);
        }
    }
    closesocket(clientSocket);
}


int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup basarisiz oldu." << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket olusturulamadi." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    std::cout << "Sunucu dinlemede..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }
      
        std::thread(handleClient, clientSocket).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}