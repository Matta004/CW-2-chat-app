#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <iomanip>
#include <cctype>

const int PORT = 8080;

// ANSI escape codes for setting text color
const std::string RED_TEXT = "\033[31m";   // Receiver messages in red
const std::string GREEN_TEXT = "\033[32m"; // Sender messages in green
const std::string RESET_COLOR = "\033[0m"; // Reset to default terminal color

void printTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::cout << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << " ";
}

// Caesar Cipher Encryption that includes numbers and special characters
std::string caesarEncrypt(const std::string& text, int shift) {
    std::string encryptedText = text;
    for (char &c : encryptedText) {
        if (c >= 32 && c <= 126) { // Only encrypt printable characters
            c = static_cast<char>(((c - 32 + shift) % 95) + 32);
        }
    }
    return encryptedText;
}

// Caesar Cipher Decryption that includes numbers and special characters
std::string caesarDecrypt(const std::string& text, int shift) {
    std::string decryptedText = text;
    for (char &c : decryptedText) {
        if (c >= 32 && c <= 126) { // Only decrypt printable characters
            c = static_cast<char>(((c - 32 - shift + 95) % 95) + 32);
        }
    }
    return decryptedText;
}

void receiveMessages(int sock) {
    char buffer[1024] = {0};
    const int shift = 3; // Example shift for Caesar cipher
    while (true) {
        int valread = read(sock, buffer, 1024);
        if (valread > 0) {
            std::string receivedMsg = caesarDecrypt(std::string(buffer, valread), shift);
            if (receivedMsg.find("status_update:") == 0) {
                std::cout << RED_TEXT << receivedMsg.substr(14) << RESET_COLOR << std::endl;
            } else {
                printTimestamp();
                std::cout << RED_TEXT << receivedMsg << RESET_COLOR << std::endl;
            }
            memset(buffer, 0, 1024);
        } else {
            std::cerr << RED_TEXT << "Server disconnected or error occurred" << RESET_COLOR << std::endl;
            close(sock);
            exit(0);
        }
    }
}

int main() {
    std::cout << "Attempting to connect to the server..." << std::endl;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << RED_TEXT << "Socket creation error" << RESET_COLOR << std::endl;
        return -1;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "192.168.176.153", &serv_addr.sin_addr) <= 0) {
        std::cerr << RED_TEXT << "Invalid address / Address not supported" << RESET_COLOR << std::endl;
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << RED_TEXT << "Connection Failed. Please check the server status and try again." << RESET_COLOR << std::endl;
        close(sock);
        return -1;
    }

    std::cout << GREEN_TEXT << "Connected successfully. Please enter your username:" << RESET_COLOR << std::endl;
    std::thread receiverThread(receiveMessages, sock);
    receiverThread.detach();

    const int shift = 3; // Example shift for Caesar cipher
    std::string message;
    std::getline(std::cin, message);  // User types their username first
    send(sock, caesarEncrypt(message, shift).c_str(), message.length(), 0);

    while (std::getline(std::cin, message)) {
        if (message == "quit") {
            send(sock, caesarEncrypt("has left the chat.", shift).c_str(), 18, 0);
            break;
        } else if (message == "clear") {
            std::cout << "\033[2J\033[1;1H"; // Clear console screen
        } else {
            std::cout << "\033[K";
            std::cout << "\033[F";
            printTimestamp(); // Add timestamp
            std::cout << GREEN_TEXT << "you: "<< message << RESET_COLOR << std::endl; // Print the message in green
            send(sock, caesarEncrypt(message, shift).c_str(), message.length(), 0); // Encrypt and send the message
        }
    }
    close(sock);
    std::cout << GREEN_TEXT << "You have disconnected from the server." << RESET_COLOR << std::endl;
    return 0;
}
