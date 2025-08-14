// MainWindow.hpp

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

    void on_user_list_update();
    void on_user_activated(Gtk::ListBoxRow* row);
    void forward_pm_to_server(const std::string& recipient, const std::string& message);
    
    void on_pm_received();

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

    Glib::Dispatcher m_pm_dispatcher;
    std::string m_pm_sender_from_thread;
    std::string m_pm_content_from_thread;
    std::mutex m_pm_mutex;

    std::map<std::string, ChatWindow*> m_open_chats;
};

#endif