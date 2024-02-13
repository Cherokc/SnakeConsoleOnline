#include <iostream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>  // Wincosk2.h должен быть подключен раньше windows.h!
#include <windows.h>
#include <ctime>
#include <conio.h>
#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
using namespace std;

#define MY_PORT    667 // Порт, который слушает сервер
int nclients = 0;
char* gateway;
DWORD WINAPI HelloToClient(LPVOID client_socket);
DWORD WINAPI HelloToWorld(LPVOID client_socket);

const int width = 50, height = 30; // размеры поля, по которому бегает змейка
const int max_length = 50; // установка максимальной длины "змейки"
int sleep_time = 100; // переменная частоты кадров 
int isOccupiedCell[width][height];
bool isStarted = false;
bool restart = false;
int winner = -1;

COORD c; // объект для хранения координат
HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); // создание хендла потока вывода

void CreateApple1(int x, int y);
void Process();
void Restart();

class Snake {
public:
	char* name;
	int sock = 0;
	int segments_X[max_length]; // массив,хранящий абсциссы звеньев "змейки"
	int segments_Y[max_length]; // массив, хранящий ординаты звеньев "змейки"
	int length = 1; // переменная длины "змейки"
	int dx = 0, dy = 0; // создание нулевого движения "змейки"
	char segment = '*'; // символ для отображения тела "змейки"
	char head = 1; // символ для отображения головы "змейки"
	bool isAlive = true;
	int X_apple; // абсцисса "яблока"
	int Y_apple; // ордината "яблока"
	int color;
	int i;

	Snake(int i) {
		if (i == 0)
			color = 1;
		if (i == 1)
			color = 2;		
		if (i == 2)
			color = 5;
		if (i == 3)
			color = 6;
	}

	void Draw() {
		do {
			segments_X[0] = rand() % (width - 2) + 1;; // установка стартовой абсциссы "змейки"
			segments_Y[0] = rand() % (height - 2) + 1; // установка стартовой ординаты "змейки"
		} while (isOccupiedCell[segments_X[0]][segments_Y[0]] != 0);

		isOccupiedCell[segments_X[0]][segments_Y[0]] = 4;

		CreateApple();

		c.X = segments_X[0]; // связываем объект координат со стартовой позицией "змейки"
		c.Y = segments_Y[0];
		SetConsoleCursorPosition(h, c); // отправляем курсор на позицию головы "змейки"
		SetConsoleTextAttribute(h, 10); // устанавливаем зеленый цвет для отрисовки "змейки"
		putchar(head); // отображаем символ головы "змейки"
	}

