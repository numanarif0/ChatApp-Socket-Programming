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

    m_user_list_dispatcher.connect([this]() {
        std::string user_list;
        {
            std::lock_guard<std::mutex> lock(m_user_list_mutex);
            user_list = m_user_list_from_thread;
        }
        on_user_list_update(user_list);
    });

    m_user_list_box->signal_row_activated().connect(sigc::mem_fun(*this, &MainWindow::on_user_activated));
    
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
    std::cout << "on_user_activated fonksiyonu tetiklendi!" << std::endl;
    auto label = dynamic_cast<Gtk::Label*>(row->get_child());
    if (label) {
        std::string target_user = label->get_text();
        std::cout << "Hedef kullanıcı: " << target_user << std::endl;

        if (target_user != m_username) { 
            if (m_open_chats.find(target_user) == m_open_chats.end()) {
                std::cout << "'" << target_user << "' için yeni sohbet penceresi oluşturuluyor..." << std::endl;
                
                try {
                    ChatWindow* chat_win = new ChatWindow(*m_window, m_username, target_user);
                    m_open_chats[target_user] = chat_win;
                    chat_win->get_window()->signal_hide().connect([this, target_user]() {
                        std::cout << "'" << target_user << "' ile olan sohbet kapatıldı ve temizlendi." << std::endl;
                        delete m_open_chats[target_user];
                        m_open_chats.erase(target_user);
                    });
                    chat_win->get_window()->present();
                } catch (const std::exception& e) {
                    std::cerr << "!!! ChatWindow oluşturulurken KRİTİK HATA: " << e.what() << std::endl;
                }

            } else {
                std::cout << "'" << target_user << "' ile zaten açık bir sohbet var." << std::endl;
            }
        }
    }
}


void MainWindow::on_user_list_update(const std::string& user_list_str) {
    auto children = m_user_list_box->get_children();
    for (auto child : children) {
        m_user_list_box->remove(*child);
    }

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
                [this, row](int n_press, double x, double y) {
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
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        m_info_label->set_text("WSAStartup hatası.");
        return;
    }

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        m_info_label->set_text("Socket oluşturulamadı.");
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        m_info_label->set_text("Sunucuya bağlanılamadı.");
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return;
    }

    m_info_label->set_text("Sunucuya bağlanıldı. Lobi bekleniyor...");

    std::string login_msg = "LOGIN|" + m_username;
    send(m_socket, login_msg.c_str(), login_msg.length(), 0);

    m_is_running = true;
    m_receive_thread = std::thread(&MainWindow::receive_server_messages, this);
}

void MainWindow::receive_server_messages() {
    char buffer[2048];
    int bytesReceived;
    while (m_is_running) {
        bytesReceived = recv(m_socket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            std::string message(buffer, bytesReceived);
            
            if (message.rfind("USERLIST|", 0) == 0) {
                 std::lock_guard<std::mutex> lock(m_user_list_mutex);
                 m_user_list_from_thread = message.substr(9); 
                 m_user_list_dispatcher.emit();
            } else {
            }

        } else {
           
            m_info_label->set_text("Sunucu bağlantısı koptu.");
            m_is_running = false;
            break;
        }
    }
}

void MainWindow::get_username() {

    auto* dialog = new Gtk::Dialog("Kullanıcı Adı", *m_window, true);
    
    auto content_area = dialog->get_content_area();
    content_area->set_spacing(12);
    content_area->append(*Gtk::make_managed<Gtk::Label>("Lütfen bir kullanıcı adı girin:"));
    
    auto entry = Gtk::make_managed<Gtk::Entry>();
    entry->set_placeholder_text("Kullanıcı Adı");
    content_area->append(*entry);

    dialog->add_button("Tamam", Gtk::ResponseType::OK);
    dialog->add_button("İptal", Gtk::ResponseType::CANCEL);
    dialog->set_default_response(Gtk::ResponseType::OK);
    dialog->set_response_sensitive(Gtk::ResponseType::OK, false);

    entry->signal_changed().connect([dialog, entry]() {
        dialog->set_response_sensitive(Gtk::ResponseType::OK, !entry->get_text().empty());
    });
    
    dialog->signal_response().connect(
        [this, dialog, entry](int response_id) {
            if (response_id == Gtk::ResponseType::OK) {
                m_username = entry->get_text();
            } else {
                m_username = "Misafir" + std::to_string(rand() % 1000);
            }
            m_window->set_title(m_username + " - Sohbet Lobisi");
            
            dialog->hide();

            delete dialog;

           
            std::thread(&MainWindow::connect_to_server, this).detach();
        });

    dialog->show();
}