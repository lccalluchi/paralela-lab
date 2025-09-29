#include <mpi.h>
#include <iostream>
#include <cmath>
#include <vector>

int main(int argc, char* argv[]) {
    int numero_procesos, mi_rango;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_procesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rango);

    double mi_valor = mi_rango + 1;  // Cada proceso tiene un valor (1, 2, 3, ...)
    double suma_parcial = mi_valor;
    double suma_total;

    std::cout << "Proceso " << mi_rango << " tiene valor inicial: " << mi_valor << std::endl;

    // Implementación para número de procesos que es potencia de 2
    if ((numero_procesos & (numero_procesos - 1)) == 0) {
        std::cout << "Usando algoritmo para potencia de 2 (" << numero_procesos << " procesos)" << std::endl;

        // Suma con estructura de árbol (potencia de 2)
        for (int paso = 1; paso < numero_procesos; paso *= 2) {
            if (mi_rango % (2 * paso) == 0) {
                // Proceso receptor
                int proceso_fuente = mi_rango + paso;
                if (proceso_fuente < numero_procesos) {
                    double valor_recibido;
                    MPI_Recv(&valor_recibido, 1, MPI_DOUBLE, proceso_fuente, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    suma_parcial += valor_recibido;
                    std::cout << "Proceso " << mi_rango << " recibió " << valor_recibido
                              << " del proceso " << proceso_fuente
                              << ", suma parcial: " << suma_parcial << std::endl;
                }
            } else {
                // Proceso emisor
                int proceso_destino = mi_rango - paso;
                if (proceso_destino >= 0 && (mi_rango % (2 * paso)) == paso) {
                    MPI_Send(&suma_parcial, 1, MPI_DOUBLE, proceso_destino, 0, MPI_COMM_WORLD);
                    std::cout << "Proceso " << mi_rango << " envió " << suma_parcial
                              << " al proceso " << proceso_destino << std::endl;
                    break;  // Este proceso ya terminó su trabajo
                }
            }
        }
    } else {
        // Implementación para cualquier número de procesos
        std::cout << "Usando algoritmo general (" << numero_procesos << " procesos)" << std::endl;

        int procesos_activos = numero_procesos;
        int mi_rango_virtual = mi_rango;

        while (procesos_activos > 1) {
            int mitad = procesos_activos / 2;

            if (mi_rango_virtual < mitad) {
                // Proceso receptor en la mitad inferior
                int proceso_fuente = mi_rango_virtual + mitad;
                if (proceso_fuente < procesos_activos) {
                    double valor_recibido;
                    MPI_Recv(&valor_recibido, 1, MPI_DOUBLE, proceso_fuente, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    suma_parcial += valor_recibido;
                    std::cout << "Proceso " << mi_rango << " recibió " << valor_recibido
                              << " del proceso " << proceso_fuente
                              << ", suma parcial: " << suma_parcial << std::endl;
                }
            } else if (mi_rango_virtual < procesos_activos) {
                // Proceso emisor en la mitad superior
                int proceso_destino = mi_rango_virtual - mitad;
                MPI_Send(&suma_parcial, 1, MPI_DOUBLE, proceso_destino, 0, MPI_COMM_WORLD);
                std::cout << "Proceso " << mi_rango << " envió " << suma_parcial
                          << " al proceso " << proceso_destino << std::endl;
                break;  // Este proceso ya terminó su trabajo
            }

            procesos_activos = mitad + (procesos_activos % 2);
        }
    }

    // El proceso 0 tiene la suma final
    if (mi_rango == 0) {
        suma_total = suma_parcial;
        std::cout << "\n=== RESULTADO FINAL ===" << std::endl;
        std::cout << "Suma total usando estructura de árbol: " << suma_total << std::endl;

        // Verificar resultado con fórmula matemática
        double suma_esperada = numero_procesos * (numero_procesos + 1) / 2.0;
        std::cout << "Suma esperada (1+2+...+" << numero_procesos << "): " << suma_esperada << std::endl;

        if (std::abs(suma_total - suma_esperada) < 1e-10) {
            std::cout << "✓ Resultado correcto!" << std::endl;
        } else {
            std::cout << "✗ Error en el cálculo!" << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
}