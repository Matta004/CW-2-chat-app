#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

const int PORT = 8080;

// ANSI Escape Codes for colors
const std::string GREEN = "\033[32m";  // Green text
const std::string YELLOW = "\033[33m"; // Yellow text
const std::string RESET = "\033[0m";   // Reset to default terminal color

void receiveMessages(int sock) {
    char buffer[1024] = {0};
    int valread;
    while((valread = read(sock, buffer, 1024)) > 0) {
        std::cout << GREEN << std::string(buffer, valread) << RESET << std::endl;
        memset(buffer, 0, 1024);
    }
    if (valread == 0 || valread == -1) {
        std::cerr << "Server disconnected" << std::endl;
        close(sock);
        exit(0);
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    std::thread receiverThread(receiveMessages, sock);
    receiverThread.detach();

    std::string message;
    std::cout << YELLOW << "Enter username: " << RESET;
    std::getline(std::cin, message);
    send(sock, message.c_str(), message.length(), 0);

    while (true) {
        std::getline(std::cin, message);
        if (message == "quit") {
            break;
        }
        std::cout << YELLOW << "You: " << message << RESET << std::endl;  // Displaying sent messages in yellow
        send(sock, message.c_str(), message.length(), 0);
    }

    close(sock);
    return 0;
}
