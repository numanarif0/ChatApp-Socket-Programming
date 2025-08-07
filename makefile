CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra `pkg-config --cflags gtkmm-4.0`
LIBS = `pkg-config --libs gtkmm-4.0` -lws2_32

LIBS_SERVER = -lws2_32

SOURCES = main.cpp ChatWindow.cpp 
OBJECTS = $(SOURCES:.cpp=.o)

SOURCES_SERVER = server.cpp
OBJECTS_SERVER = $(SOURCES_SERVER:.cpp=.o)

SERVER_EXACUTABLE = server.exe
EXECUTABLE = chatapp.exe


all: $(EXECUTABLE) $(SERVER_EXACUTABLE)


$(EXECUTABLE): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LIBS)

$(SERVER_EXACUTABLE): $(OBJECTS_SERVER)
	$(CXX) -o $@ $(OBJECTS_SERVER) $(LIBS_SERVER)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
	rm -f $(OBJECTS_SERVER) $(SERVER_EXACUTABLE)

.PHONY: all clean