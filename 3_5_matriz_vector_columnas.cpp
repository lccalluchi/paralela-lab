#include <mpi.h>
#include <iostream>
#include <vector>
#include <iomanip>

int main(int argc, char* argv[]) {
    int numero_procesos, mi_rango;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_procesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rango);

    int n;  // Orden de la matriz (n x n)
    std::vector<double> matriz_completa, vector_completo, resultado_completo;
    std::vector<double> bloque_columnas, mi_resultado_parcial;

    // El proceso 0 lee la dimensión y genera/lee la matriz y vector
    if (mi_rango == 0) {
        std::cout << "Multiplicación matriz-vector con distribución por columnas" << std::endl;
        std::cout << "Ingrese el orden de la matriz (n): ";
        std::cin >> n;

        // Verificar que n es divisible por el número de procesos
        if (n % numero_procesos != 0) {
            std::cout << "Error: n (" << n << ") debe ser divisible por el número de procesos ("
                      << numero_procesos << ")" << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Inicializar matriz y vector con valores de ejemplo
        matriz_completa.resize(n * n);
        vector_completo.resize(n);
        resultado_completo.resize(n);

        std::cout << "Generando matriz y vector de prueba..." << std::endl;

        // Llenar la matriz (por filas)
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                matriz_completa[i * n + j] = i + j + 1;  // Valores simples para verificación
            }
        }

        // Llenar el vector
        for (int i = 0; i < n; i++) {
            vector_completo[i] = i + 1;
        }

        // Mostrar datos si n es pequeño
        if (n <= 8) {
            std::cout << "\nMatriz A:" << std::endl;
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    std::cout << std::setw(6) << matriz_completa[i * n + j] << " ";
                }
                std::cout << std::endl;
            }

            std::cout << "\nVector x:" << std::endl;
            for (int i = 0; i < n; i++) {
                std::cout << std::setw(6) << vector_completo[i] << " ";
            }
            std::cout << std::endl;
        }
    }

    // Distribuir n a todos los procesos
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calcular cuántas columnas maneja cada proceso
    int columnas_por_proceso = n / numero_procesos;

    // Preparar buffer para el bloque de columnas de cada proceso
    bloque_columnas.resize(n * columnas_por_proceso);

    // El proceso 0 distribuye los bloques de columnas
    if (mi_rango == 0) {
        // Copiar mi bloque (proceso 0)
        for (int col = 0; col < columnas_por_proceso; col++) {
            for (int fila = 0; fila < n; fila++) {
                bloque_columnas[col * n + fila] = matriz_completa[fila * n + col];
            }
        }

        // Enviar bloques a otros procesos
        for (int proceso = 1; proceso < numero_procesos; proceso++) {
            std::vector<double> bloque_temp(n * columnas_por_proceso);
            int columna_inicio = proceso * columnas_por_proceso;

            for (int col = 0; col < columnas_por_proceso; col++) {
                for (int fila = 0; fila < n; fila++) {
                    bloque_temp[col * n + fila] = matriz_completa[fila * n + (columna_inicio + col)];
                }
            }

            MPI_Send(bloque_temp.data(), n * columnas_por_proceso, MPI_DOUBLE,
                     proceso, 0, MPI_COMM_WORLD);
        }
    } else {
        // Otros procesos reciben su bloque de columnas
        MPI_Recv(bloque_columnas.data(), n * columnas_por_proceso, MPI_DOUBLE,
                 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Distribuir el vector completo a todos los procesos
    vector_completo.resize(n);
    MPI_Bcast(vector_completo.data(), n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Cada proceso calcula su contribución al resultado
    mi_resultado_parcial.resize(n, 0.0);

    for (int fila = 0; fila < n; fila++) {
        for (int col = 0; col < columnas_por_proceso; col++) {
            int columna_global = mi_rango * columnas_por_proceso + col;
            mi_resultado_parcial[fila] += bloque_columnas[col * n + fila] * vector_completo[columna_global];
        }
    }

    std::cout << "Proceso " << mi_rango << " completó su cálculo parcial" << std::endl;

    // Reducir resultados parciales al resultado final
    if (mi_rango == 0) {
        resultado_completo.resize(n);
    }

    MPI_Reduce(mi_resultado_parcial.data(), resultado_completo.data(), n, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    // El proceso 0 muestra el resultado
    if (mi_rango == 0) {
        std::cout << "\n=== RESULTADO ===" << std::endl;
        std::cout << "Resultado de A * x:" << std::endl;

        if (n <= 8) {
            for (int i = 0; i < n; i++) {
                std::cout << "y[" << i << "] = " << std::setw(8) << std::fixed
                          << std::setprecision(2) << resultado_completo[i] << std::endl;
            }
        } else {
            std::cout << "Resultado calculado para matriz de " << n << "x" << n
                      << " (demasiado grande para mostrar)" << std::endl;
        }

        std::cout << "\nMultiplicación completada exitosamente usando " << numero_procesos
                  << " procesos con distribución por columnas." << std::endl;
    }

    MPI_Finalize();
    return 0;
}