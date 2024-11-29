#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const char IP_SERV[] = "127.0.0.1";
const int PORT_NUM = 1234;
const short BUFF_SIZE = 1024;

vector<SOCKET> clients;
mutex clients_mutex;
mutex file_mutex;

void write_to_csv(const string& client_ip, const string& timestamp, const string& metric_name, const string& value) {
    lock_guard<mutex> lock(file_mutex);
    ofstream csv_file("metrics.csv", ios::app);

    if (csv_file.is_open()) {
        csv_file << client_ip << "," << timestamp << "," << metric_name << "," << value << "\n";
    }
    csv_file.close();
}

void handle_clients(SOCKET ClientConn, string client_ip) {
    vector<string> metric_buffer;
    vector <char> buffer(BUFF_SIZE);
    short packet_size = 0;

    while (true) {
        packet_size = recv(ClientConn, buffer.data(), buffer.size() - 1, 0);

        if (packet_size == SOCKET_ERROR || packet_size == 0) {
            cout << "Client disconenected or error occurred." << endl;
            break;
        }

        buffer[packet_size] = '\0';

        string message(buffer.data());
        cout << "Message from client " << client_ip << ": " << message << endl;

        stringstream ss(message);
        string metric_name, value;
        getline(ss, metric_name, ',');
        getline(ss, value, ',');

        auto now = chrono::system_clock::now();
        time_t now_c = chrono::system_clock::to_time_t(now);
        tm local_time;
        localtime_s(&local_time, &now_c);
        stringstream timestamp;
        timestamp << put_time(&local_time, "%H:%M:%S");

        write_to_csv(client_ip, timestamp.str(), metric_name, value);

        metric_buffer.push_back(message);

        if (metric_buffer.size() == 3) {
            string notification = "|" + timestamp.str() + "| Received 3 metrics: ";
            for (const auto& metric : metric_buffer) {
                notification += metric + "; ";
            }
            notification += "All data recorded in csv.\0";

            send(ClientConn, notification.c_str(), notification.size(), 0);
            metric_buffer.clear();
        }

        if (message == "exit") {
            break;
        }
    }

    clients_mutex.lock();
    clients.erase(remove(clients.begin(), clients.end(), ClientConn), clients.end());
    clients_mutex.unlock();
    closesocket(ClientConn);
}

int main()
{
    int erStat;

    // IP in string format to numeric format
    in_addr ip_to_num;
    erStat = inet_pton(AF_INET, IP_SERV, &ip_to_num);

    if (erStat <= 0) {

        cout <<
            "Error in IP translation to special numeric format"
        << endl;

        return 1;
    }

    // WinSock initialization
    WSADATA wsData;

    erStat = WSAStartup(MAKEWORD(2, 2), &wsData);

    if (erStat != 0) {

        cout << "Error WinSock version initialization # ";
        cout << WSAGetLastError() << endl;
        return 1;
    }
    else {
        cout << "WinSock initialization is OK" << endl;
    }

    // Server socket initialization
    SOCKET ServSock = socket(AF_INET, SOCK_STREAM, 0);

    if (ServSock == INVALID_SOCKET) {

        cout << "Error initialization socket # "
        << WSAGetLastError() << endl;

        closesocket(ServSock);
        WSACleanup();
        return 1;
    }
    else {

        cout << "Server socket initialization is OK\n";
    }

    // Server socket binding
    sockaddr_in servInfo;
    ZeroMemory(&servInfo, sizeof(servInfo));

    servInfo.sin_family = AF_INET;
    servInfo.sin_addr = ip_to_num;
    servInfo.sin_port = htons(1234);

    erStat = bind(ServSock, (sockaddr*)&servInfo, sizeof(servInfo));
    if (erStat != 0) {

        cout << "Error Socket binding to server info. Error # "
        << WSAGetLastError() << endl;

        closesocket(ServSock);
        WSACleanup();
        return 1;
    }
    else {

        cout << "Binding socket to server info is OK" << endl;
    }

    // Starting to listen to any Clients
    erStat = listen(ServSock, SOMAXCONN);
    if (erStat != 0) {

        cout <<
        "Can't start to listen to. Error # "
        << WSAGetLastError() << endl;

        closesocket(ServSock);
        WSACleanup();
        return 1;
    }
    else {

        cout << "Listening ..." << endl;
    }

    while (true) {
        sockaddr_in clientInfo;
        ZeroMemory(&clientInfo, sizeof(clientInfo));
        int clientInfo_size = sizeof(clientInfo);

        SOCKET ClientConn = accept(ServSock, (sockaddr*)&clientInfo, &clientInfo_size);
        if (ClientConn == INVALID_SOCKET) {
            cout << "Client detected, but can't connect. Error # ";
            cout << WSAGetLastError << endl;
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientInfo.sin_addr, clientIP, INET_ADDRSTRLEN);
        cout << "Client connected with IP address : ";
        cout << clientIP << endl;

        clients_mutex.lock();
        clients.push_back(ClientConn);
        clients_mutex.unlock();

        thread clientThread(handle_clients, ClientConn, string(clientIP));
        clientThread.detach();
    }

    closesocket(ServSock);
    WSACleanup();
    return 0;
}
