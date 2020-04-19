#include "my_mpi_f.h"

vector<SOCKET> sockets;
int socketRank;
int countOfProcess;
HOSTENT *hostent;

void Init()
{
    int start_port = 1000;
    WORD version = MAKEWORD(2, 2);
    WSADATA wsaData;
    typedef unsigned long IPNumber; 
    // Инициализация Winsock 
    WSAStartup(version, (LPWSADATA)&wsaData);
    std::vector<SOCKADDR_IN> servers(countOfProcess);
    // Вектор сокетов для всех процессов 
    sockets.resize(countOfProcess);
    //  Инициализация  сокетов 
    for (int i = 0; i < servers.size(); i++)
    {
        servers[i].sin_family = PF_INET;
        hostent = gethostbyname("localhost"); 
        servers[i].sin_addr.s_addr = (*reinterpret_cast<IPNumber*>(hostent->h_addr_list[0])); 
        servers[i].sin_port = htons(start_port + i);
        sockets[i] = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); 
        if (sockets[i] == INVALID_SOCKET)
        {
            std::cout << "unable to create socket" << socketRank << std::endl; 
            return;
        }
    }
    
    if (socketRank == countOfProcess - 1)
    {
        printf("Socket port: %d\n", servers[socketRank].sin_port);
        int retVal = ::bind(sockets[socketRank], (LPSOCKADDR)&(servers[socketRank]), sizeof(servers[socketRank]));
        if (retVal == SOCKET_ERROR) 
        { 
            printf("Unable to bind\n"); 
            int error = WSAGetLastError(); 
            printf("%d\n", error); 
            WSACleanup(); 
            system("pause");
            return;
        }

        int task = 0;
        retVal = listen(sockets[socketRank], 10); 
        if (retVal == SOCKET_ERROR) 
        {
            printf("Unable to listen\n");
            int error = WSAGetLastError(); 
            printf("%d", error);
            system("pause"); 
            return;
        }
        SOCKADDR_IN from;
        int fromlen = sizeof(from); 
        int buf = 0;
        
        int *temp = new int[1];
        buf = accept(sockets[socketRank], (struct sockaddr*)&from, &fromlen); 
        retVal = recv(buf, (char*)temp, sizeof(int), 0);
        printf("Connect %d process first for \n", temp[0]); 
        sockets[temp[0]] = buf;
    }

    for (int i = socketRank + 1; i < countOfProcess; i++) 
    {
        int retVal = connect(sockets[i], (LPSOCKADDR) &servers[i], sizeof(servers[i]));
        if (retVal == SOCKET_ERROR) 
        {
            std::cout << "unable to connect" << std::endl; 
            int error = WSAGetLastError();
            printf("%ld", error); 
            return;
        }

        int *temp = new int[1];
        temp[0] = socketRank;
        retVal = send(sockets[i], (char*)temp, sizeof(int), 0);
        //perror("error");
        if (retVal == SOCKET_ERROR)
        {
            std::cout << "unable to recv" << std::endl; 
            int error = WSAGetLastError(); 
            printf("%d\n", error);
            return;
        }
    }

    int flag = countOfProcess - 1; 
    int def = 1;
    if (socketRank == countOfProcess - 1) 
        def++;

    for (int i = socketRank - def; i >= 0; i--) 
    { 
        if (socketRank < flag) 
        {
            int retVal = ::bind(sockets[socketRank], (LPSOCKADDR) & (servers[socketRank]), sizeof(servers[socketRank]));
            if (retVal == SOCKET_ERROR) 
            {
                printf("Unable to bind\n");
                int error = WSAGetLastError();
                printf("%d\n", error);
                WSACleanup(); 
                system("pause");
                return;
            }

            int task = 0;
            retVal = listen(sockets[socketRank],  10); 
            if (retVal == SOCKET_ERROR) 
            {
                printf("Unable to listen\n"); 
                int error = WSAGetLastError(); 
                printf("%d", error); 
                system("pause");
                return;
            }
        }
        flag--;
        SOCKADDR_IN from;
        int fromlen = sizeof(from);
        int buf = 0;
        int *temp = new int[1];
        
        buf = accept(sockets[socketRank], (struct sockaddr*)&from, &fromlen); 
        int retVal = recv(buf, (char*)temp, sizeof(int), 0);
        printf("Connect %d process \n", temp[0]); 
        sockets[temp[0]] = buf;
    }
    int retVal = 0;
    std::cout << "Connection made sucessfully" << std::endl;
}
// Отправка сообщения (указатель на данные, размер данных, тип - int, куда отправить)
void MPI_MySend(void *buf, int count, string type, int i)
{
    int size_;
    if (type == "MPI_INT") 
        size_ = count * sizeof(int);
    if (type == "MPI_DOUBLE")
        size_ = count * sizeof(double);
    printf("");
    if (send(sockets[i], (char*)buf, size_, 0) == SOCKET_ERROR)
    {
        std::cout << "unable to send" << std::endl; 
        int error = WSAGetLastError(); 
        printf("%d\n", error);
        return;
    }
}

