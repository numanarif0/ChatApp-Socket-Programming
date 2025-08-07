// ChatWindow.hpp

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
    ChatWindow();
    virtual ~ChatWindow();

 
    Gtk::ApplicationWindow* get_window();

protected:
    void on_send_button_clicked();
    void on_message_received();
    void connect_to_server();
    void receive_messages();

   
    Glib::RefPtr<Gtk::ApplicationWindow> m_window;
    
 
    Gtk::Label* m_Information = nullptr;
    Gtk::Button* m_SendMessage = nullptr;
    Gtk::TextView* m_chat_history_view = nullptr;
    Gtk::Entry* m_input_messages = nullptr;
    Glib::RefPtr<Gtk::TextBuffer> m_chat_buffer;


    SOCKET m_socket = INVALID_SOCKET;
    std::thread m_receive_thread;
    Glib::Dispatcher m_dispatcher;
    std::atomic<bool> m_is_running;

    std::string m_message_from_thread;
    std::mutex m_mutex;
};

#endif