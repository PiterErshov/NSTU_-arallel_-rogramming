#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <mpi.h>

#define MAX_ITER 1000 // предельное количество итераций
#define eps 1.0E-5 // точность приближения
#define BILLION 1.0E+9

// Функция вычета разности между временными величинами
#define clocktimeDifference(start, stop)            \
    1.0 * (stop.tv_sec - start.tv_sec) +            \
    1.0 * (stop.tv_nsec - start.tv_nsec) / BILLION

float *take_mass(float *arr, int size, int N); // функция разделения массива

float *take_diag(float *arr, float *diag, int N); // выделение диагонали из матрицы

float *creat_matrix(float * mat, int n); // генерация матрицы

float *creat_vec(float * vec, int n); // генерация вектора

void print_equation(float *a, int n); // вывод матрицы

void print_vector(float *v, int n); // вывод вектора