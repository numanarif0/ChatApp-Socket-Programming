#ifndef CHAT_WINDOW_HPP
#define CHAT_WINDOW_HPP

#include <gtkmm.h>
#include <winsock2.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

class ChatWindow {
public:
    
    ChatWindow(Gtk::Window& parent, const std::string& my_username, const std::string& target_username);
    virtual ~ChatWindow();

    Gtk::Window* get_window();

private:
    void on_send_button_clicked();
    void append_message(const std::string& message, bool is_mine);
    
    void connection_manager();
    void receive_messages();

    void on_message_received();
    void on_status_update();

    Glib::RefPtr<Gtk::Builder> m_builder;
    Gtk::Window* m_window = nullptr;
    Gtk::Label* m_status_label = nullptr;
    Gtk::Button* m_send_button = nullptr;
    Gtk::TextView* m_chat_history_view = nullptr;
    Gtk::Entry* m_message_input = nullptr;
    Glib::RefPtr<Gtk::TextBuffer> m_chat_buffer;


    SOCKET m_socket = INVALID_SOCKET;
    std::string m_my_username;
    std::string m_target_username;
    
    std::thread m_connection_thread;
    std::thread m_receive_thread;
    std::atomic<bool> m_is_running{true};

    Glib::Dispatcher m_dispatcher;
    std::string m_message_from_thread;
    std::mutex m_mutex;

    Glib::Dispatcher m_status_dispatcher;
    std::string m_status_message;
    std::mutex m_status_mutex;
};

#endif 