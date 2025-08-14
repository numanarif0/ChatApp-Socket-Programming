#include "ChatWindow.hpp"
#include <iostream>
#include <stdexcept>

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
    
    m_status_label->set_text(m_target_username + " ile konuşuyorsunuz.");
}

ChatWindow::~ChatWindow() {
    std::cout << m_target_username << " ile sohbet penceresi yok edildi." << std::endl;
}

Gtk::Window* ChatWindow::get_window() {
    return m_window;
}

void ChatWindow::on_send_button_clicked() {
    std::string message_text = m_message_input->get_text();
    if (!message_text.empty()) {
        append_message(message_text, true); 
        signal_send_message.emit(m_target_username, message_text);
        m_message_input->set_text("");
    }
}

void ChatWindow::receive_message(const std::string& message) {
    append_message(message, false); 
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