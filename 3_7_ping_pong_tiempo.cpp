#include <mpi.h>
#include <iostream>
#include <ctime>
#include <vector>
#include <iomanip>

int main(int argc, char* argv[]) {
    int numero_procesos, mi_rango;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_procesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rango);

    if (numero_procesos < 2) {
        if (mi_rango == 0) {
            std::cout << "Este programa requiere al menos 2 procesos" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    const int NUMERO_ITERACIONES = 1000000;
    const int TAMANO_MENSAJE = 1;
    std::vector<double> mensaje(TAMANO_MENSAJE, 42.0);

    // Variables para medición de tiempo
    clock_t inicio_clock, fin_clock;
    double inicio_mpi, fin_mpi;
    double tiempo_clock, tiempo_mpi;

    if (mi_rango == 0) {
        std::cout << "=== MEDICIÓN DE TIEMPO PING-PONG ===" << std::endl;
        std::cout << "Comparando clock() vs MPI_Wtime()" << std::endl;
        std::cout << "Número de iteraciones: " << NUMERO_ITERACIONES << std::endl;
        std::cout << "Tamaño del mensaje: " << TAMANO_MENSAJE << " double(s)" << std::endl;
        std::cout << "\nEmpezando mediciones..." << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);  // Sincronizar todos los procesos

    // =========================
    // MEDICIÓN CON clock()
    // =========================
    if (mi_rango == 0) {
        std::cout << "\n1. Midiendo con clock()..." << std::endl;

        // Encontrar tiempo mínimo detectable con clock()
        clock_t tiempo_anterior = clock();
        clock_t tiempo_actual;
        int iteraciones_minimas = 0;

        do {
            iteraciones_minimas++;
            // Realizar algunas operaciones para que pase tiempo
            for (int i = 0; i < 1000; i++) {
                mensaje[0] = mensaje[0] + 0.001;
            }
            tiempo_actual = clock();
        } while (tiempo_actual == tiempo_anterior && iteraciones_minimas < 1000000);

        double tiempo_minimo_clock = double(tiempo_actual - tiempo_anterior) / CLOCKS_PER_SEC;
        std::cout << "Tiempo mínimo detectable con clock(): " << std::scientific
                  << tiempo_minimo_clock << " segundos" << std::endl;
        std::cout << "Iteraciones necesarias: " << iteraciones_minimas << std::endl;

        // Ping-pong con medición clock()
        inicio_clock = clock();

        for (int i = 0; i < NUMERO_ITERACIONES; i++) {
            MPI_Send(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        fin_clock = clock();
        tiempo_clock = double(fin_clock - inicio_clock) / CLOCKS_PER_SEC;

    } else if (mi_rango == 1) {
        // Proceso 1 responde al ping-pong
        for (int i = 0; i < NUMERO_ITERACIONES; i++) {
            MPI_Recv(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // =========================
    // MEDICIÓN CON MPI_Wtime()
    // =========================
    if (mi_rango == 0) {
        std::cout << "\n2. Midiendo con MPI_Wtime()..." << std::endl;

        // Medir resolución de MPI_Wtime()
        double tick = MPI_Wtick();
        std::cout << "Resolución de MPI_Wtime(): " << std::scientific << tick << " segundos" << std::endl;

        // Ping-pong con medición MPI_Wtime()
        inicio_mpi = MPI_Wtime();

        for (int i = 0; i < NUMERO_ITERACIONES; i++) {
            MPI_Send(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        fin_mpi = MPI_Wtime();
        tiempo_mpi = fin_mpi - inicio_mpi;

    } else if (mi_rango == 1) {
        // Proceso 1 responde al ping-pong
        for (int i = 0; i < NUMERO_ITERACIONES; i++) {
            MPI_Recv(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
    }

    // =========================
    // MOSTRAR RESULTADOS
    // =========================
    if (mi_rango == 0) {
        std::cout << "\n=== RESULTADOS ===" << std::endl;
        std::cout << std::fixed << std::setprecision(6);

        std::cout << "\nTiempo total con clock():     " << tiempo_clock << " segundos" << std::endl;
        std::cout << "Tiempo total con MPI_Wtime(): " << tiempo_mpi << " segundos" << std::endl;
        std::cout << "Diferencia absoluta:          " << std::abs(tiempo_clock - tiempo_mpi) << " segundos" << std::endl;

        if (tiempo_mpi > 0) {
            double error_relativo = std::abs(tiempo_clock - tiempo_mpi) / tiempo_mpi * 100.0;
            std::cout << "Error relativo:               " << error_relativo << "%" << std::endl;
        }

        std::cout << "\nTiempo promedio por round-trip:" << std::endl;
        std::cout << "  clock():     " << std::scientific << (tiempo_clock / NUMERO_ITERACIONES) << " segundos" << std::endl;
        std::cout << "  MPI_Wtime(): " << (tiempo_mpi / NUMERO_ITERACIONES) << " segundos" << std::endl;

        // Calcular latencia (tiempo de ida)
        double latencia_clock = (tiempo_clock / NUMERO_ITERACIONES) / 2.0;
        double latencia_mpi = (tiempo_mpi / NUMERO_ITERACIONES) / 2.0;

        std::cout << "\nLatencia estimada (un sentido):" << std::endl;
        std::cout << "  clock():     " << latencia_clock << " segundos" << std::endl;
        std::cout << "  MPI_Wtime(): " << latencia_mpi << " segundos" << std::endl;

        // Análisis de precisión
        double tick = MPI_Wtick();
        std::cout << "\n=== ANÁLISIS DE PRECISIÓN ===" << std::endl;
        std::cout << "Resolución de MPI_Wtime() es " << (tick > 0 ? (1.0/tick) : 0) << " veces mejor que segundos" << std::endl;

        if (tiempo_clock == 0.0) {
            std::cout << "⚠️  clock() devolvió 0 - tiempo muy pequeño para medir con precisión" << std::endl;
        }

        if (tiempo_mpi > 0 && tiempo_clock > 0) {
            std::cout << "✅ Ambos métodos produjeron mediciones válidas" << std::endl;
        }

        std::cout << "\nRecomendación: Use MPI_Wtime() para mediciones precisas en programas MPI" << std::endl;
    }

    MPI_Finalize();
    return 0;
}