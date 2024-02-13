// Пример простого TCP клиента
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib") 
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ctime>
#include <conio.h>
#include <vector>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
using namespace std;

#define PORT 667

const int width = 50, height = 30; // размеры поля, по которому бегает змейка
int sleep_time = 10;
void DrawField(char* buff);
HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); // создание хендла потока вывода
COORD c; // объект для хранения координат
SOCKET my_sock;
char* gateway;
int endsX[4] = {1,1,1,1};
int endsY[4] = {1,1,1,1};
bool isFrame = false;

void DrawFrame() 
{
    isFrame = true;
    system("cls");
    c.X = 0;
    c.Y = 0;
    SetConsoleCursorPosition(h, c); // отправляем курсор в левый верхний угол
    for (int y = 0; y < height; y++) // стандартный двойной цикл на отрисовку рамки
    {
        for (int x = 0; x < width; x++)
        {
            SetConsoleTextAttribute(h, 4); // установка цвета, которым рисуется рамка поля
            char s;
            if (x == 0 && y == 0) // в верхнем левом углу поля - символ соответствующего угла
                s = 218;
            else if (x == 0 && y == height - 1) // нижний левый угол
                s = 192;
            else if (y == 0 && x == width - 1) // верхний правый угол
                s = 191;
            else if (y == height - 1 && x == width - 1) // нижний правый угол
                s = 217;
            else if (y == 0 || y == height - 1) // верхняя и нижняя граница поля
                s = 196;
            else if (x == 0 || x == width - 1) // левая и правая граница поля
                s = 179;
            else s = ' '; // во всех остальных случаях должен быть просто пробел (означает пустую область поля)
            putchar(s); // выводим символ
        }
        cout << endl;
    }
    printf("Score:");
}

void DrawField(char* buff) {
    int buffIndex = 1;
    for (int snake = 0; snake < 4; snake++) 
    {
        if (buff[3] == 0)
        {
            break;
        }
        c.X = endsX[snake];
        c.Y = endsY[snake];
        SetConsoleCursorPosition(h, c);
        putchar(' '); // стираем следы от змеек

        int length = buff[buffIndex++];
        int color = buff[buffIndex++];
        SetConsoleTextAttribute(h, color); // установка цвета, которым рисуется рамка поля
        endsX[snake] = buff[buffIndex];
        endsY[snake] = buff[buffIndex + 1];
        for (int i = 0; i < length - 1; i++) // рисуем сегменты
        {
            c.X = buff[buffIndex++];
            c.Y = buff[buffIndex++];
            SetConsoleCursorPosition(h, c); 
            putchar('*');
        }

        c.X = buff[buffIndex++];
        c.Y = buff[buffIndex++];
        SetConsoleCursorPosition(h, c); // рисуем голову
        putchar((char)1);

        c.X = buff[buffIndex++];
        c.Y = buff[buffIndex++];
        SetConsoleTextAttribute(h, 12); // рисуем яблоко
        SetConsoleCursorPosition(h, c); 
        putchar('o');

        int namelength = buff[buffIndex++];
        c.X = 0;
        c.Y = 31 + snake;
        SetConsoleCursorPosition(h, c);
        SetConsoleTextAttribute(h, color);
        for (int i = 0; i < namelength; i++)
        {
            putchar(buff[buffIndex++]);
        }
        printf(": ");
        cout << length;
    }
}

BOOL WINAPI ConsHandler(DWORD event)
{
    switch (event)
    {
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        closesocket(my_sock);
        return TRUE;
        break;
    }
    return FALSE;
}

struct PublicServer {
    sockaddr_in address;
    int count = 0;
};