	void Move() {	
		if (isAlive) {
			int new_X = segments_X[length - 1] + dx; // определяем значение абсциссы головы "змейки" после смещения
			int new_Y = segments_Y[length - 1] + dy; // то же самое для ординаты
			if (dx != dy) 
			{
				if (isOccupiedCell[new_X][new_Y] != 0 && isOccupiedCell[new_X][new_Y] != 2) // проверка на достижение препятствия
				{
					isAlive = false;
					color = 8;
					return;
				}
				else if (isOccupiedCell[new_X][new_Y] == 2) // проверка на достижение "яблока"
				{
					c.X = segments_X[length - 1]; // установка в объект координат позиции головы "змейки"
					c.Y = segments_Y[length - 1];
					SetConsoleCursorPosition(h, c); // установка курсора в эту позицию
					putchar(segment); // отображение символа тела "змейки"
					isOccupiedCell[c.X][c.Y] = 3;

					length++; // увеличение длины "змейки" (яблоко проглочено)
					c.X = segments_X[length - 1] = new_X; // установка в массивы позиции нового звена "змейки"
					c.Y = segments_Y[length - 1] = new_Y;
					SetConsoleCursorPosition(h, c); // установка туда курсора
					putchar(head); // и отображение там символа головы "змейки"
					isOccupiedCell[new_X][new_Y] = 4;

					if (length == max_length) // проверка, достигла ли длина "змейки" своего максимального значения
					{
						isAlive = false;
						color = 8;
						return;
					}

					CreateApple1(new_X, new_Y);
					SetConsoleTextAttribute(h, 10); // обратная установка цвета в зеленый - для дальнейшего отображения "змейки"
				}
				else // случай, когда голова "змейки" оказалась на новой пустой позиции
				{
					int i = 1; // переменная на количество звеньев, не совпадающих с новой позицией - кроме хвоста "змейки"
					for (; i < length; i++)
						if (new_X == segments_X[i] && new_Y == segments_Y[i]) // если совпадение найдено в цикле - прерываемся
							break;
					if (i < length) // если число несовпадающих звеньев меньше длины "змейки" - то прерываем основной цикл игры
					{
						isAlive = false;
						color = 8;
						return;
					}
					else // а иначе запускаем обработку сдвига "змейки"
					{
						c.X = segments_X[0]; // устанавливаем в объект координат позицию хвоста "змейки"
						c.Y = segments_Y[0];
						SetConsoleCursorPosition(h, c); // двигаем туда курсор
						putchar(' '); // и отображаем пробел (затирка хвоста)
						isOccupiedCell[c.X][c.Y] = 0;

						if (length > 1) // если длина змейки больше 
						{
							c.X = segments_X[length - 1]; // устанавливаем в объект координат предыдущую позицию головы "змейки"
							c.Y = segments_Y[length - 1];
							SetConsoleCursorPosition(h, c);  // двигаем туда курсор
							putchar(segment); // выводим символ тела "змейки"
							isOccupiedCell[segments_X[length - 1]][segments_Y[length - 1]] = 3;
						}

						for (int i = 0; i < length - 1; i++) // запускаем цикл свдига координат звеньев "змейки"
						{
							segments_X[i] = segments_X[i + 1]; // обрабатываем все звенья - кроме последнего
							segments_Y[i] = segments_Y[i + 1];
						}

						c.X = segments_X[length - 1] = new_X; // устанавливаем новую позицию головы "змейки"
						c.Y = segments_Y[length - 1] = new_Y;
						SetConsoleCursorPosition(h, c); // двигаем туда курсора
						putchar(head); // отображаем символ головы "змейки"
						isOccupiedCell[new_X][new_Y] = 4;
					}
				}
			}
		}
		else // отображение мертвой змейки
		{
			SetConsoleTextAttribute(h, 8);
			for (int i = 0; i < length - 1; i++) 
			{
				c.X = segments_X[i];
				c.Y = segments_Y[i];
				SetConsoleCursorPosition(h, c);
				putchar('*');
				isOccupiedCell[c.X][c.Y] = 9;
			}
			c.X = segments_X[length - 1];
			c.Y = segments_Y[length - 1];
			SetConsoleCursorPosition(h, c);
			putchar((char)1);
			isOccupiedCell[c.X][c.Y] = 10;
		}
		return;
	}

	void CreateApple() {
		char apple = 'o'; // символ для отображения "яблока"
		do // цикл ставит координаты яблока случанйм образом - но чтобы они не совпадали со "змейкой"
		{
			X_apple = rand() % (width - 2) + 1;
			Y_apple = rand() % (height - 2) + 1;
		} while (isOccupiedCell[X_apple][Y_apple] != 0);

		isOccupiedCell[X_apple][Y_apple] = 2;
		c.X = X_apple; // связываем объект координат с позициями "яблока"
		c.Y = Y_apple;
		SetConsoleCursorPosition(h, c); // отправляем курсор на позицию "яблока"
		SetConsoleTextAttribute(h, 12); // устанавливаем красный цвет для отрисовки "яблока"
		putchar(apple); // отображаем символ "яблока"
	}
};
Snake snakes[4] = {Snake(0),Snake(1),Snake(2),Snake(3)};

void CreateApple1(int x, int y) {
	int i = 0;
	for (; i < 4; i++) {
		if (snakes[i].X_apple == x && snakes[i].Y_apple == y)
			break;
	}
	char apple = 'o'; // символ для отображения "яблока"
	do // цикл ставит координаты яблока случанйм образом - но чтобы они не совпадали со "змейкой"
	{
		snakes[i].X_apple = rand() % (width - 2) + 1;
		snakes[i].Y_apple = rand() % (height - 2) + 1;
	} while (isOccupiedCell[snakes[i].X_apple][snakes[i].Y_apple] != 0);

	isOccupiedCell[snakes[i].X_apple][snakes[i].Y_apple] = 2;
	c.X = snakes[i].X_apple; // связываем объект координат с позициями "яблока"
	c.Y = snakes[i].Y_apple;
	SetConsoleCursorPosition(h, c); // отправляем курсор на позицию "яблока"
	SetConsoleTextAttribute(h, 12); // устанавливаем красный цвет для отрисовки "яблока"
	putchar(apple); // отображаем символ "яблока"
}

