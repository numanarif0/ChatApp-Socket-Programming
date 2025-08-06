CXX = g++


CXXFLAGS = -Wall -g
LDFLAGS = -lws2_32


CLIENT_TARGET = client.exe
SERVER_TARGET = server.exe


CLIENT_SRC = client.cpp
SERVER_SRC = server.cpp


.PHONY: all clean


all: $(CLIENT_TARGET) $(SERVER_TARGET)


$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Istemci uygulamasi '$(CLIENT_TARGET)' basariyla olusturuldu."


$(SERVER_TARGET): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Sunucu uygulamasi '$(SERVER_TARGET)' basariyla olusturuldu."

clean:
	@echo "Olusturulan dosyalar temizleniyor..."
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET) *.o
	@echo "Temizlik tamamlandi."