int GetGateway()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", GetLastError());
        return 1;
    }

    // Получаем информацию об адаптерах
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD adapterInfoSize = sizeof(adapterInfo);
    DWORD result = GetAdaptersInfo(adapterInfo, &adapterInfoSize);
    if (result == ERROR_BUFFER_OVERFLOW) {
        printf("GetAdaptersInfo buffer overflow\n");
        return 1;
    }
    else if (result != ERROR_SUCCESS) {
        printf("GetAdaptersInfo failed: %d\n", GetLastError());
        return 1;
    }

    // Ищем адаптер с адресом шлюза
    IP_ADAPTER_INFO* pAdapterInfo = adapterInfo;
    while (pAdapterInfo != NULL) {
        IP_ADDR_STRING* pGateway = &pAdapterInfo->GatewayList;
        if (strcmp(pGateway->IpAddress.String, "0.0.0.0")) {
            while (pGateway != NULL) {
                if (pGateway->IpAddress.String[0] != '\0') {
                    printf("Gateway: %s (adapter: %s)\n", pGateway->IpAddress.String, pAdapterInfo->AdapterName);
                    gateway = pGateway->IpAddress.String;
                    break;
                }
                pGateway = pGateway->Next;
            }

        }
        pAdapterInfo = pAdapterInfo->Next;
    }

    int i = 0;
    int dotCount = 0;
    for (; i < 13 && dotCount < 3; i++) // находим индекс последней точки
    {
        if (gateway[i] == '.')
            dotCount++;
    }

    gateway[i++] = '2'; // заполняем последний байт числом 255
    gateway[i++] = '5';
    gateway[i++] = '5';

    printf("Gateway: %s \n", gateway);

    // Освобождаем библиотеку Winsock
    WSACleanup();
    return 0;
}

std::vector<PublicServer> discoverServers() {
    std::vector<PublicServer> serverList;

    system("cls");

    // Создание сокета для приема ответов от серверов
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Ошибка при создании сокета" << std::endl;
        return serverList;
    }

    // Включение опции широковещательной рассылки
    int broadcastEnable = 1;
    if (setsockopt(clientSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable)) < 0) {
        std::cerr << "Ошибка при настройке опции широковещательной рассылки" << std::endl;
        return serverList;
    }
   
    GetGateway();
    // Настройка адреса сервера
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr(gateway);

    // Отправка широковещательного запроса
    const char* request = "snakereque";
    int bytesSent = sendto(clientSocket, request, strlen(request), 0,
        (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (bytesSent < 0) {
        std::cerr << "Ошибка при отправке данных" << std::endl;
        return serverList;
    }

    // Установка времени ожидания ответов от серверов
    struct timeval timeout;
    timeout.tv_sec = 50;
    timeout.tv_usec = 0;
    if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        std::cerr << "Ошибка при настройке времени ожидания" << std::endl;
        return serverList;
    }

    // Получение ответов от серверов
    char buffer[1];
    while (true) {
        struct sockaddr_in serverResponse;
        int serverResponseLength = sizeof(serverResponse);
        int bytesRead = recvfrom(clientSocket, buffer, sizeof(buffer), 0,
            (struct sockaddr*)&serverResponse, &serverResponseLength);
        if (bytesRead < 0) {
            // Ошибка при чтении данных или время ожидания истекло
            break;
        }

        bool isExists = false;
        // Сохранение информации о сервере в списке доступных серверов
        for (const auto& server : serverList)
        {
            if (memcmp(&server.address.sin_addr, &serverResponse.sin_addr, sizeof(server.address.sin_addr)) == 0) 
            {
                isExists = true;
                break;
            }
        }
        if (!isExists)
        {
            PublicServer server;
            server.address = serverResponse;
            server.count = buffer[0];
            serverList.push_back(server);
        }
    }

    // Закрытие сокета
    closesocket(clientSocket);

    return serverList;
}