void FieldToArray(char* buff) {
	int buffIndex = 1;
	for (int i = 0; i < 4; i++) {
		buff[buffIndex++] = snakes[i].length;
		buff[buffIndex++] = snakes[i].color;
		for (int j = 0; j < snakes[i].length; j++)
		{
			buff[buffIndex++] = snakes[i].segments_X[j];
			buff[buffIndex++] = snakes[i].segments_Y[j];
		}
		buff[buffIndex++] = snakes[i].X_apple;
		buff[buffIndex++] = snakes[i].Y_apple;
		if (snakes[i].name) 
		{
			int namesize = strlen(snakes[i].name);
			buff[buffIndex++] = namesize;
			for (int j = 0; j < namesize; j++)
			{
				buff[buffIndex++] = snakes[i].name[j];
			}
		}
		else 
		{
			buff[buffIndex++] = 1;
			buff[buffIndex++] = i;
		}
	}
}

DWORD WINAPI Game(LPVOID client_socket) {
	srand(time(0)); // запуск генератора случайных чисел
	rand(); // холостой ход генератора случаный чисел
	system("mode con cols=51 lines=31"); // установка размеров окна консоли
	MoveWindow(GetConsoleWindow(), 50, 50, 400, 700, true); // установка стартовой позиции окна консоли (50 и 50 - это пиксели

	CONSOLE_CURSOR_INFO cci = { sizeof(cci), false }; // создание параметров на отображение курсора
	SetConsoleCursorInfo(h, &cci); //связывание параметров и хендла

	while (true) {
		restart = false;
		Process();
		restart = true;
		Sleep(5000);
		Restart();
	}

	return 0;
}

void Process() {
	system("cls");
	c.X = 0;
	c.Y = 0;
	SetConsoleCursorPosition(h, c); // установка курсора в эту позицию
	cout << "Waiting for players, current count: ";
	while (!isStarted) {
		c.X = 36;
		c.Y = 0;
		SetConsoleCursorPosition(h, c); // установка курсора в эту позицию
		putchar((char)(nclients + 48)); // отображение символа тела "змейки"
		if (nclients == 4)
			isStarted = true;
		Sleep(500);
	}

	system("cls"); // очищаем экран
	SetConsoleTextAttribute(h, 4); // установка цвета, которым рисуется рамка поля
	for (int y = 0; y < height; y++) // стандартный двойной цикл на отрисовку рамки
	{
		for (int x = 0; x < width; x++)
		{
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
			if (s != ' ')
				isOccupiedCell[x][y] = 1;
			putchar(s); // выводим символ
		}
		cout << endl;
	}
	printf("Score:\n");
	for (int i = 0; i < 4; i++) {
		snakes[i].Draw();
	}
	int countAlive = 4;
	while (countAlive > 1) // выходим из цикла, если осталась 1 живая змейка
	{
		Sleep(sleep_time); // задержка потока программы на заданный ранее интервал
		for (int i = 0; i < 4; i++) {
			snakes[i].Move();
		}

		countAlive = 0;
		for (int i = 0; i < 4; i++)
			if (snakes[i].isAlive)
				countAlive++;
	}
	winner = 0;
	for (int i = 0; i < 4; i++)
		if (snakes[winner].length < snakes[i].length)
			winner = i;
}

void Restart() 
{
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
			isOccupiedCell[i][j] = 0;
	isStarted = false;
	for (int i = 0; i < 4; i++)
	{
		Snake sn = Snake(i);
		snakes[i] = sn;
	}
	winner = -1;
}

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

	gateway[11] = '2';
	gateway[12] = '5';
	gateway[13] = '5';
	printf("Gateway: %s \n", gateway);

	// Освобождаем библиотеку Winsock
	WSACleanup();
	return 0;
}

int main()
{
	SetConsoleCP(437);
	SetConsoleOutputCP(437);
	DWORD thID;
	CreateThread(NULL, NULL, Game, NULL, NULL, &thID);
	char buff[1500];    // Буфер для различных нужд
	if (WSAStartup(0x0202, (WSADATA*)&buff[0]))
	{
		printf("Error WSAStartup %d\n",
			WSAGetLastError());
		return -1;
	}

	SOCKET mysocket;
	if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Error socket %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpSocket == INVALID_SOCKET)
	{
		std::cerr << "Failed to create UDP socket." << std::endl;
		WSACleanup();
		return -1;
	}

	sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(MY_PORT);
	local_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(udpSocket, (sockaddr*)&local_addr,
		sizeof(local_addr)))
	{
		std::cerr << "Failed to bind UDP socket." << std::endl;
		closesocket(udpSocket);
		closesocket(udpSocket);
		WSACleanup();
		return 1;
	}

	CreateThread(NULL, NULL, HelloToWorld, &udpSocket, NULL, &thID);

	if (bind(mysocket, (sockaddr*)&local_addr, sizeof(local_addr)))
	{
		// Ошибка
		printf("Error bind %d\n", WSAGetLastError());
		closesocket(mysocket);  // закрываем сокет!
		WSACleanup();
		return -1;
	}

	if (listen(mysocket, 0x100))
	{
		// Ошибка
		printf("Error listen %d\n", WSAGetLastError());
		closesocket(mysocket);
		WSACleanup();
		return -1;
	}

	SOCKET client_socket;
	sockaddr_in client_addr;
	int client_addr_size = sizeof(client_addr);
	while ((client_socket = accept(mysocket, (sockaddr*)&client_addr, &client_addr_size)))
	{
		nclients++;      // увеличиваем счетчик
		// подключившихся клиентов

		// Вызов нового потока для обслужвания клиента
		// Да, для этого рекомендуется использовать
		// _beginthreadex но, поскольку никаких вызов
		// функций стандартной Си библиотеки поток не
		// делает, можно обойтись и CreateThread
		DWORD thID;
		CreateThread(NULL, NULL, HelloToClient, &client_socket, NULL, &thID);
	}
	return 0;
}

