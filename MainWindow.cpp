// MainWindow.cpp

#include "MainWindow.hpp"
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

MainWindow::MainWindow() {
    auto builder = Gtk::Builder::create_from_file("mainwindow.ui");
    m_window = std::static_pointer_cast<Gtk::ApplicationWindow>(builder->get_object("main_lobby_window"));
    m_info_label = builder->get_widget<Gtk::Label>("info_label");
    m_user_list_box = builder->get_widget<Gtk::ListBox>("user_list_box");

    if (!m_window || !m_info_label || !m_user_list_box) {
        throw std::runtime_error("Hata: Lobi penceresi UI elemanları yüklenemedi.");
    }

    m_user_list_dispatcher.connect(sigc::mem_fun(*this, &MainWindow::on_user_list_update));
    m_pm_dispatcher.connect(sigc::mem_fun(*this, &MainWindow::on_pm_received));
    
    get_username();
}

MainWindow::~MainWindow() {
    m_is_running = false;
    if (m_socket != INVALID_SOCKET) {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
    }
    if (m_receive_thread.joinable()) {
        m_receive_thread.join();
    }
    WSACleanup();

    for(auto const& [key, val] : m_open_chats) {
        delete val;
    }
}

Gtk::ApplicationWindow* MainWindow::get_window() {
    return m_window.get();
}

void MainWindow::on_user_activated(Gtk::ListBoxRow* row) {
    auto label = dynamic_cast<Gtk::Label*>(row->get_child());
    if (label) {
        std::string target_user = label->get_text();
        if (target_user != m_username) { 
            if (m_open_chats.find(target_user) == m_open_chats.end()) {
                ChatWindow* chat_win = new ChatWindow(*m_window, m_username, target_user);
                m_open_chats[target_user] = chat_win;
                
                chat_win->signal_send_message.connect(sigc::mem_fun(*this, &MainWindow::forward_pm_to_server));
                
                chat_win->get_window()->signal_hide().connect([this, target_user]() {
                    delete m_open_chats[target_user];
                    m_open_chats.erase(target_user);
                });
                chat_win->get_window()->present();
            } else {
                m_open_chats[target_user]->get_window()->present();
            }
        }
    }
}

void MainWindow::forward_pm_to_server(const std::string& recipient, const std::string& message) {
    if (m_socket != INVALID_SOCKET) {
        std::string message_to_send = "PM|" + recipient + "|" + message;
        send(m_socket, message_to_send.c_str(), message_to_send.length(), 0);
    }
}

void MainWindow::on_pm_received() {
    std::string sender, content;
    {
        std::lock_guard<std::mutex> lock(m_pm_mutex);
        sender = m_pm_sender_from_thread;
        content = m_pm_content_from_thread;
    }

    if (m_open_chats.count(sender)) {
        m_open_chats[sender]->receive_message(content);
        m_open_chats[sender]->get_window()->present();
    } else {
        ChatWindow* chat_win = new ChatWindow(*m_window, m_username, sender);
        m_open_chats[sender] = chat_win;
        chat_win->signal_send_message.connect(sigc::mem_fun(*this, &MainWindow::forward_pm_to_server));
        chat_win->get_window()->signal_hide().connect([this, sender]() {
            delete m_open_chats[sender];
            m_open_chats.erase(sender);
        });
        chat_win->get_window()->present();
        chat_win->receive_message(content);
    }
}


void MainWindow::receive_server_messages() {
    char buffer[2048];
    while (m_is_running) {
        int bytes_received = recv(m_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            std::string message(buffer, bytes_received);
            
            if (message.rfind("USERLIST|", 0) == 0) {
                 std::lock_guard<std::mutex> lock(m_user_list_mutex);
                 m_user_list_from_thread = message.substr(9);
                 m_user_list_dispatcher.emit();
            } 
            else if (message.rfind("FROM|", 0) == 0) {
                std::stringstream ss(message.substr(5));
                std::string sender, content;
                std::getline(ss, sender, '|');
                std::getline(ss, content);

                std::lock_guard<std::mutex> lock(m_pm_mutex);
                m_pm_sender_from_thread = sender;
                m_pm_content_from_thread = content;
                m_pm_dispatcher.emit(); 
            }

        } else {
            m_info_label->set_text("Sunucu bağlantısı koptu.");
            m_is_running = false;
            break;
        }
    }
}


void MainWindow::on_user_list_update() {
 
    std::string user_list_str;
    {
        std::lock_guard<std::mutex> lock(m_user_list_mutex);
        user_list_str = m_user_list_from_thread;
    }

    m_user_list_box->remove_all();
    std::stringstream ss(user_list_str);
    std::string username;
    while (std::getline(ss, username, ',')) {
        if (!username.empty()) {
            auto row = Gtk::make_managed<Gtk::ListBoxRow>();
            auto label = Gtk::make_managed<Gtk::Label>(username);
            label->set_halign(Gtk::Align::START);
            label->set_margin_start(10);
            row->set_child(*label);

            auto gesture = Gtk::GestureClick::create();
            gesture->signal_pressed().connect(
                [this, row](int n_press, [[maybe_unused]] double x, [[maybe_unused]] double y) {
                    if (n_press == 2) {
                        this->on_user_activated(row);
                    }
                }
            );
            row->add_controller(gesture);
            m_user_list_box->append(*row);
        }
    }
}

void MainWindow::connect_to_server() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        m_info_label->set_text("Sunucuya bağlanılamadı.");
        return;
    }
    m_info_label->set_text("Sunucuya bağlanıldı. Lobi bekleniyor...");
    std::string login_msg = "LOGIN|" + m_username;
    send(m_socket, login_msg.c_str(), login_msg.length(), 0);

    m_is_running = true;
    m_receive_thread = std::thread(&MainWindow::receive_server_messages, this);
}

void MainWindow::get_username() {
    auto* dialog = new Gtk::Dialog("Kullanıcı Adı", *m_window, true);
    auto content_area = dialog->get_content_area();
    content_area->append(*Gtk::make_managed<Gtk::Label>("Lütfen bir kullanıcı adı girin:"));
    auto entry = Gtk::make_managed<Gtk::Entry>();
    dialog->get_content_area()->append(*entry);
    dialog->add_button("Tamam", Gtk::ResponseType::OK);

    dialog->signal_response().connect(
        [this, dialog, entry](int response_id) {
            if (response_id == Gtk::ResponseType::OK && !entry->get_text().empty()) {
                m_username = entry->get_text();
            } else {
                m_username = "Misafir" + std::to_string(rand() % 1000);
            }
            m_window->set_title(m_username + " - Sohbet Lobisi");
            delete dialog;
            std::thread(&MainWindow::connect_to_server, this).detach();
        });
    dialog->show();
}