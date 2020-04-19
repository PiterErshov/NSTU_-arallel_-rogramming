#ifndef MY_MPI_F_H
#define MY_MPI_F_H
#pragma comment (lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <sys/types.h>
#include <WinSock2.h> 
#include <winsock.h> 
#include <conio.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <vector> 
#include <cmath>
#include <ctime>
#include <locale.h>
#include <iostream> 
#include <string> 

#define MAX_ITER 1000 // предельное количество итераций
#define eps 1.0E-5 // точность приближения

using namespace std;
extern vector<SOCKET> sockets;
extern int socketRank;
extern int countOfProcess; 

extern  HOSTENT *hostent;

void Init();

void MPI_MySend(void *buf, int count, string type, int i); // Отправка сообщения (указатель на данные, размер данных, тип - int, куда отправить)

void MPI_MyRecv(void *buf, int count, string type, int i); // Приём сообщения (указатель на область памяти, размер получаемых данных, от какого процесса записывать)


void MPI_MyReduse(void *buf, void *send_b, int count, string type, string operation, int i); // Сборка указанного сообщения в указанном процессе 
                                                                                             // (указатель на область памяти, что передаём, указатель на область куда передаём,
                                                                                             // размер получаемых данных, тип данных, операция, в какой процесс записывать)

double *creat_matrix(double * mat, int n); // создаём случайную матрицу

double *creat_vec(double * vec, int n); // создаём случайный вектор 

double *take_diag(double *arr, double *diag, int N); // выделяем диагональ из матрицы

double *take_mass(double *arr, int size, int N); // разбиваем массив на под массивы

void print_equation(double *a, int n); // выводим матрицу

void print_vector(double *v, int n); // выводим вектор

#endif