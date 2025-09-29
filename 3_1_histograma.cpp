#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

int main(int argc, char* argv[]) {
    int numero_procesos, mi_rango;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_procesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rango);

    int cantidad_datos, numero_bins;
    float valor_minimo, valor_maximo;
    std::vector<float> datos;
    std::vector<int> histograma_local, histograma_global;

    // El proceso 0 lee los datos de entrada
    if (mi_rango == 0) {
        std::cout << "Ingrese cantidad de datos: ";
        std::cin >> cantidad_datos;
        std::cout << "Ingrese número de bins: ";
        std::cin >> numero_bins;
        std::cout << "Ingrese valor mínimo: ";
        std::cin >> valor_minimo;
        std::cout << "Ingrese valor máximo: ";
        std::cin >> valor_maximo;

        // Generar datos aleatorios
        datos.resize(cantidad_datos);
        srand(time(NULL));
        for (int i = 0; i < cantidad_datos; i++) {
            datos[i] = valor_minimo + (valor_maximo - valor_minimo) * ((float)rand() / RAND_MAX);
        }

        std::cout << "\nDatos generados aleatoriamente entre " << valor_minimo
                  << " y " << valor_maximo << std::endl;
    }

    // Distribuir parámetros a todos los procesos
    MPI_Bcast(&cantidad_datos, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&numero_bins, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&valor_minimo, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&valor_maximo, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // Calcular cantidad de datos por proceso
    int datos_locales = cantidad_datos / numero_procesos;
    std::vector<float> mi_porcion_datos(datos_locales);

    // Distribuir datos entre procesos
    MPI_Scatter(datos.data(), datos_locales, MPI_FLOAT,
                mi_porcion_datos.data(), datos_locales, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // Inicializar histograma local
    histograma_local.resize(numero_bins, 0);
    float ancho_bin = (valor_maximo - valor_minimo) / numero_bins;

    // Calcular histograma local
    for (int i = 0; i < datos_locales; i++) {
        int indice_bin = (int)((mi_porcion_datos[i] - valor_minimo) / ancho_bin);
        // Manejar casos límite
        if (indice_bin >= numero_bins) indice_bin = numero_bins - 1;
        if (indice_bin < 0) indice_bin = 0;
        histograma_local[indice_bin]++;
    }

    // Preparar buffer para histograma global (solo en proceso 0)
    if (mi_rango == 0) {
        histograma_global.resize(numero_bins);
    }

    // Reducir histogramas locales al histograma global
    MPI_Reduce(histograma_local.data(), histograma_global.data(), numero_bins, MPI_INT,
               MPI_SUM, 0, MPI_COMM_WORLD);

    // El proceso 0 imprime el histograma final
    if (mi_rango == 0) {
        std::cout << "\n=== HISTOGRAMA FINAL ===" << std::endl;
        std::cout << "Rango\t\t\tFrecuencia" << std::endl;
        std::cout << "-----\t\t\t----------" << std::endl;

        for (int i = 0; i < numero_bins; i++) {
            float inicio_bin = valor_minimo + i * ancho_bin;
            float fin_bin = inicio_bin + ancho_bin;
            std::cout << "[" << inicio_bin << ", " << fin_bin << ")\t\t"
                      << histograma_global[i] << std::endl;
        }

        std::cout << "\nTotal de datos procesados: " << cantidad_datos << std::endl;
    }

    MPI_Finalize();
    return 0;
}