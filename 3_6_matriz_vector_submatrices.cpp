#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

int main(int argc, char* argv[]) {
    int numero_procesos, mi_rango;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_procesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rango);

    // Verificar que el número de procesos es un cuadrado perfecto
    int sqrt_procesos = (int)sqrt(numero_procesos);
    if (sqrt_procesos * sqrt_procesos != numero_procesos) {
        if (mi_rango == 0) {
            std::cout << "Error: El número de procesos (" << numero_procesos
                      << ") debe ser un cuadrado perfecto" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    int n;  // Orden de la matriz
    std::vector<double> matriz_completa, vector_completo, resultado_completo;
    std::vector<double> submatriz, subvector, subresultado;

    // Calcular coordenadas del proceso en la grilla 2D
    int fila_proceso = mi_rango / sqrt_procesos;
    int col_proceso = mi_rango % sqrt_procesos;

    // El proceso 0 lee la dimensión y genera los datos
    if (mi_rango == 0) {
        std::cout << "Multiplicación matriz-vector con distribución por submatrices" << std::endl;
        std::cout << "Grilla de procesos: " << sqrt_procesos << "x" << sqrt_procesos << std::endl;
        std::cout << "Ingrese el orden de la matriz (n): ";
        std::cin >> n;

        // Verificar que n es divisible por sqrt_procesos
        if (n % sqrt_procesos != 0) {
            std::cout << "Error: n (" << n << ") debe ser divisible por sqrt(procesos) ("
                      << sqrt_procesos << ")" << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Generar matriz y vector
        matriz_completa.resize(n * n);
        vector_completo.resize(n);
        resultado_completo.resize(n);

        std::cout << "Generando matriz y vector de prueba..." << std::endl;

        // Llenar matriz
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                matriz_completa[i * n + j] = (i + 1) * 10 + (j + 1);
            }
        }

        // Llenar vector
        for (int i = 0; i < n; i++) {
            vector_completo[i] = i + 1;
        }

        // Mostrar datos si n es pequeño
        if (n <= 6) {
            std::cout << "\nMatriz A:" << std::endl;
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    std::cout << std::setw(4) << matriz_completa[i * n + j] << " ";
                }
                std::cout << std::endl;
            }

            std::cout << "\nVector x:" << std::endl;
            for (int i = 0; i < n; i++) {
                std::cout << std::setw(4) << vector_completo[i] << " ";
            }
            std::cout << std::endl;
        }
    }

    // Distribuir n a todos los procesos
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calcular dimensiones de cada submatriz
    int filas_por_proceso = n / sqrt_procesos;
    int cols_por_proceso = n / sqrt_procesos;

    // Preparar buffers para submatriz y subvector
    submatriz.resize(filas_por_proceso * cols_por_proceso);
    subvector.resize(cols_por_proceso);
    subresultado.resize(filas_por_proceso, 0.0);

    // El proceso 0 distribuye las submatrices
    if (mi_rango == 0) {
        // Distribuir mi propia submatriz (proceso 0)
        int mi_fila_inicio = 0;
        int mi_col_inicio = 0;
        for (int i = 0; i < filas_por_proceso; i++) {
            for (int j = 0; j < cols_por_proceso; j++) {
                submatriz[i * cols_por_proceso + j] =
                    matriz_completa[(mi_fila_inicio + i) * n + (mi_col_inicio + j)];
            }
        }

        // Enviar submatrices a otros procesos
        for (int proceso = 1; proceso < numero_procesos; proceso++) {
            int proc_fila = proceso / sqrt_procesos;
            int proc_col = proceso % sqrt_procesos;
            int fila_inicio = proc_fila * filas_por_proceso;
            int col_inicio = proc_col * cols_por_proceso;

            std::vector<double> submatriz_temp(filas_por_proceso * cols_por_proceso);
            for (int i = 0; i < filas_por_proceso; i++) {
                for (int j = 0; j < cols_por_proceso; j++) {
                    submatriz_temp[i * cols_por_proceso + j] =
                        matriz_completa[(fila_inicio + i) * n + (col_inicio + j)];
                }
            }

            MPI_Send(submatriz_temp.data(), filas_por_proceso * cols_por_proceso,
                     MPI_DOUBLE, proceso, 0, MPI_COMM_WORLD);
        }
    } else {
        // Otros procesos reciben su submatriz
        MPI_Recv(submatriz.data(), filas_por_proceso * cols_por_proceso,
                 MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Distribuir partes del vector a procesos en la diagonal
    vector_completo.resize(n);
    MPI_Bcast(vector_completo.data(), n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Cada proceso extrae su parte del vector
    int col_inicio = col_proceso * cols_por_proceso;
    for (int i = 0; i < cols_por_proceso; i++) {
        subvector[i] = vector_completo[col_inicio + i];
    }

    // Realizar multiplicación local: submatriz * subvector
    for (int i = 0; i < filas_por_proceso; i++) {
        for (int j = 0; j < cols_por_proceso; j++) {
            subresultado[i] += submatriz[i * cols_por_proceso + j] * subvector[j];
        }
    }

    std::cout << "Proceso " << mi_rango << " (posición [" << fila_proceso
              << "," << col_proceso << "]) completó cálculo local" << std::endl;

    // Reducir resultados por filas (cada fila de procesos suma sus contribuciones)
    std::vector<double> resultado_fila(filas_por_proceso, 0.0);

    // Crear comunicador para cada fila de procesos
    MPI_Comm comunicador_fila;
    MPI_Comm_split(MPI_COMM_WORLD, fila_proceso, col_proceso, &comunicador_fila);

    // Sumar contribuciones dentro de cada fila
    MPI_Reduce(subresultado.data(), resultado_fila.data(), filas_por_proceso,
               MPI_DOUBLE, MPI_SUM, 0, comunicador_fila);

    // Los procesos en la columna 0 recolectan el resultado final
    if (col_proceso == 0) {
        if (mi_rango == 0) {
            resultado_completo.resize(n);
            // Copiar mi parte
            for (int i = 0; i < filas_por_proceso; i++) {
                resultado_completo[i] = resultado_fila[i];
            }

            // Recibir de otros procesos en la columna 0
            for (int fila = 1; fila < sqrt_procesos; fila++) {
                int proceso_fuente = fila * sqrt_procesos;  // Procesos (1,0), (2,0), etc.
                std::vector<double> parte_resultado(filas_por_proceso);
                MPI_Recv(parte_resultado.data(), filas_por_proceso, MPI_DOUBLE,
                         proceso_fuente, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                for (int i = 0; i < filas_por_proceso; i++) {
                    resultado_completo[fila * filas_por_proceso + i] = parte_resultado[i];
                }
            }
        } else {
            // Otros procesos en columna 0 envían sus resultados al proceso 0
            MPI_Send(resultado_fila.data(), filas_por_proceso, MPI_DOUBLE,
                     0, 1, MPI_COMM_WORLD);
        }
    }

    // El proceso 0 muestra el resultado
    if (mi_rango == 0) {
        std::cout << "\n=== RESULTADO ===" << std::endl;
        std::cout << "Resultado de A * x:" << std::endl;

        if (n <= 10) {
            for (int i = 0; i < n; i++) {
                std::cout << "y[" << i << "] = " << std::setw(8) << std::fixed
                          << std::setprecision(1) << resultado_completo[i] << std::endl;
            }
        } else {
            std::cout << "Resultado calculado para matriz de " << n << "x" << n
                      << " (demasiado grande para mostrar)" << std::endl;
        }

        std::cout << "\nMultiplicación completada usando grilla " << sqrt_procesos
                  << "x" << sqrt_procesos << " de procesos." << std::endl;
    }

    MPI_Comm_free(&comunicador_fila);
    MPI_Finalize();
    return 0;
}