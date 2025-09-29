#include <mpi.h>
#include <iostream>
#include <cmath>

int main(int argc, char* argv[]) {
    int numero_procesos, mi_rango;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_procesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rango);

    double mi_valor = mi_rango + 1;  // Cada proceso tiene un valor (1, 2, 3, ...)
    double suma_parcial = mi_valor;

    std::cout << "Proceso " << mi_rango << " tiene valor inicial: " << mi_valor << std::endl;

    // Implementación tipo mariposa para potencia de 2
    if ((numero_procesos & (numero_procesos - 1)) == 0) {
        std::cout << "Usando algoritmo mariposa para potencia de 2 (" << numero_procesos << " procesos)" << std::endl;

        // Algoritmo tipo mariposa (butterfly)
        for (int paso = 1; paso < numero_procesos; paso *= 2) {
            // Calcular el socio (partner) para este paso
            int socio = mi_rango ^ paso;  // XOR para encontrar el socio

            if (socio < numero_procesos) {
                double valor_recibido;

                // Intercambio simultáneo con el socio
                MPI_Sendrecv(&suma_parcial, 1, MPI_DOUBLE, socio, 0,
                            &valor_recibido, 1, MPI_DOUBLE, socio, 0,
                            MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // Sumar el valor recibido
                suma_parcial += valor_recibido;

                std::cout << "Paso " << (int)log2(paso) + 1 << ": Proceso " << mi_rango
                          << " intercambió con proceso " << socio
                          << ", recibió " << valor_recibido
                          << ", nueva suma: " << suma_parcial << std::endl;
            }
        }
    } else {
        // Implementación para cualquier número de procesos
        std::cout << "Usando algoritmo mariposa generalizado (" << numero_procesos << " procesos)" << std::endl;

        // Encontrar la potencia de 2 más cercana y mayor
        int potencia_2 = 1;
        while (potencia_2 < numero_procesos) {
            potencia_2 *= 2;
        }

        for (int paso = 1; paso < potencia_2; paso *= 2) {
            int socio = mi_rango ^ paso;

            // Solo realizar intercambio si ambos procesos existen
            if (socio < numero_procesos) {
                double valor_recibido;

                MPI_Sendrecv(&suma_parcial, 1, MPI_DOUBLE, socio, 0,
                            &valor_recibido, 1, MPI_DOUBLE, socio, 0,
                            MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                suma_parcial += valor_recibido;

                std::cout << "Paso " << (int)log2(paso) + 1 << ": Proceso " << mi_rango
                          << " intercambió con proceso " << socio
                          << ", recibió " << valor_recibido
                          << ", nueva suma: " << suma_parcial << std::endl;
            } else {
                std::cout << "Paso " << (int)log2(paso) + 1 << ": Proceso " << mi_rango
                          << " no tiene socio válido (sería " << socio << ")" << std::endl;
            }
        }
    }

    // Todos los procesos tienen la suma final en el algoritmo mariposa
    std::cout << "\n=== RESULTADO EN PROCESO " << mi_rango << " ===" << std::endl;
    std::cout << "Suma total usando algoritmo mariposa: " << suma_parcial << std::endl;

    // Verificar resultado solo en proceso 0
    if (mi_rango == 0) {
        double suma_esperada = numero_procesos * (numero_procesos + 1) / 2.0;
        std::cout << "\n=== VERIFICACIÓN ===" << std::endl;
        std::cout << "Suma esperada (1+2+...+" << numero_procesos << "): " << suma_esperada << std::endl;

        if (std::abs(suma_parcial - suma_esperada) < 1e-10) {
            std::cout << "✓ Resultado correcto!" << std::endl;
        } else {
            std::cout << "✗ Error en el cálculo!" << std::endl;
        }

        std::cout << "\nNOTA: En el algoritmo mariposa, TODOS los procesos obtienen el resultado final," << std::endl;
        std::cout << "      a diferencia del algoritmo de árbol donde solo el proceso 0 lo tiene." << std::endl;
    }

    MPI_Finalize();
    return 0;
}