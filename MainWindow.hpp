#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <gtkmm.h>
#include <winsock2.h>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include "ChatWindow.hpp"

class MainWindow {
public:
    MainWindow();
    virtual ~MainWindow();

    Gtk::ApplicationWindow* get_window();

private:
    void get_username();
    void connect_to_server();
    void receive_server_messages();

    void on_user_list_update(const std::string& user_list_str);
    void on_user_activated(Gtk::ListBoxRow* row);
    
    Glib::RefPtr<Gtk::ApplicationWindow> m_window;
    Gtk::Label* m_info_label = nullptr;
    Gtk::ListBox* m_user_list_box = nullptr;

    SOCKET m_socket = INVALID_SOCKET;
    std::string m_username;
    std::thread m_receive_thread;
    std::atomic<bool> m_is_running{true};
    
    Glib::Dispatcher m_user_list_dispatcher;
    std::string m_user_list_from_thread;
    std::mutex m_user_list_mutex;

    std::map<std::string, ChatWindow*> m_open_chats;
};

#endif 