#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>  // For std::remove_if and std::for_each

const int PORT = 8080;
const int MAX_CLIENTS = 10;
std::mutex clientListMutex;

struct ClientInfo {
    int socket;
    std::string username;
};

std::vector<ClientInfo> clients;

void broadcastMessage(const std::string& message, int senderSocket) {
    std::lock_guard<std::mutex> lock(clientListMutex);
    for (const auto& client : clients) {
        if (client.socket != senderSocket) { // Do not send the message back to the sender
            send(client.socket, message.c_str(), message.length(), 0);
        }
    }
}

void handleClient(int clientSocket) {
    char buffer[1024] = {0};
    int valread = read(clientSocket, buffer, 1024);
    if (valread <= 0) {
        std::cerr << "Failed to read username or client disconnected" << std::endl;
        close(clientSocket);
        return;
    }
    std::string username = std::string(buffer, valread);

    {
        std::lock_guard<std::mutex> lock(clientListMutex);
        clients.push_back({clientSocket, username});
    }

    std::string welcomeMessage = username + " has joined the chat\n";
    broadcastMessage(welcomeMessage, clientSocket); // Notify others that a new client has joined
    std::cout << "New client connected: " << username << std::endl;

    while((valread = read(clientSocket, buffer, 1024)) > 0) {
        std::string message = username + ": " + std::string(buffer, valread) + "\n";
        std::cout << message;
        broadcastMessage(message, clientSocket); // Send received message to all clients except the sender
    }

    std::string disconnectMessage = username + " has left the chat\n";
    broadcastMessage(disconnectMessage, clientSocket); // Notify others that a client has disconnected
    std::cout << "Client disconnected: " << username << std::endl;

    {
        std::lock_guard<std::mutex> lock(clientListMutex);
        clients.erase(std::remove_if(clients.begin(), clients.end(), 
                                     [clientSocket](const ClientInfo& ci) { return ci.socket == clientSocket; }),
                       clients.end());
    }

    close(clientSocket);
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(serverSocket);
        return -1;
    }

    if (listen(serverSocket, MAX_CLIENTS) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(serverSocket);
        return -1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;
    sockaddr_in clientAddr;
    socklen_t addrlen = sizeof(clientAddr);

    while(true) {
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrlen);
        if (clientSocket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}
