#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>


#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    int bytes_received;
    
 
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Ошибка инициализации WinSock\n");
        return 1;
    }
    

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Ошибка создания сокета: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    

    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Ошибка подключения: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    
    printf("Успешно подключились к серверу!\n");
    

    char *message = "Hello World от C клиента!";
    if (send(sock, message, strlen(message), 0) == SOCKET_ERROR) {
        printf("Ошибка отправки: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("Сообщение отправлено: %s\n", message);
    

    bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Ответ от сервера: %s\n", buffer);
    } else if (bytes_received == 0) {
        printf("Соединение закрыто сервером\n");
    } else {
        printf("Ошибка получения данных: %d\n", WSAGetLastError());
    }
    

    closesocket(sock);
    WSACleanup();
    
    printf("Клиент завершил работу\n");
    system("pause");
    return 0;
}