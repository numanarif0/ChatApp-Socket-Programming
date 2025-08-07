#include "ChatWindow.hpp"
#include <iostream>
#include <gtkmm.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>

#define PORT 8080

ChatWindow::ChatWindow() : m_is_running(false) {
    auto builder = Gtk::Builder::create_from_file("chatapp.ui");

    
    m_window = std::static_pointer_cast<Gtk::ApplicationWindow>(builder->get_object("main_window"));
    if (!m_window) {
        throw std::runtime_error("Hata: 'main_window' UI dosyasından alınamadı.");
    }

   
    m_Information = builder->get_widget<Gtk::Label>("m_Information");
    m_SendMessage = builder->get_widget<Gtk::Button>("m_SendMessage");
    m_chat_history_view = builder->get_widget<Gtk::TextView>("chat_history_view");
    m_input_messages = builder->get_widget<Gtk::Entry>("message_input");
    
    m_chat_buffer = m_chat_history_view->get_buffer();

 
    m_SendMessage->signal_clicked().connect(sigc::mem_fun(*this, &ChatWindow::on_send_button_clicked));
    m_input_messages->signal_activate().connect(sigc::mem_fun(*this, &ChatWindow::on_send_button_clicked)); 
    m_dispatcher.connect(sigc::mem_fun(*this, &ChatWindow::on_message_received));

    connect_to_server();
}


ChatWindow::~ChatWindow()
{
    m_is_running = false;

    if (m_socket != INVALID_SOCKET) {
        shutdown(m_socket, SD_BOTH); 
        closesocket(m_socket);
    }
    if (m_receive_thread.joinable()) {
        m_receive_thread.join();
    }
    WSACleanup();
}
Gtk::ApplicationWindow* ChatWindow::get_window() {
    return m_window.get();
}
void ChatWindow::connect_to_server(){
    WSADATA wsadata;
    int result = WSAStartup(MAKEWORD(2,2),&wsadata);
    if (result!=0){
        std::cerr << "WSAStartup basarisiz oldu: " << result << std::endl;
        return;
    }
    m_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(m_socket==INVALID_SOCKET)
    {
        std::cerr<<"Socket creation bu hata ile fail oldu: " << WSAGetLastError() << std::endl;
        return ;
    }
     
    sockaddr_in serverAddr={};
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed with error: " << WSAGetLastError() << std::endl;
        closesocket(m_socket);
        WSACleanup();
        return ;
    }

    m_Information->set_text("Sunucuya baglandi");
    m_is_running=true;
    m_receive_thread = std::thread(&ChatWindow::receive_messages,this);
}

void ChatWindow::receive_messages(){
    char buffer[1024];
    int bytesReceived;
    while (m_is_running)
    {
        bytesReceived = recv(m_socket, buffer, sizeof(buffer), 0);
        
        std::string received_text;
        if(bytesReceived > 0){
           buffer[bytesReceived] = '\0';
           received_text = std::string(buffer);
        } else {
            received_text = "Sunucu bağlantısı koptu.";
            m_is_running = false;
        }
        
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_message_from_thread = received_text;
        }
        m_dispatcher.emit();

        if (!m_is_running) {
            break;
        }
    }
}

void ChatWindow::on_message_received(){
    std::string message;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        message = m_message_from_thread;
    }
    auto end_iter = m_chat_buffer->end();
    m_chat_buffer->insert(end_iter,"Server: " + message + "\n");
    m_chat_history_view->scroll_to(m_chat_buffer->get_insert());
}

void ChatWindow::

on_send_button_clicked(){
    std::string message = m_input_messages->get_text();

    if (!message.empty()) {
        send(m_socket, message.c_str(), message.length(), 0);
        auto end_iter = m_chat_buffer->end();
        m_chat_buffer->insert(end_iter, "Siz: " + message + "\n");
        m_chat_history_view->scroll_to(m_chat_buffer->get_insert());

        m_input_messages->set_text("");
    }
}