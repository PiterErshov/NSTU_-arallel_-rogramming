#include "my_mpi_f.h"

int main(int argc, char **argv)
{
    socketRank = atoi(argv[1]); 
    countOfProcess = atoi(argv[2]); 
    int n = atoi(argv[3]); 
        
    double *a; // матрица
    double *b; // вектор
    double *x; // невязка
    double *buf; // старое значение вектора невязки
    double *diag; // массив диагональных элементов
    double *loc_mat; // локальная часть матрицы
    double *r; // массив для передачи частей массива
    
    if (countOfProcess != 1)
        Init();
    a = (double *) malloc(sizeof (double) * n * n);
    b = (double *) malloc(sizeof (double) * n);
    x = (double *) malloc(sizeof (double) * n);
    diag = (double *) malloc(sizeof (double)* n);
    buf = (double *) malloc(sizeof (double) * n);    
    loc_mat = (double *) malloc(sizeof (double) * n * n / countOfProcess);
    double *send = (double *) malloc(sizeof (double) * n);  // инициализация массвиа для сборки всех частей вектора невязки между потоками
    
    clock_t start = clock();

    if(socketRank == countOfProcess - 1)
    {
        a = creat_matrix(a, n);
        b = creat_vec(b, n);    // получаем случайный вектор b
        diag = take_diag(a, diag, n); // извлекаем из матрицы диагональ

        for (int i = 0; i < n; i++) // присваиваем вектору невязки x начальное приближение
            x[i] = 1;
        
        r = take_mass(b, countOfProcess, n); // разбиваем случайный вектор b на части
        int t = 0;
        for(int i = 0; i < countOfProcess - 1; i++)
        {            
            MPI_MySend(a + t, n * n / countOfProcess, "MPI_DOUBLE", i);
            MPI_MySend(b, n, "MPI_DOUBLE", i);
            MPI_MySend(x, n, "MPI_DOUBLE", i);
            MPI_MySend(diag, n, "MPI_DOUBLE", i);
            t +=  n * n / countOfProcess;
        }
        loc_mat = a + t; // присваеваем свою часть матрицы бля процесса 0         
    }
    else
    {
        for (int i = 0; i < n; i++) // присваиваем вектору невязки x начальное приближение
            x[i] = 0;
        MPI_MyRecv(loc_mat,  n * n / countOfProcess, "MPI_DOUBLE", countOfProcess - 1);
        MPI_MyRecv(b, n, "MPI_DOUBLE", countOfProcess - 1);
        MPI_MyRecv(send, n, "MPI_DOUBLE", countOfProcess - 1);
        MPI_MyRecv(diag, n, "MPI_DOUBLE", countOfProcess - 1);
        //for(int j = socketRank * n / countOfProcess; j < n / countOfProcess + socketRank * n / countOfProcess; j++) // переносим в вектор невязки те элементы, которые нужны процессу
            //x[j] = send[j];       
        memcpy(x, send + socketRank * n / countOfProcess, n / countOfProcess + socketRank * n / countOfProcess * sizeof(double));
        for(int i = 0; i < n; i++) // обнуляем вектор сборки невязки
            send[i] = 0;            
    }
    

    double norm; // создаём параметр нормы
    double sum; // создаём парамерт суммы
    double max; // максимальное значение нормы
    int iter; // количество итераций
    do // запускаем основной цикл метода Якоби
    {   
        double x_l[n];
        if(socketRank == countOfProcess - 1) // для процесса 0
        {             
            
            for(int i = 0; i < countOfProcess - 1; i++) // рассылаем обновлённый вектор невязки процесам
            {
                MPI_MySend(x, n, "MPI_DOUBLE", i);
            }           
            for(int j = 0; j < socketRank * n / countOfProcess; j++) // обнуляем часть вектора неваязки, котоаря не нужна для процесса,
                                          // чтобы при сборке результатов MPI_Reduce не накапливать ненужные значения
                x[j] = 0;  
        }
        else           
            MPI_MyRecv(send,  n, "MPI_DOUBLE", countOfProcess - 1); // получаем вектор невязки

        for(int i = 0; i < n; i++) // обнуляем вектор невязки в каждом потоке
            x[i] = 0; 
        int p = socketRank * n / countOfProcess; // задаём параметр смещения для правильно вычисления вектора невязки
        for(int i = 0; i < n / countOfProcess; i++) // запускаем вычисления вектора невязки
        {
            sum = 0; // обнуляем сумму
            for(int j = 0; j < n; j++)
            {
                if ( loc_mat[i * n + j] != diag[i + p])  // получаем результат умножение матрицы на вектор, 
                {
                    double e = loc_mat[i * n + j];
                    double s = x[j];
                    printf("");
                    sum += (e * s); // исключая диагональные элементы
                }
            }
            
            buf[i + p] = (b[i + p] - sum) / diag[i + p]; // получаем промежуточное значени вычитая сумму из вектора b и деля её на диагональный элемент

            norm = fabs(x[p] - buf[p]); // вычисяем норму вектора
            
            for(int h = p; h < p + n / countOfProcess; h++)
            {
                if(fabs(x[h] - buf[h]) > norm)
                    norm = fabs(x[h] - buf[h]);
                x[h] = buf[h]; // присваиваем новое значение вектору невязки                
            }            
        }
        for(int i = 0; i < n; i++) // обнуляем вектор сборки невязки
            send[i] = 0; 
        MPI_MyReduse(&norm, &max, 1, "MPI_DOUBLE", "MPI_MAX", countOfProcess - 1); // получаем максимальное среди процессов значение нормы

        if (socketRank == countOfProcess - 1)
        {           
            norm = max; // присваиваем значние максимума норм общей норме

            for (int i = 0; i < countOfProcess - 1; ++i) // и рассылваем всем процессам новую норму, чтобы они не выполнили лишних вычислений
                MPI_MySend(&max, 1, "MPI_DOUBLE", i);           
        }
        else
             MPI_MyRecv(&norm,  1, "MPI_DOUBLE", countOfProcess - 1); // получаем вектор невязки
        printf("");
        MPI_MyReduse(x, send, n, "MPI_DOUBLE", "MPI_SUM", countOfProcess - 1); // собираем новый вектор невязки
        
        if(socketRank == countOfProcess - 1)
        {      
            memcpy(x, send, n * sizeof(double));
        }
               

        iter++; // увеличивавем число итераций
    } 
    while(norm > eps && iter < MAX_ITER); // продолжаем вычисления, пока норма больше параметра приближения 
    
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;

    if(socketRank == countOfProcess - 1)
    {
        //print_vector(b, n); // выводим вектор
        //print_equation(a, n); // выводим матрицу
        for(int i = 0; i < n; i++)  // выводим результирующий веткор невязки
            printf("x[%d] = %0.9f \n", i, x[i]);
        printf("%d \n", iter); // выводим число итераций
        printf("Elapsed time: %f\n", seconds); // выводим время выполнения
    }
    free(a);
    free(b);
    free(x);
    free(diag);
    free(buf);
    free(send);
    
    WSACleanup();

    return 0;
}