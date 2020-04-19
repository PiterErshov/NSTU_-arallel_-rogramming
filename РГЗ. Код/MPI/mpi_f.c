#include "mpi_f.h"

float *take_mass(float *arr, int size, int N) // разбиваем массив на под массивы
{
  int n = N / size; // размер части массива, котоаря попадёт в поток
  int k = 0;
  float *mass = (float*)malloc(sizeof(float) * size * N); // размер входного вектора умножить на число потоков
  
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

float *take_diag(float *arr, float *diag, int N) // выделяем диагональ из матрицы
{   
    for(int i = 0; i < N; i++)
        diag[i] =  arr[i * N + i];  
    return diag;
}

float *creat_matrix(float * mat, int n) // создаём случайную матрицу
{
    float sum; // парметр суммы элементов вне диагонали
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

float *creat_vec(float * vec, int n) // создаём случайный вектор 
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

void print_equation(float *a, int n) // выводим матрицу
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

void print_vector(float *v, int n) // выводим вектор
{
    for(int i = 0; i < n; i++) 
        printf("%.2f ",v[i]);
    printf("\n");
}