DWORD WINAPI HelloToClient(LPVOID client_socket)
{
	SOCKET my_sock;
	my_sock = ((SOCKET*)client_socket)[0];

	char buff[1500];
	buff[0] = 'n';
	send(my_sock, &buff[0], 1, 0);

	int i = 0;
	for (; i < 4; i++)
	{
		if (snakes[i].sock == 0)
		{
			snakes[i].sock = my_sock;
			break;
		}
		if (i == 3)
			closesocket(my_sock);
	}

	// цикл эхо-сервера: прием строки от клиента и
	// возвращение ее клиенту
	while (recv(my_sock, &buff[0], sizeof(buff),NULL) >= 0)
	{
		if (!(snakes[i].name))
		{
			sockaddr_in address;
			HOSTENT* hst;
			int addressSize = sizeof(address);
			if (getpeername(my_sock, (struct sockaddr*)&address, &addressSize) == 0)
			{
				hst = gethostbyaddr((char*)&address.sin_addr.s_addr, 4, AF_INET);
				snakes[i].name = hst->h_name;
			}
		}

		if (restart) 
		{
			buff[0] = 'r';
			buff[1] = winner;
			buff[2] = snakes[winner].color;
		}
		else if (isStarted)
		{
			if (buff[0] == 72 && snakes[i].dy != 1) // верх
			{
				snakes[i].dx = 0;
				snakes[i].dy = -1;
			}
			if (buff[0] == 80 && snakes[i].dy != -1) // вниз
			{
				snakes[i].dx = 0;
				snakes[i].dy = 1;
			}
			if (buff[0] == 75 && snakes[i].dx != 1) // влево
			{
				snakes[i].dx = -1;
				snakes[i].dy = 0;
			}
			if (buff[0] == 77 && snakes[i].dx != -1) // вправо
			{
				snakes[i].dx = 1;
				snakes[i].dy = 0;
			}
			buff[0] = 's';
			FieldToArray(buff);
		}
		else 
		{
			buff[0] = 'n';
			buff[1] = nclients;
		}

		send(my_sock, &buff[0], sizeof(buff), 0);
	}

	// если мы здесь, то произошел выход из цикла по
	// причине возращения функцией recv ошибки –
	// соединение клиентом разорвано
	nclients--; // уменьшаем счетчик активных клиентов

	// закрываем сокет
	closesocket(my_sock);
	snakes[i].sock = 0;
	return 0;
}

DWORD WINAPI HelloToWorld(LPVOID client_socket) {
	char buff[10];
	SOCKET udpSocket;
	udpSocket = ((SOCKET*)client_socket)[0];

	sockaddr_in local_addr;
	int addressSize = sizeof(local_addr);
	getsockname(udpSocket, (struct sockaddr*)&local_addr, &addressSize);

	while (true) {
		int clientAddressLength = sizeof(local_addr);

		// Получение данных от клиента
		int bytesRead = recvfrom(udpSocket, buff, sizeof(buff), 0,
			(struct sockaddr*)&local_addr, &clientAddressLength);
		if (bytesRead < 0) {
			std::cerr << "Ошибка при чтении данных" << std::endl;
			return -1;
		}
		if (strncmp(buff, "snakereque", sizeof(buff)) == 0)
		{
			// Отправка ответа клиенту5
			buff[0] = nclients;
			int bytesSent = sendto(udpSocket, &buff[0], 1, 0,
				(struct sockaddr*)&local_addr, clientAddressLength);
			if (bytesSent < 0) {
				std::cerr << "Ошибка при отправке данных" << std::endl;
				return -1;
			}
		}
	}
}