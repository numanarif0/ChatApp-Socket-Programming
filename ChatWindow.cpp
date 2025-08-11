#include "ChatWindow.hpp"
#include <iostream>
#include <gtkmm.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#include <chrono> 

#define PORT 8080
#define SERVER_IP "127.0.0.1"


ChatWindow::ChatWindow() : m_is_running(true) { 
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
    m_status_dispatcher.connect(sigc::mem_fun(*this, &ChatWindow::on_status_update)); 

    
    m_connection_thread = std::thread(&ChatWindow::connection_manager, this);
}


ChatWindow::~ChatWindow() {
    m_is_running = false; 

    if (m_socket != INVALID_SOCKET) {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
    }


    if (m_connection_thread.joinable()) {
        m_connection_thread.join();
    }
    if (m_receive_thread.joinable()) {
        m_receive_thread.join();
    }

    WSACleanup(); 
}

Gtk::ApplicationWindow* ChatWindow::get_window() {
    return m_window.get();
}


void ChatWindow::connection_manager() {
    WSADATA wsadata;
    int result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result != 0) {
        std::cerr << "WSAStartup basarisiz oldu: " << result << std::endl;
        return;
    }

    while (m_is_running) {
        {
       
            std::lock_guard<std::mutex> lock(m_status_mutex);
            m_status_message = "Sunucuya bağlanmaya çalışılıyor...";
        }
        m_status_dispatcher.emit();

        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket == INVALID_SOCKET) {
            if (m_is_running) { 
                 std::this_thread::sleep_for(std::chrono::seconds(5));
            }
            continue; 
        }

        sockaddr_in serverAddr = {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    
        if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            if (m_is_running) {
                std::this_thread::sleep_for(std::chrono::seconds(5)); 
            }
            continue; 
        }

        
        {
            std::lock_guard<std::mutex> lock(m_status_mutex);
            m_status_message = "Sunucuya bağlanıldı.";
        }
        m_status_dispatcher.emit();

      
        m_receive_thread = std::thread(&ChatWindow::receive_messages, this);
        
        if(m_receive_thread.joinable()) {
            m_receive_thread.join();
        }

      
        if (m_socket != INVALID_SOCKET) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }
        
      
        if (m_is_running) {
            {
                std::lock_guard<std::mutex> lock(m_status_mutex);
                m_status_message = "Sunucu bağlantısı koptu. Yeniden denenecek...";
            }
            m_status_dispatcher.emit();
        }
    }
}


void ChatWindow::receive_messages() {
    char buffer[1024];
    int bytesReceived;
    while (m_is_running) {
        bytesReceived = recv(m_socket, buffer, sizeof(buffer), 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_message_from_thread = std::string(buffer);
            }
            m_dispatcher.emit();
        } else {
           
            break;
        }
    }
}


void ChatWindow::on_status_update() {
    std::string status;
    {
        std::lock_guard<std::mutex> lock(m_status_mutex);
        status = m_status_message;
    }
    m_Information->set_text(status);
}

void ChatWindow::on_message_received() {
    std::string message;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        message = m_message_from_thread;
    }
    auto end_iter = m_chat_buffer->end();
    m_chat_buffer->insert(end_iter, message + "\n");
    m_chat_history_view->scroll_to(m_chat_buffer->get_insert());
}

void ChatWindow::on_send_button_clicked() {
    std::string message = m_input_messages->get_text();

    if (!message.empty() && m_socket != INVALID_SOCKET) {
        int bytesSent = send(m_socket, message.c_str(), message.length(), 0);
        if (bytesSent != SOCKET_ERROR) {
            auto end_iter = m_chat_buffer->end();
            m_chat_buffer->insert(end_iter, "Siz: " + message + "\n");
            m_chat_history_view->scroll_to(m_chat_buffer->get_insert());
            m_input_messages->set_text("");
        } else {
           
            auto end_iter = m_chat_buffer->end();
            m_chat_buffer->insert(end_iter, "[Hata: Mesaj gönderilemedi. Bağlantıyı kontrol edin.]\n");
        }
    }
}