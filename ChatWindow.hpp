#ifndef CHAT_WINDOW_HPP
#define CHAT_WINDOW_HPP

#include <gtkmm.h>
#include <string>



class ChatWindow {
public:

    sigc::signal<void(const std::string&, const std::string&)> signal_send_message;
    
    ChatWindow(Gtk::Window& parent, const std::string& my_username, const std::string& target_username);
    virtual ~ChatWindow();

    Gtk::Window* get_window();
    void receive_message(const std::string& message);

private:
    void on_send_button_clicked();
    void append_message(const std::string& message, bool is_mine);
    

    Glib::RefPtr<Gtk::Builder> m_builder;
    Gtk::Window* m_window = nullptr;
    Gtk::Label* m_status_label = nullptr;
    Gtk::Button* m_send_button = nullptr;
    Gtk::TextView* m_chat_history_view = nullptr;
    Gtk::Entry* m_message_input = nullptr;
    Glib::RefPtr<Gtk::TextBuffer> m_chat_buffer;

    std::string m_my_username;
    std::string m_target_username;
};

#endif