// Приём сообщения (указатель на область памяти, размер получаемых данных, от какого процесса записывать)
void MPI_MyRecv(void *buf, int count, string type, int i)
{
    int size_;
    if (type == "MPI_INT") 
        size_ = count * sizeof(int);
    if (type == "MPI_DOUBLE") 
        size_ = count * sizeof(double);

    if (recv(sockets[i], (char*)(buf), size_, 0) == SOCKET_ERROR)
    {
        std::cout << "unable to recv" << std::endl; 
        int error = WSAGetLastError();
        printf("%d\n", error);
        return;
    }
}

// Сборка указанного сообщения в указанном процессе 
//(указатель на область памяти, что передаём, указатель на область куда передаём, размер получаемых данных, тип данных, операция, в какой процесс записывать)
//*
void MPI_MyReduse(void *buf, void *send_b, int count, string type, string operation, int i)
{
    int size_;
    if (type == "MPI_INT")
        size_ = count * sizeof(int);
    if (type == "MPI_DOUBLE")
        size_ = count * sizeof(double);

    if(socketRank == i)
    {
        memcpy(send_b, buf, size_);
        void *u;
        for(int j = 0; j < countOfProcess; j++)
        {
            if(j != i)
            {
                if (recv(sockets[j], (char*)u, size_, 0) == SOCKET_ERROR)
                {
                    std::cout << "REDUCE unable to recv" << std::endl; 
                    int error = WSAGetLastError();
                    printf("%d\n", error);
                    printf("%d\n", j);
                    return;
                }
                if(operation == "MPI_SUM")
                {
                    if (type == "MPI_INT")
                        for(int k = 0; k < count; k++)
                            ((int *)send_b)[k] += ((int *)u)[k];

                    if (type == "MPI_DOUBLE")
                        for(int k = 0; k < count; k++)
                            ((double *)send_b)[k] += ((double *)u)[k];
                }
                if(operation == "MPI_MAX")
                {
                    if (type == "MPI_INT") 
                        if(*(int *)send_b < *(int *)u)
                            *(int *)send_b = *(int *)u;   
                    
                    if (type == "MPI_DOUBLE") 
                        if(*(double *)send_b < *(double *)u)
                            *(double*)send_b = *(double*)u;
                }
            }
        }
    }
    else
    {
        printf("");
        if (send(sockets[i], (char*)buf, size_, 0) == SOCKET_ERROR)
        {
            std::cout << "REDUCE unable to send" << std::endl; 
            int error = WSAGetLastError(); 
            printf("%d\n", error);
            return;
        }
    }
}

double *creat_matrix(double * mat, int n) // создаём случайную матрицу
{
    double sum; // парметр суммы элементов вне диагонали
    int seed = time(0) % 100; // параметр времни для получения сида рандомных значений

    srand(seed); // изменение случайных значений в зависимости от текущего времени
    for (int i = 0; i < n; i++) // общий цикл
    {
        for (int j = 0; j < n; j++)
        {
            mat[i * n + j] = rand() % 7; // получаем числа в диапазоне до 0 до 7
            if (rand() & 1) // меняем знак некоторых чисел чисел
                mat[i* n + j] *= -1;
        }
        sum = 0; // обнуляем сумму
        for (int j = 0; j < n; j++)
            if(i != j) // если элемент вне диагонали
                sum += abs(mat[i * n + j]); // заносим его в сумму
        if (mat[i* n + i] < sum) // если диагональный элемент меньше суммы элементво вне диагонали
            mat[i* n + i] += 2 * sum; // прибавляем к ниму удвоенную сумму внедиагональных элементов,
                                      // чтобы добиться диагонального преобладания в матрице
    }   
	return mat;
}

double *creat_vec(double * vec, int n) // создаём случайный вектор 
{
    int seed = time(0) % 100; // параметр времни для получения сида рандомных значений
    
    srand(seed); // изменение случайных значений в зависимости от текущего времени
    for (int i = 0; i < n; i++)
    {
        vec[i] = rand() % 10; // получаем значения от 0 до 10
        if (rand() & 1 && vec[i] != 0) // меняем знак у некоторых элементов вектора
            vec[i] *= -1;
    }
	return vec;
}

double *take_diag(double *arr, double *diag, int N) // выделяем диагональ из матрицы
{   
    for(int i = 0; i < N; i++)
        diag[i] =  arr[i * N + i];  
    return diag;
}

double *take_mass(double *arr, int size, int N) // разбиваем массив на под массивы
{
  int n = N / size; // размер части массива, котоаря попадёт в поток
  int k = 0;
  double *mass = (double*)malloc(sizeof(double) * size * N); // размер входного вектора умножить на число потоков
  
  for(int i = 0; i < size; i++)
  {    
    for(int j = 0; j < N; j++) // зануляем значения выходного вектора
        mass[i * size + j] = 0;
    
    for(int j = k; j < n + k; j++) // заносим в массив только часть, которая попадает в указанную область 
        mass[i * size + j] = arr[j];        
    k += n; // увеличиваем чёстчит на размер части
  }
  return mass;
}

void print_equation(double *a, int n) // выводим матрицу
{
    printf("A*x = b\n");
    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < n; j++) 
            printf("%2d ", (int)(a[i * n + j]));
        printf("\n");
    }
    printf("\n");
}

void print_vector(double *v, int n) // выводим вектор
{
    for(int i = 0; i < n; i++) 
        printf("%.2f ",v[i]);
    printf("\n");
}
