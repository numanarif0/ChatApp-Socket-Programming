// server.cpp

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

// Sadece bu iki harita yeterli. PM için ayrı bir haritaya gerek yok.
std::map<SOCKET, std::string> socket_to_user;
std::map<std::string, SOCKET> user_to_socket;
std::mutex client_mutex;

void broadcast_user_list() {
    std::string user_list_str = "USERLIST|";
    std::lock_guard<std::mutex> lock(client_mutex);
    
    if (user_to_socket.empty()) {
        user_list_str += ","; 
    } else {
        for (auto const& [name, sock] : user_to_socket) {
            user_list_str += name + ",";
        }
    }

    // Listeyi herkese gönder
    for (auto const& [name, sock] : user_to_socket) {
        send(sock, user_list_str.c_str(), user_list_str.length(), 0);
    }
}

void handle_client(SOCKET client_socket) {
    char buffer[2048];
    int bytes_received;
    std::string username;

    // Kullanıcının ilk mesajı LOGIN olmalı
    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        closesocket(client_socket);
        return;
    }

    std::string first_msg(buffer, bytes_received);
    if (first_msg.rfind("LOGIN|", 0) == 0) {
        username = first_msg.substr(6);
        std::cout << "Kullanici baglandi: " << username << " (" << client_socket << ")" << std::endl;
        
        {
            std::lock_guard<std::mutex> lock(client_mutex);
            socket_to_user[client_socket] = username;
            user_to_socket[username] = client_socket;
        }
        broadcast_user_list();
    } else {
        // Geçersiz ilk mesaj, bağlantıyı kapat
        closesocket(client_socket);
        return;
    }

    // Mesaj döngüsü
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        std::string message(buffer, bytes_received);
        
        // Sadece özel mesajları (PM) işle
        if (message.rfind("PM|", 0) == 0) {
            std::stringstream ss(message.substr(3));
            std::string recipient, msg_content;
            
            std::getline(ss, recipient, '|');
            std::getline(ss, msg_content);

            std::cout << "'" << username << "' -> '" << recipient << "': " << msg_content << std::endl;

            SOCKET recipient_socket = INVALID_SOCKET;
            {
                std::lock_guard<std::mutex> lock(client_mutex);
                if (user_to_socket.count(recipient)) {
                    recipient_socket = user_to_socket[recipient];
                }
            }
            
            if (recipient_socket != INVALID_SOCKET) {
                // Mesajı alıcıya şu formatta ilet: "FROM|gonderen_kullanici|mesaj"
                std::string forward_msg = "FROM|" + username + "|" + msg_content;
                send(recipient_socket, forward_msg.c_str(), forward_msg.length(), 0);
            }
        }
    }

    // Bağlantı koptu
    std::cout << username << " (" << client_socket << ") baglantisi koptu." << std::endl;
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        socket_to_user.erase(client_socket);
        user_to_socket.erase(username);
    }
    broadcast_user_list(); // Herkese güncel listeyi tekrar gönder
    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(server_socket, SOMAXCONN);

    std::cout << "Sunucu dinlemede..." << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        std::thread(handle_client, client_socket).detach();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}