#include <iostream>
#include <string>
#include <thread>
#include <list>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

const int PORT = 8080;
const int MAX_CLIENTS = 10;
std::mutex clientListMutex;

struct ClientInfo {
    SOCKET socket;
    std::string username;
};

std::list<ClientInfo> clients;

void broadcastMessage(const std::string& message, SOCKET senderSocket) {
    std::lock_guard<std::mutex> lock(clientListMutex);
    for (const auto& client : clients) {
        if (client.socket != senderSocket) {
            send(client.socket, message.c_str(), message.length(), 0);
        }
    }
}

void handleClient(SOCKET clientSocket) {
    char buffer[1024] = {0};
    int valread = recv(clientSocket, buffer, 1024, 0);
    if (valread <= 0) {
        std::cerr << "Failed to read from client or client disconnected" << std::endl;
        closesocket(clientSocket);
        return;
    }
    std::string username = std::string(buffer, valread);

    std::string welcomeMessage = username + " has joined the chat\n";
    broadcastMessage(welcomeMessage, clientSocket); // Notify others that a new client has joined
    std::cout << "New client connected: " << username << std::endl;

    while((valread = recv(clientSocket, buffer, 1024, 0)) > 0) {
        std::string message = username + ": " + std::string(buffer, valread) + "\n";
        std::cout << message;
        broadcastMessage(message, clientSocket); // Send received message to all clients except the sender
    }

    std::string disconnectMessage = username + " has left the chat\n";
    broadcastMessage(disconnectMessage, clientSocket); // Notify others that a client has disconnected
    std::cout << "Client disconnected: " << username << std::endl;

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return -1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation error" << std::endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Bind to any address
    address.sin_port = htons(PORT);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    if (listen(serverSocket, MAX_CLIENTS) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        int addrlen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrlen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}