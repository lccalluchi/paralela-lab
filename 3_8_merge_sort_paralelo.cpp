#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <iomanip>

void merge(std::vector<int>& arr, int izq, int medio, int der) {
    int n1 = medio - izq + 1;
    int n2 = der - medio;

    std::vector<int> izquierda(n1), derecha(n2);

    for (int i = 0; i < n1; i++)
        izquierda[i] = arr[izq + i];
    for (int j = 0; j < n2; j++)
        derecha[j] = arr[medio + 1 + j];

    int i = 0, j = 0, k = izq;

    while (i < n1 && j < n2) {
        if (izquierda[i] <= derecha[j]) {
            arr[k] = izquierda[i];
            i++;
        } else {
            arr[k] = derecha[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = izquierda[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = derecha[j];
        j++;
        k++;
    }
}

void merge_sort_secuencial(std::vector<int>& arr, int izq, int der) {
    if (izq < der) {
        int medio = izq + (der - izq) / 2;
        merge_sort_secuencial(arr, izq, medio);
        merge_sort_secuencial(arr, medio + 1, der);
        merge(arr, izq, medio, der);
    }
}

std::vector<int> merge_dos_vectores(const std::vector<int>& vec1, const std::vector<int>& vec2) {
    std::vector<int> resultado;
    resultado.reserve(vec1.size() + vec2.size());

    size_t i = 0, j = 0;
    while (i < vec1.size() && j < vec2.size()) {
        if (vec1[i] <= vec2[j]) {
            resultado.push_back(vec1[i]);
            i++;
        } else {
            resultado.push_back(vec2[j]);
            j++;
        }
    }

    while (i < vec1.size()) {
        resultado.push_back(vec1[i]);
        i++;
    }

    while (j < vec2.size()) {
        resultado.push_back(vec2[j]);
        j++;
    }

    return resultado;
}

int main(int argc, char* argv[]) {
    int numero_procesos, mi_rango;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_procesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rango);

    int n_total;
    std::vector<int> datos_completos, mi_porcion, resultado_final;

    // El proceso 0 lee n y genera/distribuye los datos
    if (mi_rango == 0) {
        std::cout << "=== MERGE SORT PARALELO CON MPI ===" << std::endl;
        std::cout << "Ingrese el número total de elementos (n): ";
        std::cin >> n_total;

        // Verificar que n es divisible por el número de procesos
        if (n_total % numero_procesos != 0) {
            std::cout << "Ajustando n para que sea divisible por " << numero_procesos << " procesos" << std::endl;
            n_total = (n_total / numero_procesos) * numero_procesos;
            std::cout << "Nuevo n: " << n_total << std::endl;
        }

        // Generar datos aleatorios
        datos_completos.resize(n_total);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000);

        for (int i = 0; i < n_total; i++) {
            datos_completos[i] = dis(gen);
        }

        std::cout << "Datos generados aleatoriamente entre 1 y 1000" << std::endl;

        // Mostrar datos si n es pequeño
        if (n_total <= 50) {
            std::cout << "\nDatos originales: ";
            for (int i = 0; i < n_total; i++) {
                std::cout << datos_completos[i] << " ";
            }
            std::cout << std::endl;
        }
    }

    // Distribuir n a todos los procesos
    MPI_Bcast(&n_total, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calcular elementos por proceso
    int elementos_por_proceso = n_total / numero_procesos;

    // Preparar buffer local
    mi_porcion.resize(elementos_por_proceso);

    // Distribuir datos
    MPI_Scatter(datos_completos.data(), elementos_por_proceso, MPI_INT,
                mi_porcion.data(), elementos_por_proceso, MPI_INT, 0, MPI_COMM_WORLD);

    std::cout << "Proceso " << mi_rango << " recibió " << elementos_por_proceso << " elementos" << std::endl;

    // Cada proceso ordena su porción localmente
    merge_sort_secuencial(mi_porcion, 0, elementos_por_proceso - 1);

    std::cout << "Proceso " << mi_rango << " completó ordenamiento local" << std::endl;

    // Mostrar porciones ordenadas si n es pequeño
    if (n_total <= 50) {
        for (int proc = 0; proc < numero_procesos; proc++) {
            if (mi_rango == proc) {
                std::cout << "Proceso " << mi_rango << " ordenó: ";
                for (int x : mi_porcion) {
                    std::cout << x << " ";
                }
                std::cout << std::endl;
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    // Fase de merge usando comunicación en árbol
    // Solo el proceso 0 recolecta todos los datos ordenados al final

    // Recolectar todas las porciones ordenadas en el proceso 0
    if (mi_rango == 0) {
        resultado_final = mi_porcion;  // Empezar con mi propia porción

        // Recibir porciones ordenadas de otros procesos y hacer merge
        for (int proceso = 1; proceso < numero_procesos; proceso++) {
            std::vector<int> porcion_recibida(elementos_por_proceso);
            MPI_Recv(porcion_recibida.data(), elementos_por_proceso, MPI_INT,
                     proceso, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Hacer merge de resultado_final con la porción recibida
            resultado_final = merge_dos_vectores(resultado_final, porcion_recibida);

            std::cout << "Proceso 0 hizo merge con datos del proceso " << proceso << std::endl;
        }
    } else {
        // Otros procesos envían sus porciones ordenadas al proceso 0
        MPI_Send(mi_porcion.data(), elementos_por_proceso, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    // El proceso 0 muestra el resultado final
    if (mi_rango == 0) {
        std::cout << "\n=== RESULTADO FINAL ===" << std::endl;
        std::cout << "Ordenamiento paralelo completado con " << numero_procesos << " procesos" << std::endl;

        // Verificar que está ordenado
        bool esta_ordenado = std::is_sorted(resultado_final.begin(), resultado_final.end());
        std::cout << "¿Está ordenado correctamente? " << (esta_ordenado ? "✅ SÍ" : "❌ NO") << std::endl;

        // Mostrar resultado si n es pequeño
        if (n_total <= 50) {
            std::cout << "\nDatos ordenados: ";
            for (int x : resultado_final) {
                std::cout << x << " ";
            }
            std::cout << std::endl;
        } else {
            std::cout << "\nPrimeros 10 elementos: ";
            for (int i = 0; i < std::min(10, (int)resultado_final.size()); i++) {
                std::cout << resultado_final[i] << " ";
            }
            std::cout << "\nÚltimos 10 elementos: ";
            for (int i = std::max(0, (int)resultado_final.size() - 10); i < (int)resultado_final.size(); i++) {
                std::cout << resultado_final[i] << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "\nTotal de elementos ordenados: " << resultado_final.size() << std::endl;
    }

    MPI_Finalize();
    return 0;
}