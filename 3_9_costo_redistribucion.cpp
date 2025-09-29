#include <mpi.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>

int main(int argc, char* argv[]) {
    int numero_procesos, mi_rango;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_procesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rango);

    int n_vector;
    std::vector<double> vector_bloques, vector_ciclico;

    if (mi_rango == 0) {
        std::cout << "=== MEDICIÓN DEL COSTO DE REDISTRIBUCIÓN ===" << std::endl;
        std::cout << "Midiendo tiempo de cambio entre distribución por bloques y cíclica" << std::endl;
        std::cout << "Ingrese el tamaño del vector: ";
        std::cin >> n_vector;

        // Ajustar n para que sea divisible por el número de procesos
        if (n_vector % numero_procesos != 0) {
            n_vector = (n_vector / numero_procesos + 1) * numero_procesos;
            std::cout << "Tamaño ajustado a: " << n_vector << " elementos" << std::endl;
        }
    }

    // Distribuir el tamaño del vector a todos los procesos
    MPI_Bcast(&n_vector, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calcular elementos por proceso
    int elementos_por_proceso = n_vector / numero_procesos;

    // Preparar buffers
    vector_bloques.resize(elementos_por_proceso);
    vector_ciclico.resize(elementos_por_proceso);

    // ==============================================
    // INICIALIZACIÓN: Crear distribución por bloques
    // ==============================================
    if (mi_rango == 0) {
        std::cout << "\n1. Inicializando distribución por bloques..." << std::endl;
    }

    // Cada proceso inicializa su parte con valores secuenciales
    for (int i = 0; i < elementos_por_proceso; i++) {
        vector_bloques[i] = mi_rango * elementos_por_proceso + i + 1;
    }

    // Mostrar distribución inicial si el vector es pequeño
    if (n_vector <= 40) {
        for (int proc = 0; proc < numero_procesos; proc++) {
            if (mi_rango == proc) {
                std::cout << "Proceso " << mi_rango << " (bloques): ";
                for (double val : vector_bloques) {
                    std::cout << std::setw(4) << (int)val << " ";
                }
                std::cout << std::endl;
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);  // Sincronizar antes de medir

    // ==============================================
    // MEDICIÓN 1: De distribución por bloques a cíclica
    // ==============================================
    if (mi_rango == 0) {
        std::cout << "\n2. Convirtiendo de distribución por bloques a cíclica..." << std::endl;
    }

    double inicio_bloques_a_ciclica = MPI_Wtime();

    // Algoritmo para redistribuir de bloques a cíclico
    std::vector<double> buffer_envio(n_vector);
    std::vector<double> buffer_recepcion(n_vector);

    // Paso 1: Recolectar todos los datos en el proceso 0
    MPI_Gather(vector_bloques.data(), elementos_por_proceso, MPI_DOUBLE,
               buffer_recepcion.data(), elementos_por_proceso, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Paso 2: El proceso 0 redistribuye de forma cíclica
    if (mi_rango == 0) {
        for (int i = 0; i < n_vector; i++) {
            int proceso_destino = i % numero_procesos;
            int indice_local = i / numero_procesos;
            buffer_envio[proceso_destino * elementos_por_proceso + indice_local] = buffer_recepcion[i];
        }
    }

    // Paso 3: Distribuir datos reorganizados
    MPI_Scatter(buffer_envio.data(), elementos_por_proceso, MPI_DOUBLE,
                vector_ciclico.data(), elementos_por_proceso, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double fin_bloques_a_ciclica = MPI_Wtime();
    double tiempo_bloques_a_ciclica = fin_bloques_a_ciclica - inicio_bloques_a_ciclica;

    // Mostrar distribución cíclica si el vector es pequeño
    if (n_vector <= 40) {
        for (int proc = 0; proc < numero_procesos; proc++) {
            if (mi_rango == proc) {
                std::cout << "Proceso " << mi_rango << " (cíclica): ";
                for (double val : vector_ciclico) {
                    std::cout << std::setw(4) << (int)val << " ";
                }
                std::cout << std::endl;
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // ==============================================
    // MEDICIÓN 2: De distribución cíclica a bloques
    // ==============================================
    if (mi_rango == 0) {
        std::cout << "\n3. Convirtiendo de distribución cíclica a por bloques..." << std::endl;
    }

    double inicio_ciclica_a_bloques = MPI_Wtime();

    // Algoritmo para redistribuir de cíclico a bloques
    // Paso 1: Recolectar todos los datos en el proceso 0
    MPI_Gather(vector_ciclico.data(), elementos_por_proceso, MPI_DOUBLE,
               buffer_recepcion.data(), elementos_por_proceso, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Paso 2: El proceso 0 redistribuye por bloques
    if (mi_rango == 0) {
        for (int proc = 0; proc < numero_procesos; proc++) {
            for (int i = 0; i < elementos_por_proceso; i++) {
                int indice_global = proc * elementos_por_proceso + i;
                int indice_ciclico = (indice_global % numero_procesos) * elementos_por_proceso +
                                     (indice_global / numero_procesos);
                buffer_envio[proc * elementos_por_proceso + i] = buffer_recepcion[indice_ciclico];
            }
        }
    }

    // Paso 3: Distribuir datos reorganizados
    MPI_Scatter(buffer_envio.data(), elementos_por_proceso, MPI_DOUBLE,
                vector_bloques.data(), elementos_por_proceso, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double fin_ciclica_a_bloques = MPI_Wtime();
    double tiempo_ciclica_a_bloques = fin_ciclica_a_bloques - inicio_ciclica_a_bloques;

    // Verificar que regresamos a la distribución original
    if (n_vector <= 40) {
        for (int proc = 0; proc < numero_procesos; proc++) {
            if (mi_rango == proc) {
                std::cout << "Proceso " << mi_rango << " (bloques restaurados): ";
                for (double val : vector_bloques) {
                    std::cout << std::setw(4) << (int)val << " ";
                }
                std::cout << std::endl;
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    // ==============================================
    // RECOLECTAR Y MOSTRAR RESULTADOS
    // ==============================================
    double tiempo_max_bloques_a_ciclica, tiempo_max_ciclica_a_bloques;

    // Encontrar el tiempo máximo (peor caso) entre todos los procesos
    MPI_Reduce(&tiempo_bloques_a_ciclica, &tiempo_max_bloques_a_ciclica, 1,
               MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tiempo_ciclica_a_bloques, &tiempo_max_ciclica_a_bloques, 1,
               MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (mi_rango == 0) {
        std::cout << "\n=== RESULTADOS DE MEDICIÓN ===" << std::endl;
        std::cout << std::fixed << std::setprecision(6);

        std::cout << "Tamaño del vector: " << n_vector << " elementos" << std::endl;
        std::cout << "Número de procesos: " << numero_procesos << std::endl;
        std::cout << "Elementos por proceso: " << elementos_por_proceso << std::endl;

        std::cout << "\nTiempos de redistribución:" << std::endl;
        std::cout << "  Bloques → Cíclica:  " << tiempo_max_bloques_a_ciclica << " segundos" << std::endl;
        std::cout << "  Cíclica → Bloques:  " << tiempo_max_ciclica_a_bloques << " segundos" << std::endl;

        double promedio = (tiempo_max_bloques_a_ciclica + tiempo_max_ciclica_a_bloques) / 2.0;
        std::cout << "  Tiempo promedio:    " << promedio << " segundos" << std::endl;

        // Análisis del overhead
        double elementos_transferidos = n_vector * sizeof(double);
        double mbytes_transferidos = elementos_transferidos / (1024.0 * 1024.0);

        std::cout << "\nAnálisis del overhead:" << std::endl;
        std::cout << "  Datos transferidos: " << mbytes_transferidos << " MB" << std::endl;

        if (tiempo_max_bloques_a_ciclica > 0) {
            double bandwidth_1 = mbytes_transferidos / tiempo_max_bloques_a_ciclica;
            std::cout << "  Ancho de banda efectivo (B→C): " << bandwidth_1 << " MB/s" << std::endl;
        }

        if (tiempo_max_ciclica_a_bloques > 0) {
            double bandwidth_2 = mbytes_transferidos / tiempo_max_ciclica_a_bloques;
            std::cout << "  Ancho de banda efectivo (C→B): " << bandwidth_2 << " MB/s" << std::endl;
        }

        std::cout << "\nNota: Este overhead debe considerarse al decidir la estrategia de distribución" << std::endl;
        std::cout << "      en algoritmos que requieren cambios de distribución." << std::endl;
    }

    MPI_Finalize();
    return 0;
}