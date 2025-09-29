#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <random>

int main(int argc, char* argv[]) {
    int numero_procesos, mi_rango;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_procesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rango);

    long long int total_lanzamientos;
    long long int puntos_en_circulo_local = 0;
    long long int puntos_en_circulo_global = 0;

    // El proceso 0 lee el número total de lanzamientos
    if (mi_rango == 0) {
        std::cout << "Estimación de π usando método Monte Carlo con MPI" << std::endl;
        std::cout << "Ingrese el número total de lanzamientos: ";
        std::cin >> total_lanzamientos;
        std::cout << "\nUsando " << numero_procesos << " procesos..." << std::endl;
    }

    // Distribuir el número de lanzamientos a todos los procesos
    MPI_Bcast(&total_lanzamientos, 1, MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD);

    // Calcular lanzamientos por proceso
    long long int lanzamientos_locales = total_lanzamientos / numero_procesos;

    // Configurar generador de números aleatorios único para cada proceso
    std::random_device dispositivo_aleatorio;
    std::mt19937 generador(dispositivo_aleatorio() + mi_rango);
    std::uniform_real_distribution<double> distribucion(-1.0, 1.0);

    // Realizar lanzamientos locales (simulación Monte Carlo)
    for (long long int lanzamiento = 0; lanzamiento < lanzamientos_locales; lanzamiento++) {
        double x = distribucion(generador);  // Coordenada x aleatoria entre -1 y 1
        double y = distribucion(generador);  // Coordenada y aleatoria entre -1 y 1

        double distancia_cuadrada = x * x + y * y;

        // Si el punto está dentro del círculo unitario
        if (distancia_cuadrada <= 1.0) {
            puntos_en_circulo_local++;
        }
    }

    // Reducir resultados locales al resultado global
    MPI_Reduce(&puntos_en_circulo_local, &puntos_en_circulo_global, 1,
               MPI_LONG_LONG_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // El proceso 0 calcula e imprime la estimación de π
    if (mi_rango == 0) {
        double estimacion_pi = 4.0 * puntos_en_circulo_global / ((double)total_lanzamientos);

        std::cout << "\n=== RESULTADOS ===" << std::endl;
        std::cout << "Total de lanzamientos: " << total_lanzamientos << std::endl;
        std::cout << "Puntos dentro del círculo: " << puntos_en_circulo_global << std::endl;
        std::cout << "Estimación de π: " << estimacion_pi << std::endl;
        std::cout << "Valor real de π: " << 3.141592653589793 << std::endl;
        std::cout << "Error absoluto: " << std::abs(estimacion_pi - 3.141592653589793) << std::endl;

        // Mostrar estadísticas por proceso
        std::cout << "\nCada proceso procesó aproximadamente "
                  << lanzamientos_locales << " lanzamientos" << std::endl;
    }

    MPI_Finalize();
    return 0;
}