#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <iomanip>
#include <cctype>
#include <unordered_map>

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

std::string caesarEncrypt(const std::string& text, int shift) {
    std::string encryptedText = text;
    for (char &c : encryptedText) {
        // Skip encrypting space and colon
        if (c == ' ' || c == ':') {
            continue;
        }
        if (c >= 32 && c <= 126) {
            c = static_cast<char>(((c - 32 + shift) % 95) + 32);
        }
    }
    return encryptedText;
}

std::string caesarDecrypt(const std::string& text, int shift) {
    std::string decryptedText = text;
    for (char &c : decryptedText) {
        // Skip decrypting space and colon
        if (c == ' ' || c == ':') {
            continue;
        }
        if (c >= 32 && c <= 126) {
            c = static_cast<char>(((c - 32 - shift + 95) % 95) + 32);
        }
    }
    return decryptedText;
}

bool loadCredentials(std::unordered_map<std::string, std::string>& credentials) {
    std::ifstream file("credentials.txt");
    if (!file.is_open()) {
        return false;
    }
    std::string username, password;
    while (file >> username >> password) {
        credentials[username] = password;
    }
    file.close();
    return true;
}

bool saveCredentials(const std::string& username, const std::string& password) {
    std::ofstream file("credentials.txt", std::ios::app);
    if (!file.is_open()) {
        return false;
    }
    file << username << " " << password << std::endl;
    file.close();
    return true;
}

bool registerUser(std::unordered_map<std::string, std::string>& credentials) {
    std::string username, password;
    std::cout << "Enter new username: ";
    std::getline(std::cin, username);
    if (credentials.find(username) != credentials.end()) {
        std::cout << "Username already exists. Try another." << std::endl;
        return false;
    }
    std::cout << "Enter password: ";
    std::getline(std::cin, password);

    credentials[username] = caesarEncrypt(password, 3); // Encrypt password before saving
    return saveCredentials(username, caesarEncrypt(password, 3));
}

bool loginUser(const std::unordered_map<std::string, std::string>& credentials, std::string& username) {
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    std::string password;
    std::cout << "Enter password: ";
    std::getline(std::cin, password);

    auto it = credentials.find(username);
    if (it != credentials.end() && caesarDecrypt(it->second, 3) == password) {
        return true;
    }
    std::cout << "Invalid credentials. Please try again." << std::endl;
    return false;
}

void receiveMessages(int sock) {
    char buffer[1024] = {0};
    const int shift = 3; // Example shift for Caesar cipher
    while (true) {
        memset(buffer, 0, 1024); // Clear buffer at the beginning of the loop
        int valread = read(sock, buffer, 1024);
        if (valread > 0) {
            std::string receivedMsg = caesarDecrypt(std::string(buffer, valread), shift);
            printTimestamp();
            std::cout << RED_TEXT << receivedMsg << RESET_COLOR << std::endl;
        } else {
            std::cerr << RED_TEXT << "Server disconnected or error occurred" << RESET_COLOR << std::endl;
            close(sock);
            exit(0);
        }
    }
}

int main() {
    std::unordered_map<std::string, std::string> credentials;
    if (!loadCredentials(credentials)) {
        std::cout << "Could not load credentials file. New file will be created upon registration." << std::endl;
    }

    std::cout << "Do you want to (1) Register or (2) Login? Enter choice: ";
    std::string choice, username;
    std::getline(std::cin, choice);

    bool isAuthenticated = false;
    if (choice == "1") {
        isAuthenticated = registerUser(credentials);
        if (isAuthenticated) {
            // Assuming that the username is set within registerUser function or another way to retrieve it here
            std::cout << "Please login using your new credentials." << std::endl;
            isAuthenticated = loginUser(credentials, username);  // Modified to pass username
            if (isAuthenticated) {
                std::cout << "Login successful." << std::endl;
            } else {
                std::cout << "Login failed." << std::endl;
                return -1;
            }
        }
    } else if (choice == "2") {
        isAuthenticated = loginUser(credentials, username);  // Corrected to include the username parameter
    } else {
        std::cout << "Invalid choice. Exiting program." << std::endl;
        return -1;
    }

    if (!isAuthenticated) {
        return -1;
    }

    std::cout << "Attempting to connect to the server..." << std::endl;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << RED_TEXT << "Socket creation error" << RESET_COLOR << std::endl;
        return -1;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "192.168.1.38", &serv_addr.sin_addr) <= 0) {
        std::cerr << RED_TEXT << "Invalid address / Address not supported" << RESET_COLOR << std::endl;
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << RED_TEXT << "Connection Failed. Please check the server status and try again." << RESET_COLOR << std::endl;
        close(sock);
        return -1;
    }

    std::cout << GREEN_TEXT << "Connected successfully. Welcome, " << username << RESET_COLOR << std::endl;
    std::thread receiverThread(receiveMessages, sock);
    receiverThread.detach();

    const int shift = 3; // Example shift for Caesar cipher
    std::string message;
    send(sock, caesarEncrypt(username, shift).c_str(), username.length(), 0);

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
