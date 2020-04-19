#include "mpi_f.h"

int main(int argc, char **argv) 
{    
    float *a; // матрица
    float *b; // вектор
    float *x; // невязка
    float *buf; // старое значение вектора невязки
    float *diag; // массив диагональных элементов
    float *loc_mat; // локальная часть матрицы
    int n; // размерность стистемы
    float *r; // массив для передачи частей массива
    float max; // максимальное значение нормы
    int tag = 1; // параметр передачи и приёма для MPI_Send и MPI_Recv
    int iter; // количество итераций
    int size; // количество потоков
    int rank; // ранг потока
    MPI_Status status;
    MPI_Init (&argc, &argv); // инициализация MPI 
    MPI_Comm_size (MPI_COMM_WORLD, &size); // получение числа потоков
    MPI_Comm_rank (MPI_COMM_WORLD, &rank); // получение ранга потока
    n = atoi(argv[1]); // полчение размерности ситемы

    float send[n]; // инициализация массвиа для сборки всех частей вектора невязки между потоками
    a = (float *) malloc(sizeof (float) * n * n);
    b = (float *) malloc(sizeof (float) * n);
    x = (float *) malloc(sizeof (float) * n);
    diag = (float *) malloc(sizeof (float)* n);
    buf = (float *) malloc(sizeof (float) * n);    
    loc_mat = (float *) malloc(sizeof (float) * n * n / size);
    
    for(int i = 0; i < n; i++) // обнуляем вектор сборки частей вектора невязки
        send[i] = 0;

    MPI_Barrier(MPI_COMM_WORLD); // инициализируем барьер, чтобы можно было корректно получить время выполнения программы
    struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start); // получаем текущее время

    if(rank == 0) 
    {         
        a = creat_matrix(a, n); // получаем случайную матрицу A
        b = creat_vec(b, n);    // получаем случайный вектор b
        
        diag = take_diag(a, diag, n); // извлекаем из матрицы диагональ

        for (int i = 0; i < n; i++) // присваиваем вектору невязки x начальное приближение
            x[i] = 1;

        r = take_mass(b, size, n); // разбиваем случайный вектор b на части
        int t = n * n / size; // присваиваем параметру смещения начально указателя на 
                              // элемент матрицы значений размера блока матрицы для каждого потока

        for(int i = 1; i < size; i++) // рассылаем всем потокам, кроме 0 начальные параметры
        {               
            MPI_Send(x, n, MPI_FLOAT, i, tag, MPI_COMM_WORLD); // рассылваем вектор невязки
            MPI_Send(r + i * size, n, MPI_FLOAT, i, tag, MPI_COMM_WORLD); // рассылваем части вектора b
            MPI_Send(a + t, n * n / size, MPI_FLOAT, i, tag, MPI_COMM_WORLD); // рассылаем части матрицы
            MPI_Send(diag, n, MPI_FLOAT, i, tag, MPI_COMM_WORLD); // рассылаем диагональ
            t += n * n / size; // увеличивем параметр смещения
        }
        loc_mat = a; // присваеваем свою часть матрицы бля процесса 0        
    }
    else
    {
        for(int i = 0; i < n; i++) // обнуляем вектор невязки в каждом потоке
            x[i] = 0;        
        MPI_Recv (send, n, MPI_FLOAT, 0, tag, MPI_COMM_WORLD, &status); // получаем вектор невязки
        MPI_Recv (b, n, MPI_FLOAT, 0, tag, MPI_COMM_WORLD, &status); // получаем вектор b
        MPI_Recv (loc_mat, n * n / size, MPI_FLOAT, 0, tag, MPI_COMM_WORLD, &status); // получаем часть матрицы
        MPI_Recv (diag, n, MPI_FLOAT, 0, tag, MPI_COMM_WORLD, &status); // получаем диагональ
        
        for(int j = rank * n / size; j < n / size + rank * n/ size; j++) // переносим в вектор невязки те элементы, которые нужны процессу
            x[j] = send[j];   
        for(int i = 0; i < n; i++) // обнуляем вектор сборки невязки
            send[i] = 0;
    }

    float norm; // создаём параметр нормы
    float sum; // создаём парамерт суммы

    do // запускаем основной цикл метода Якоби
    {
        if(rank == 0) // для процесса 0
        {   
            for(int i = 1; i < size; i++) // рассылаем обновлённый вектор невязки процесам
            {
                MPI_Send(x, n, MPI_FLOAT, i, tag, MPI_COMM_WORLD);
            }            
            for(int j = n / size; j < n; j++) // обнуляем часть вектора неваязки, котоаря не нужна для процесса,
                                          // чтобы при сборке результатов MPI_Reduce не накапливать ненужные значения
                x[j] = 0;
        }
        else
        {
            for(int i = 0; i < n; i++) // обнуляем вектор невязки в каждом потоке
                x[i] = 0;            
            MPI_Recv (send, n, MPI_FLOAT, 0, tag, MPI_COMM_WORLD, &status); // получаем вектор невязки

            for(int j = rank * n / size; j < n / size + rank * n/ size; j++) // переносим в вектор невязки те элементы, которые нужны процессу
                x[j] = send[j];   
            for(int i = 0; i < n; i++) // обнуляем вектор сборки невязки
                send[i] = 0;
        }
          
        int p = rank * n / size; // задаём параметр смещения для правильно вычисления вектора невязки
        for(int i = 0; i < n / size; i++) // запускаем вычисления вектора невязки
        {
            sum = 0; // обнуляем сумму
            for(int j = 0; j < n; j++)
            {
                if ( loc_mat[i * n + j]!= diag[i + p])  // получаем результат умножение матрицы на вектор, 
                    sum += (loc_mat[i * n + j] * x[j]); // исключая диагональные элементы
            }            
            buf[i + p] = (b[i + p] - sum) / diag[i + p]; // получаем промежуточное значени вычитая сумму из вектора b и деля её на диагональный элемент

            norm = fabs(x[p] - buf[p]); // вычисяем норму вектора
            for(int h = p; h < p + n / size; h++)
            {
                if(fabs(x[h] - buf[h]) > norm)
                    norm = fabs(x[h] - buf[h]);
                x[h] = buf[h]; // присваиваем новое значение вектору невязки
            }            
        }
        
        MPI_Reduce(&norm, &max, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD); // получаем максимальное среди процессов значение нормы

        if (rank == 0)
        {           
            norm = max; // присваиваем значние максимума норм общей норме
            for (int i = 1; i != size; ++i) // и рассылваем всем процессам новую норму, чтобы они не выполнили лишних вычислений
                MPI_Send(&max, 1, MPI_FLOAT, i, tag, MPI_COMM_WORLD);            
        }
        else
            MPI_Recv(&norm, 1, MPI_FLOAT, 0, tag, MPI_COMM_WORLD, &status); // принимаем новую норму 

        MPI_Reduce(x, send, n, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD); // собираем новый вектор невязки
        
        if(rank == 0)
        {    
            for(int i = 0; i < n; i++) // обновляем значение вектора невязки
                x[i] = send[i];
        }

        iter++; // увеличивавем число итераций
    } 
    while(norm > eps && iter < MAX_ITER); // продолжаем вычисления, пока норма больше параметра приближения 
                                          // и пока не превышенно максимальное число итераций
    
    MPI_Barrier(MPI_COMM_WORLD); // ждём пока все процессы закончат работу
    
    clock_gettime(CLOCK_MONOTONIC, &stop); // завершаем замер времени

    if(rank == 0)
    {        
        print_vector(b, n); // выводим вектор
        print_equation(a, n); // выводим матрицу
        for(int i = 0; i < n; i++)  // выводим результирующий веткор невязки
            printf("x[%d] = %0.9f \n", i, x[i]);        
        printf("%d \n", iter); // выводим число итераций
        printf("Elapsed time: %lf\n", clocktimeDifference(start, stop)); // выводим время выполнения
    }

    // очищаем память
    free(a);
    free(b);
    free(x);
    MPI_Finalize(); // завершаем параллельную часть программы
    return 0;
}