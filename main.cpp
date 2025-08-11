#include "MainWindow.hpp"
#include <gtkmm/application.h>

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("org.gtkmm.chatapp");

 
    app->signal_activate().connect([&]() {
   
        auto controller = new MainWindow();
        
   
        app->add_window(*controller->get_window());

    
        controller->get_window()->present();
    });


    return app->run(argc, argv);
}