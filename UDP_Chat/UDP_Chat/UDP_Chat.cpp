#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

#ifndef _DEBUG
namespace 
{
    PCSTR const PORT = "12121";
}
#endif // !_DEBUG

std::mutex mtx;

void Recieve(SOCKET socket)
{
    int len = sizeof(sockaddr);
    unsigned const bufLen = 255;
    char buffer[255];
    sockaddr_in recvSockAddr;
    while (true)
    {
        if (recvfrom(socket, buffer, sizeof(buffer), 0, (sockaddr*)&recvSockAddr, &len) < 0)
        {
            std::cout << "Receiving error: " << WSAGetLastError() << "\n";
            break;
        }
        mtx.lock();

        char buffer2[255];
        std::cout << "\a\nFrom " << inet_ntop(AF_INET, &recvSockAddr.sin_addr, buffer2, 255) << " " << buffer << "\n";
        mtx.unlock();
    }
}

void Send(SOCKET socket, sockaddr_in recvSockAddr)
{
    while (true)
    {
        std::string buffer;
        std::getline(std::cin, buffer);
        if (buffer == "")
        {
            continue;
        }
        if (sendto(socket, buffer.c_str(), buffer.size() + 1, 0, (sockaddr*)&recvSockAddr, sizeof(recvSockAddr)) < 0)
        {
            std::cout << "Sending error: " << WSAGetLastError() << "\n";
            break;
        }
    }
}

int main()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    std::cout << result << std::endl;
    if (result != 0)
    {
        return 1;
    }

    addrinfo* infoResult = nullptr;
    addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    std::string recieverPort;
    std::string port;
#ifdef _DEBUG
    std::cout << "Input your port: ";
    std::cin >> port;
    
    std::cout << "Input reciever port: ";
    std::cin >> recieverPort;
#else
    recieverPort = PORT;
    port = PORT;
#endif // _DEBUG

    // Resolve the local address and port to be used by the server

    result = getaddrinfo(nullptr, port.c_str(), &hints, &infoResult);

    if (result != 0) {
        std::cout << "getaddrinfo failed: " << result << std::endl;
        WSACleanup();
        return 1;
    }
    SOCKET mySocket = INVALID_SOCKET;
    mySocket = socket(infoResult->ai_family, infoResult->ai_socktype, infoResult->ai_protocol);

    if (mySocket == INVALID_SOCKET) {
        std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(infoResult);
        WSACleanup();
        return 1;
    }

    result = bind(mySocket, infoResult->ai_addr, (int)infoResult->ai_addrlen);
    if (result == SOCKET_ERROR) {
        std::cout << "bind failed with error: \n" << WSAGetLastError() << std::endl;
        freeaddrinfo(infoResult);
        closesocket(mySocket);
        WSACleanup();
        return 1;
    }

    char broadcastOption = '1';

    if (setsockopt(mySocket, SOL_SOCKET, SO_BROADCAST, &broadcastOption, sizeof(broadcastOption)) < 0)
    {
        std::cout << "Error in setting Broadcast option: " << WSAGetLastError() << std::endl;
        closesocket(mySocket);
        return 0;
    }

    freeaddrinfo(infoResult);

    sockaddr_in recvSockAddr;
    ZeroMemory(&recvSockAddr, sizeof(recvSockAddr));
    
    recvSockAddr.sin_family = AF_INET;
    recvSockAddr.sin_port = htons(stoul(recieverPort));
    recvSockAddr.sin_addr.s_addr = INADDR_BROADCAST;

#ifdef _DEBUG
    std::cin.ignore();
#endif // _DEBUG

    std::thread sendThred(Send, mySocket, recvSockAddr);
    Recieve(mySocket);

    closesocket(mySocket);
    WSACleanup();
}
