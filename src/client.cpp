#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <vector>
#include <thread>
#include <random>
#include <chrono>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const char SERVER_IP[] = "127.0.0.1";
const short SERVER_PORT_NUM = 1234;
const short BUFF_SIZE = 1024;

void sendMetrics(SOCKET ClientSock) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> mouseUsageDist(0, 60);
    uniform_int_distribution<> hotkeyUsageDist(0, 10);

    while (true) {
        string cpuLoad = "CPU," + to_string(rand() % 100);
        string mouseUsage = "Mouse," + to_string(mouseUsageDist(gen));
        string hotkeyUsage = "Hotkeys," + to_string(hotkeyUsageDist(gen));

        vector<string> metrics = {cpuLoad, mouseUsage, hotkeyUsage};
        for (const auto& metric : metrics) {
            send(ClientSock, metric.c_str(), metric.size(), 0);
            this_thread::sleep_for(chrono::milliseconds(200));
        }

        this_thread::sleep_for(chrono::seconds(10));
    }
}

void receiveMessages(SOCKET ClientSock) {

    vector <char> buffer(BUFF_SIZE);
    short packet_size = 0;

    while (true) {
        packet_size = recv(ClientSock, buffer.data(), buffer.size() - 1, 0);

        if (packet_size == SOCKET_ERROR || packet_size == 0) {
            cout << "Connection closed or error receiving message." << endl;
            break;
        }

        buffer[packet_size] = '\0';
        cout << "\n[SERVER]: " << buffer.data() << endl;
    }

    closesocket(ClientSock);
    WSACleanup();
    exit(0);
}

int main () {

    int erStat;

    in_addr ip_to_num;
    erStat = inet_pton(AF_INET, SERVER_IP, &ip_to_num);

    if (erStat <= 0) {

        cout << "Error in IP translation to special numeric format\n";
        return 1;
    }

    //WinSock initialization
    WSADATA wsData;

    erStat = WSAStartup(MAKEWORD(2, 2), &wsData);

    if (erStat != 0) {

        cout << "Error WinSock version initializaion #";
        cout << WSAGetLastError() << endl;
        return 1;
    }
    else {

        cout << "WinSock initializaion is OK" << endl;
    }

    //Socket initialization
    SOCKET ClientSock = socket(AF_INET, SOCK_STREAM, 0);

    if (ClientSock == INVALID_SOCKET) {

        cout << "Error initialization socket #";
        cout << WSAGetLastError() << endl;
        closesocket(ClientSock);
        WSACleanup();
    }
    else {

        cout << "Client socket initialization is OK" << endl;
    }

    //Establishing a connection to Server
    sockaddr_in servInfo;

    ZeroMemory(&servInfo, sizeof(servInfo));

    servInfo.sin_family = AF_INET;
    servInfo.sin_addr = ip_to_num;
    servInfo.sin_port = htons(SERVER_PORT_NUM);

    erStat = connect(ClientSock, (sockaddr*)&servInfo, sizeof(servInfo));

    if (erStat != 0) {

        cout << "Connection to server is FALIED. Error #";
        cout << WSAGetLastError() << endl;
        closesocket(ClientSock);
        WSACleanup();
        return 1;
    }
    else {

        cout << "Connection established SUCCESSFULY. ";
        cout << "Ready to send a message to Server" << endl;
    }

    thread receiveThread(receiveMessages, ClientSock);
    thread metricsThread(sendMetrics, ClientSock);

    receiveThread.join();
    metricsThread.join();

    closesocket(ClientSock);
    WSACleanup();
    return 0;
}
