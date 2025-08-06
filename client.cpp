#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <string>
#include <thread>


#define PORT 8080

void receiveMessages(SOCKET serverSocket)
{
    char buffer[1024];
    int bytesReceived;
    while (true)
    {
        bytesReceived = recv(serverSocket, buffer, sizeof(buffer), 0);
        buffer[bytesReceived] = '\0';
        std::cout << "Message from Server: " << buffer << std::endl;          
        if (strcmp(buffer, "exit") == 0) {
            std::cout << "Exit..." << std::endl; 
        }
    }
    closesocket(serverSocket);
    
   
}

int main(){


WSADATA wsadata;

int result = WSAStartup(MAKEWORD(2,2),&wsadata);
if (result!=0){
    std::cerr << "WSAStartup basarisiz oldu: " << result << std::endl;
    return 1;
}
SOCKET clientSocket = INVALID_SOCKET;

clientSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

if(clientSocket==INVALID_SOCKET){

    std::cerr<<"Socket creation bu hata ile fail oldu: " << WSAGetLastError() << std::endl;
    return 1 ;
}   

sockaddr_in serverAddr={};
serverAddr.sin_family=AF_INET;
serverAddr.sin_port = htons(PORT);
serverAddr.sin_addr.s_addr = INADDR_ANY;
inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);




connect(clientSocket,(sockaddr*)&serverAddr,sizeof(serverAddr));



while(true){
    std::thread altIslem(receiveMessages,clientSocket);
   // altIslem.detach();
    std::string message;
    std::cout<<"the message you want to send to the server: "<< message<<std::endl;
    std::getline(std::cin ,message); 

    send(clientSocket, message.c_str(), message.length(),0);
    if(message=="exit")
    {
       break;
    }
    
    
}   
std::cout << "Sunucu kapatiliyor." << std::endl;
closesocket(clientSocket);
WSACleanup();
return 0 ;

}