int main(int argc, char* argv[])
{
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsHandler, TRUE);
    char buff[1500];
    char SERVERADDR[15] = "127.0.0.1";

    SetConsoleCP(471);
    SetConsoleOutputCP(471);
    system("mode con cols=52 lines=32"); // установка размеров окна консоли
    MoveWindow(GetConsoleWindow(), 50, 50, 500, 600, true); // установка стартовой позиции окна консоли (50 и 50 - это пиксели
    CONSOLE_CURSOR_INFO cci = { sizeof(cci), false }; // создание параметров на отображение курсора
    SetConsoleCursorInfo(h, &cci); //связывание параметров и хендла

    // Шаг 1 - инициализация библиотеки Winsock
    if (WSAStartup(0x202, (WSADATA*)&buff[0]))
    {
        printf("WSAStart error %d\n", WSAGetLastError());
        return -1;
    }
    
    printf("Press 1 to find servers, press 2 to input the IP\n");
    int k;
    while (true)
    {
        if (_kbhit())
        {
            k = _getch();
            if (k == 49 || k == 50)
                break;
        }
    }
    if (k == 49)
    {
        auto serverList = discoverServers();
        if (serverList.size() > 0)
        {
            printf("Choose server by pressing 1-10\n");
            for (const auto& server : serverList)
                cout << inet_ntoa(server.address.sin_addr) << ": " << server.count << "/4\n";
            while (true)
            {
                if (_kbhit());
                k = _getch();
                if (k >= 49 && k <= 48 + serverList.size())
                {
                    strncpy_s(SERVERADDR, inet_ntoa(serverList[k - 49].address.sin_addr), 15);
                    break;
                }
            }
        }
        else 
        {
            printf("Servers not found, enter the IP: ");
            char input[15];
            cin >> input;
            strncpy_s(SERVERADDR, input, 15);
        }
    }
    else
    {
        printf("Enter the IP: ");
        char input[15];
        cin >> input;
        strncpy_s(SERVERADDR, input, 15);
    }

    // Шаг 2 - создание сокета
    my_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (my_sock < 0)
    {
        printf("Socket() error %d\n", WSAGetLastError());
        return -1;
    }

    // Шаг 3 - установка соединения

    // заполнение структуры sockaddr_in
    // указание адреса и порта сервера
    sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    HOSTENT* hst;

    // преобразование IP адреса из символьного в
    // сетевой формат
    if (inet_addr(SERVERADDR) != INADDR_NONE)
        dest_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
    else
        // попытка получить IP адрес по доменному
        // имени сервера
        if (hst = gethostbyname(SERVERADDR))
            // hst->h_addr_list содержит не массив адресов,
            // а массив указателей на адреса
            ((unsigned long*)&dest_addr.sin_addr)[0] = ((unsigned long**)hst->h_addr_list)[0][0];
        else
        {
            printf("Invalid address %s\n", SERVERADDR);
            closesocket(my_sock);
            WSACleanup();
            return -1;
        }

    // адрес сервера получен – пытаемся установить
    // соединение 
    if (connect(my_sock, (sockaddr*)&dest_addr,
        sizeof(dest_addr)))
    {
        printf("Connect error %d\n", WSAGetLastError());
        return -1;
    }

    // Шаг 4 - чтение и передача сообщений
    while ((recv(my_sock, &buff[0], sizeof(buff), 0)) != SOCKET_ERROR)
    {
        char s = buff[0];
        if (buff[0] == 'n')
        {
            system("cls");
            c.X = 0;
            c.Y = 0;
            SetConsoleCursorPosition(h, c); // установка курсора в эту позицию
            printf("Waiting for players, current count: ");
            putchar((int)buff[1] + 48); // отображение символа тела "змейки"
        }
        else if (buff[0] == 's')
        {
            if (!isFrame)
                DrawFrame();
            DrawField(buff);
            if (_kbhit()) // проверяем, была ли нажата какая-либо клавиша и запускаем её обработку в случае ИСТИНЫ
            {
                int k = _getch(); // считываем код клавиши из буфера
                if (k == 0 || k == 224) // если первый код - вспомогательный, считываем второй код
                    k = _getch();
                if (k == 80 || k == 72 || k == 75 || k == 77)
                    buff[0] = k;
            }
        }
        else if (buff[0] == 'r')
        {
            c.X = 42;
            c.Y = 31 + buff[1];
            SetConsoleCursorPosition(h, c);
            SetConsoleTextAttribute(h, buff[2]);
            printf("WINNER!");
            isFrame = false;
        }

        // передаем строку клиента серверу
        send(my_sock, &buff[0], 1, 0);
        Sleep(sleep_time);
    }

    printf("Recv error %d\n", WSAGetLastError());
    closesocket(my_sock);
    WSACleanup();
    return -1;
}

