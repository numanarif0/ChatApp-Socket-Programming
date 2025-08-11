#include "ChatWindow.hpp"
#include <ws2tcpip.h>
#include <iostream>
#include <stdexcept>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

ChatWindow::ChatWindow(Gtk::Window& parent, const std::string& my_username, const std::string& target_username)
: m_my_username(my_username), m_target_username(target_username) {
    
    m_builder = Gtk::Builder::create_from_file("chatapp.ui");
    m_window = m_builder->get_widget<Gtk::Window>("ChatWindow");
    if (!m_window) {
        throw std::runtime_error("Hata: 'ChatWindow' UI dosyasından alınamadı.");
    }
    
    m_window->set_transient_for(parent);
    m_window->set_title(target_username + " ile sohbet");

    m_status_label = m_builder->get_widget<Gtk::Label>("status_label");
    m_send_button = m_builder->get_widget<Gtk::Button>("send_button");
    m_chat_history_view = m_builder->get_widget<Gtk::TextView>("chat_history_view");
    m_message_input = m_builder->get_widget<Gtk::Entry>("message_input");
    m_chat_buffer = m_chat_history_view->get_buffer();

    m_send_button->signal_clicked().connect(sigc::mem_fun(*this, &ChatWindow::on_send_button_clicked));
    m_message_input->signal_activate().connect(sigc::mem_fun(*this, &ChatWindow::on_send_button_clicked));
    
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

Gtk::Window* ChatWindow::get_window() {
    return m_window;
}

void ChatWindow::connection_manager() {
    try {
        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
            std::cerr << "!!! ChatWindow Thread: WSAStartup hatasi." << std::endl;
            return;
        }

        while(m_is_running) {
            {
                std::lock_guard<std::mutex> lock(m_status_mutex);
                m_status_message = "Bağlanılıyor...";
            }
            m_status_dispatcher.emit();

            m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (m_socket == INVALID_SOCKET) {
                std::cerr << "!!! ChatWindow Thread: Socket olusturulamadi." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(3));
                continue;
            }

            sockaddr_in serverAddr = {};
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(PORT);
            inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

            if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                std::cerr << "!!! ChatWindow Thread: Sunucuya baglanilamadi." << std::endl;
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                std::this_thread::sleep_for(std::chrono::seconds(3));
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(m_status_mutex);
                m_status_message = "Bağlandı.";
            }
            m_status_dispatcher.emit();
            
            std::string init_msg = "INIT_PM|" + m_my_username;
            send(m_socket, init_msg.c_str(), init_msg.length(), 0);

            if(m_receive_thread.joinable()) m_receive_thread.join();
            m_receive_thread = std::thread(&ChatWindow::receive_messages, this);
            m_receive_thread.join(); 

            if (m_socket != INVALID_SOCKET) {
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "!!! connection_manager thread'inde YAKALANAMAYAN HATA: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "!!! connection_manager thread'inde bilinmeyen bir türde YAKALANAMAYAN HATA olustu!" << std::endl;
    }
}


void ChatWindow::receive_messages() {
    char buffer[1024];
    int bytesReceived;
    while (m_is_running) {
        bytesReceived = recv(m_socket, buffer, sizeof(buffer), 0);

        if (bytesReceived > 0) {
            std::string received_data(buffer, bytesReceived);
            
            if (received_data.rfind("FROM|", 0) == 0) {
                std::string msg_part = received_data.substr(5); 
                size_t delimiter = msg_part.find('|');
                if(delimiter != std::string::npos) {
                    std::string sender = msg_part.substr(0, delimiter);
                  
                    if(sender == m_target_username) {
                         std::lock_guard<std::mutex> lock(m_mutex);
                         m_message_from_thread = msg_part.substr(delimiter + 1);
                         m_dispatcher.emit();
                    }
                }
            }
        } else {
            break;
        }
    }
}


void ChatWindow::on_send_button_clicked() {
    std::string message_text = m_message_input->get_text();
    if (!message_text.empty() && m_socket != INVALID_SOCKET) {
        std::string message_to_send = "PM|" + m_target_username + "|" + message_text;
        
        send(m_socket, message_to_send.c_str(), message_to_send.length(), 0);
        append_message(message_text, true); 
        m_message_input->set_text("");
    }
}

void ChatWindow::on_message_received() {
    std::string message;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        message = m_message_from_thread;
    }
    append_message(message, false); 
}

void ChatWindow::on_status_update() {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    m_status_label->set_text(m_status_message);
}

void ChatWindow::append_message(const std::string& message, bool is_mine) {
    auto end_iter = m_chat_buffer->end();
    std::string formatted_message;
    if (is_mine) {
        formatted_message = "Siz: " + message + "\n";
    } else {
        formatted_message = m_target_username + ": " + message + "\n";
    }
    m_chat_buffer->insert(end_iter, formatted_message);
    m_chat_history_view->scroll_to(m_chat_buffer->get_insert());
}