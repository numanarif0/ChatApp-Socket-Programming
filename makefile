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
	@echo "İstemci uygulaması '$(CLIENT_TARGET)' başarıyla oluşturuldu."


$(SERVER_TARGET): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Sunucu uygulaması '$(SERVER_TARGET)' başarıyla oluşturuldu."

clean:
	@echo "Oluşturulan dosyalar temizleniyor..."
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET) *.o
	@echo "Temizlik tamamlandı."