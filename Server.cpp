#include <iostream>
#include <string>
#include <thread>
#include <list>
#include <mutex>
#include <atomic>
#include <csignal>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

const int PORT = 8080;
const int MAX_CLIENTS = 10;
std::mutex clientListMutex;
std::atomic<bool> running(true);

struct ClientInfo {
    SOCKET socket;
    std::string username;
};

std::list<ClientInfo> clients;

void broadcastMessage(const std::string& message, SOCKET senderSocket) {
    std::lock_guard<std::mutex> lock(clientListMutex);
    for (auto it = clients.begin(); it != clients.end(); ) {
        if (it->socket != senderSocket) {
            int sendResult = send(it->socket, message.c_str(), message.length(), 0);
            if (sendResult == SOCKET_ERROR) {
                std::cerr << "Failed to send message to " << it->username << std::endl;
                closesocket(it->socket);
                it = clients.erase(it);  // Remove client from the list if send fails
            }
            else {
                ++it;
            }
        }
        else {
            ++it;
        }
    }
}

void removeClient(SOCKET clientSocket) {
    std::lock_guard<std::mutex> lock(clientListMutex);
    clients.remove_if([clientSocket](const ClientInfo& client) {
        return client.socket == clientSocket;
        });
}

void handleClient(SOCKET clientSocket) {
    char buffer[1024] = { 0 };
    int valread = recv(clientSocket, buffer, 1024, 0);
    if (valread <= 0) {
        std::cerr << "Failed to read from client or client disconnected" << std::endl;
        closesocket(clientSocket);
        return;
    }
    std::string username = std::string(buffer, valread);
    clients.push_back({ clientSocket, username });
    std::string welcomeMessage = username + " kdv mrlqhg wkh fkdw\n";
    broadcastMessage(welcomeMessage, clientSocket);
    std::cout << "New client connected: " << username << std::endl;

    while ((valread = recv(clientSocket, buffer, 1024, 0)) > 0 && running) {
        std::string message = username + ": " + std::string(buffer, valread) + "\n";
        std::cout << message;
        broadcastMessage(message, clientSocket);
    }

    std::string disconnectMessage = username + " kdv ohiw wkh fkdw\n";
    broadcastMessage(disconnectMessage, clientSocket);
    std::cout << "Client disconnected: " << username << std::endl;

    removeClient(clientSocket);
    closesocket(clientSocket);
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        running = false;
    }
}

int main() {
    signal(SIGINT, signalHandler);
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
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
    address.sin_addr.s_addr = INADDR_ANY;
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

    while (running) {
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

    for (const auto& client : clients) {
        closesocket(client.socket);
    }

    closesocket(serverSocket);
    WSACleanup();
    std::cout << "Server shutdown successfully." << std::endl;
    return 0;
}