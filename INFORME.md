# Informe de Programas Paralelos con MPI

## Descripción General

Este proyecto contiene una colección de programas paralelos implementados en C++ utilizando la biblioteca MPI (Message Passing Interface). Los programas demuestran diferentes técnicas de paralelización y patrones de comunicación entre procesos.

---

## Programas Implementados

### 1. Histograma Paralelo (`3_1_histograma.cpp`)

**Descripción:** Calcula un histograma de datos distribuidos utilizando MPI.

**Características principales:**
- Distribución de datos aleatorios entre procesos
- Cálculo local de histogramas parciales
- Reducción global usando `MPI_Reduce`
- El proceso 0 genera datos y presenta resultados

**Funciones MPI utilizadas:**
- `MPI_Bcast`: Distribuye parámetros a todos los procesos
- `MPI_Scatter`: Distribuye datos entre procesos
- `MPI_Reduce`: Combina histogramas locales

**Complejidad:**
- Secuencial: O(n)
- Paralelo: O(n/p) donde p es el número de procesos

#### Explicación del Código

**1. Distribución de parámetros y datos:**
```cpp
// El proceso 0 lee parámetros
if (mi_rango == 0) {
    std::cin >> cantidad_datos >> numero_bins >> valor_minimo >> valor_maximo;
    // Generar datos aleatorios
    datos.resize(cantidad_datos);
    for (int i = 0; i < cantidad_datos; i++) {
        datos[i] = valor_minimo + (valor_maximo - valor_minimo) * ((float)rand() / RAND_MAX);
    }
}

// Broadcast de parámetros a todos los procesos
MPI_Bcast(&cantidad_datos, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&numero_bins, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&valor_minimo, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
MPI_Bcast(&valor_maximo, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
```
- Proceso 0 genera datos aleatorios
- Los parámetros se distribuyen a todos con `MPI_Bcast`

**2. Distribución de datos con Scatter:**
```cpp
int datos_locales = cantidad_datos / numero_procesos;
std::vector<float> mi_porcion_datos(datos_locales);

// Scatter distribuye partes iguales a cada proceso
MPI_Scatter(datos.data(), datos_locales, MPI_FLOAT,
            mi_porcion_datos.data(), datos_locales, MPI_FLOAT, 0, MPI_COMM_WORLD);
```
- Cada proceso recibe `n/p` datos

**3. Cálculo del histograma local:**
```cpp
histograma_local.resize(numero_bins, 0);
float ancho_bin = (valor_maximo - valor_minimo) / numero_bins;

for (int i = 0; i < datos_locales; i++) {
    int indice_bin = (int)((mi_porcion_datos[i] - valor_minimo) / ancho_bin);
    if (indice_bin >= numero_bins) indice_bin = numero_bins - 1;
    histograma_local[indice_bin]++;
}
```
- Cada dato se asigna a un bin según su valor
- Manejo de casos límite

**4. Reducción con MPI_Reduce:**
```cpp
// Combinar todos los histogramas locales en el proceso 0
MPI_Reduce(histograma_local.data(), histograma_global.data(), numero_bins,
           MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
```
- `MPI_SUM` suma las frecuencias de todos los procesos

#### Ejemplo de Ejecución (4 procesos, 10000 datos, 10 bins)

```
=== HISTOGRAMA FINAL ===
Rango			Frecuencia
-----			----------
[0, 10)		1050
[10, 20)		955
[20, 30)		1038
[30, 40)		967
[40, 50)		1010
[50, 60)		953
[60, 70)		993
[70, 80)		1051
[80, 90)		1009
[90, 100)		974

Total de datos procesados: 10000
```

**Observaciones:**
- Distribución uniforme de frecuencias (aproximadamente 1000 por bin)
- Suma total = 10000 ✓

---

### 2. Estimación de π (Monte Carlo) (`3_2_monte_carlo_pi.cpp`)

**Descripción:** Estima el valor de π mediante el método de Monte Carlo distribuido.

**Características principales:**
- Generación de puntos aleatorios en cada proceso
- Cada proceso cuenta puntos dentro del círculo unitario
- Reducción de resultados con `MPI_Reduce`
- Cálculo de error absoluto respecto al valor real de π

**Funciones MPI utilizadas:**
- `MPI_Bcast`: Distribuye número total de lanzamientos
- `MPI_Reduce`: Suma puntos dentro del círculo

**Precisión:** Mejora con el número de lanzamientos (n)

#### Explicación del Código

**1. Distribución del número de lanzamientos:**
```cpp
if (mi_rango == 0) {
    std::cout << "Ingrese el número total de lanzamientos: ";
    std::cin >> total_lanzamientos;
}
MPI_Bcast(&total_lanzamientos, 1, MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD);
```
- Proceso 0 lee el parámetro y lo distribuye

**2. Generador de números aleatorios único por proceso:**
```cpp
long long int lanzamientos_locales = total_lanzamientos / numero_procesos;

// Generador único para cada proceso (semilla diferente)
std::random_device dispositivo_aleatorio;
std::mt19937 generador(dispositivo_aleatorio() + mi_rango);
std::uniform_real_distribution<double> distribucion(-1.0, 1.0);
```
- Cada proceso tiene su propio generador con semilla única
- Evita duplicación de secuencias aleatorias

**3. Simulación Monte Carlo:**
```cpp
for (long long int lanzamiento = 0; lanzamiento < lanzamientos_locales; lanzamiento++) {
    double x = distribucion(generador);  // Punto aleatorio en [-1, 1]
    double y = distribucion(generador);

    double distancia_cuadrada = x * x + y * y;

    // Verificar si está dentro del círculo unitario
    if (distancia_cuadrada <= 1.0) {
        puntos_en_circulo_local++;
    }
}
```
- Genera puntos aleatorios en el cuadrado [-1,1]×[-1,1]
- Cuenta los que caen dentro del círculo (x²+y²≤1)

**4. Reducción y cálculo de π:**
```cpp
// Sumar puntos de todos los procesos
MPI_Reduce(&puntos_en_circulo_local, &puntos_en_circulo_global, 1,
           MPI_LONG_LONG_INT, MPI_SUM, 0, MPI_COMM_WORLD);

if (mi_rango == 0) {
    // π ≈ 4 × (área_círculo / área_cuadrado)
    double estimacion_pi = 4.0 * puntos_en_circulo_global / ((double)total_lanzamientos);
}
```
- Razón de puntos en círculo ≈ π/4
- Por tanto: π ≈ 4 × (puntos_dentro / total_puntos)

#### Ejemplo de Ejecución (4 procesos, 10 millones de lanzamientos)

```
=== RESULTADOS ===
Total de lanzamientos: 10000000
Puntos dentro del círculo: 7854701
Estimación de π: 3.14188
Valor real de π: 3.14159
Error absoluto: 0.000287746

Cada proceso procesó aproximadamente 2500000 lanzamientos
```

**Observaciones:**
- Error absoluto de 0.0003 con 10M lanzamientos
- Precisión de ~99.99%
- Cada proceso maneja 2.5M lanzamientos de forma independiente

---

### 3. Suma con Estructura de Árbol (`3_3_suma_arbol.cpp`)

**Descripción:** Implementa reducción de suma usando comunicación en árbol.

**Características principales:**
- Algoritmo optimizado para potencias de 2
- Algoritmo general para cualquier número de procesos
- Comunicación punto a punto (`MPI_Send`/`MPI_Recv`)
- Solo el proceso 0 obtiene el resultado final

**Funciones MPI utilizadas:**
- `MPI_Send`: Envío de sumas parciales
- `MPI_Recv`: Recepción de sumas parciales

**Complejidad de comunicación:** O(log p)

#### Explicación del Código

**1. Algoritmo de reducción en árbol para potencias de 2:**
```cpp
for (int paso = 1; paso < numero_procesos; paso *= 2) {
    if (mi_rango % (2 * paso) == 0) {
        // Proceso receptor
        int proceso_fuente = mi_rango + paso;
        if (proceso_fuente < numero_procesos) {
            double valor_recibido;
            MPI_Recv(&valor_recibido, 1, MPI_DOUBLE, proceso_fuente, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            suma_parcial += valor_recibido;
        }
    } else {
        // Proceso emisor
        int proceso_destino = mi_rango - paso;
        if (proceso_destino >= 0 && (mi_rango % (2 * paso)) == paso) {
            MPI_Send(&suma_parcial, 1, MPI_DOUBLE, proceso_destino, 0, MPI_COMM_WORLD);
            break;  // Este proceso ya terminó
        }
    }
}
```

**Patrón de comunicación (4 procesos):**
```
Paso 1 (paso=1):
  P1 → P0: P0 recibe de P1
  P3 → P2: P2 recibe de P3

Paso 2 (paso=2):
  P2 → P0: P0 recibe de P2

Resultado: P0 tiene suma total = 1+2+3+4 = 10
```

**2. Características clave:**
- Cada paso duplica la distancia entre comunicadores
- Procesos con índices pares son receptores
- Procesos con índices impares son emisores
- Número de pasos: log₂(p)
- Solo P0 tiene el resultado final

#### Ejemplo de Ejecución (4 procesos)

```
Proceso 0: valor inicial = 1
Proceso 1: valor inicial = 2
Proceso 2: valor inicial = 3
Proceso 3: valor inicial = 4

Árbol de comunicación:
- P1 → P0, P3 → P2 (paso 1)
- P2 → P0 (paso 2)

=== RESULTADO FINAL ===
Suma total: 10
Suma esperada (1+2+3+4): 10
✓ Resultado correcto!
```

**Observaciones:**
- Solo 2 pasos para 4 procesos (log₂4 = 2)
- Solo el proceso 0 tiene el resultado final
- Eficiente para operaciones de reducción

---

### 4. Suma con Algoritmo Mariposa (`3_4_suma_mariposa.cpp`)

**Descripción:** Implementa reducción de suma usando patrón butterfly (mariposa).

**Características principales:**
- Intercambio simultáneo de datos con socios (XOR para encontrar pares)
- Todos los procesos obtienen el resultado final
- Usa `MPI_Sendrecv` para comunicación bidireccional
- Optimizado para potencias de 2, con versión generalizada

**Funciones MPI utilizadas:**
- `MPI_Sendrecv`: Intercambio bidireccional simultáneo

**Complejidad de comunicación:** O(log p)

**Ventaja:** Todos los procesos tienen el resultado (all-reduce)

#### Explicación del Código

**1. Algoritmo Butterfly usando XOR:**
```cpp
for (int paso = 1; paso < numero_procesos; paso *= 2) {
    // Calcular el socio (partner) usando XOR
    int socio = mi_rango ^ paso;  // XOR bit a bit

    if (socio < numero_procesos) {
        double valor_recibido;

        // Intercambio bidireccional simultáneo
        MPI_Sendrecv(&suma_parcial, 1, MPI_DOUBLE, socio, 0,
                    &valor_recibido, 1, MPI_DOUBLE, socio, 0,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        suma_parcial += valor_recibido;
    }
}
```

**Patrón de comunicación usando XOR (4 procesos):**
```
Representación binaria de rangos:
P0 = 00, P1 = 01, P2 = 10, P3 = 11

Paso 1 (XOR con 01):
  P0 (00) ↔ P1 (01)
  P2 (10) ↔ P3 (11)

Paso 2 (XOR con 10):
  P0 (00) ↔ P2 (10)
  P1 (01) ↔ P3 (11)

Resultado: TODOS los procesos tienen suma = 10
```

**2. Características clave del Butterfly:**
- **XOR bit a bit** encuentra parejas de intercambio
- **MPI_Sendrecv** hace envío y recepción simultánea (evita deadlock)
- Comunicación **simétrica** (ambos procesos envían y reciben)
- Al final, **todos** los procesos tienen el resultado completo
- Mismo número de pasos que árbol: O(log p)
- Más eficiente que árbol cuando todos necesitan el resultado

**3. Ventaja sobre el árbol:**
```
Árbol:      Solo P0 tiene resultado → necesita Broadcast si otros lo requieren
Butterfly:  TODOS tienen resultado → no necesita comunicación extra
```

#### Ejemplo de Ejecución (4 procesos)

```
Patrón de comunicación mariposa:
Paso 1: P0↔P1, P2↔P3 (XOR con 1)
Paso 2: P0↔P2, P1↔P3 (XOR con 2)

=== RESULTADO ===
Todos los procesos obtienen suma = 10
✓ Resultado correcto!

NOTA: TODOS los procesos obtienen el resultado final,
      a diferencia del árbol donde solo P0 lo tiene.
```

**Observaciones:**
- Intercambios simultáneos (más eficiente que árbol)
- Todos los procesos tienen el resultado (all-reduce)
- Usa XOR bit a bit para encontrar parejas
- Mismo número de pasos que árbol pero con comunicación simétrica

---

### 5. Multiplicación Matriz-Vector (Columnas) (`3_5_matriz_vector_columnas.cpp`)

**Descripción:** Multiplica una matriz por un vector distribuyendo la matriz por columnas.

**Características principales:**
- Distribución de columnas de la matriz entre procesos
- El vector completo se distribuye a todos con `MPI_Bcast`
- Cada proceso calcula su contribución al resultado
- Reducción final con `MPI_Reduce` para sumar contribuciones

**Funciones MPI utilizadas:**
- `MPI_Bcast`: Distribuye el vector completo
- `MPI_Send`/`MPI_Recv`: Distribución manual de columnas
- `MPI_Reduce`: Suma contribuciones parciales

**Requisitos:** n debe ser divisible por el número de procesos

#### Explicación del Código

**1. Distribución de columnas desde P0:**
```cpp
if (mi_rango == 0) {
    // P0 copia su bloque de columnas (primeras columnas)
    for (int col = 0; col < columnas_por_proceso; col++) {
        for (int fila = 0; fila < n; fila++) {
            bloque_columnas[col * n + fila] = matriz_completa[fila * n + col];
        }
    }

    // Enviar bloques a otros procesos
    for (int proceso = 1; proceso < numero_procesos; proceso++) {
        std::vector<double> bloque_temp(n * columnas_por_proceso);
        int columna_inicio = proceso * columnas_por_proceso;

        // Extraer columnas asignadas a ese proceso
        for (int col = 0; col < columnas_por_proceso; col++) {
            for (int fila = 0; fila < n; fila++) {
                bloque_temp[col * n + fila] =
                    matriz_completa[fila * n + (columna_inicio + col)];
            }
        }

        MPI_Send(bloque_temp.data(), n * columnas_por_proceso, MPI_DOUBLE,
                 proceso, 0, MPI_COMM_WORLD);
    }
}
```
- La matriz se almacena por filas pero se distribuye por columnas
- Cada proceso recibe `n/p` columnas completas

**2. Broadcast del vector:**
```cpp
MPI_Bcast(vector_completo.data(), n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
```
- Todos los procesos necesitan el vector completo

**3. Cálculo local de contribución:**
```cpp
mi_resultado_parcial.resize(n, 0.0);

for (int fila = 0; fila < n; fila++) {
    for (int col = 0; col < columnas_por_proceso; col++) {
        int columna_global = mi_rango * columnas_por_proceso + col;
        mi_resultado_parcial[fila] +=
            bloque_columnas[col * n + fila] * vector_completo[columna_global];
    }
}
```
- Cada proceso multiplica sus columnas por los elementos correspondientes del vector
- Produce un vector parcial de tamaño n

**4. Reducción del resultado:**
```cpp
MPI_Reduce(mi_resultado_parcial.data(), resultado_completo.data(), n,
           MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
```
- Suma los vectores parciales elemento por elemento

**Ejemplo visual (4 procesos, n=8):**
```
Matriz A (8×8):          Vector x:
[a00 a01 a02 ... a07]    [x0]
[a10 a11 a12 ... a17]    [x1]
...                       ...
[a70 a71 a72 ... a77]    [x7]

Distribución de columnas:
P0: columnas 0-1
P1: columnas 2-3
P2: columnas 4-5
P3: columnas 6-7

Cada proceso calcula:
P0: [a00·x0+a01·x1, a10·x0+a11·x1, ..., a70·x0+a71·x1]
P1: [a02·x2+a03·x3, a12·x2+a13·x3, ..., a72·x2+a73·x3]
...

Reduce suma todos los resultados parciales
```

#### Ejemplo de Ejecución (4 procesos, matriz 8×8)

```
Matriz A (8×8) y Vector x (8×1):
Cada proceso maneja 2 columnas

=== RESULTADO ===
y[0] = 204.00
y[1] = 240.00
y[2] = 276.00
y[3] = 312.00
y[4] = 348.00
y[5] = 384.00
y[6] = 420.00
y[7] = 456.00

Multiplicación completada con distribución por columnas.
```

**Observaciones:**
- Cada proceso calcula su contribución parcial
- El vector x se broadcast a todos
- Resultados parciales se suman con `MPI_Reduce`

---

### 6. Multiplicación Matriz-Vector (Submatrices) (`3_6_matriz_vector_submatrices.cpp`)

**Descripción:** Multiplica matriz por vector distribuyendo la matriz en submatrices (bloques 2D).

**Características principales:**
- Distribución bidimensional de la matriz (grilla 2D de procesos)
- Cada proceso maneja una submatriz
- Requiere número de procesos que sea cuadrado perfecto (4, 9, 16, etc.)
- Mejor localidad de caché que distribución por columnas

**Requisitos:**
- Número de procesos debe ser cuadrado perfecto
- n debe ser divisible por √p

#### Explicación del Código

**1. Organización en grilla 2D:**
```cpp
int sqrt_procesos = (int)sqrt(numero_procesos);
int fila_proceso = mi_rango / sqrt_procesos;  // Fila en la grilla
int col_proceso = mi_rango % sqrt_procesos;   // Columna en la grilla
```
- Procesos se organizan en grilla √p × √p
- Ejemplo con p=4: grilla 2×2
  ```
  P0 [0,0]  P1 [0,1]
  P2 [1,0]  P3 [1,1]
  ```

**2. Distribución de submatrices:**
```cpp
int filas_por_bloque = n / sqrt_procesos;
int cols_por_bloque = n / sqrt_procesos;

// Cada proceso recibe una submatriz
submatriz.resize(filas_por_bloque * cols_por_bloque);
```

**3. Cálculo local:**
```cpp
// Cada proceso multiplica su submatriz por su parte del vector
for (int i = 0; i < filas_por_bloque; i++) {
    for (int j = 0; j < cols_por_bloque; j++) {
        int fila_global = fila_proceso * filas_por_bloque + i;
        int col_global = col_proceso * cols_por_bloque + j;

        subresultado[i] += submatriz[i * cols_por_bloque + j] *
                           subvector[j];
    }
}
```

**4. Reducción por filas:**
```cpp
// Procesos en la misma fila reducen sus resultados
if (col_proceso == 0) {
    // Proceso 0 de cada fila recolecta
    for (int p = 1; p < sqrt_procesos; p++) {
        // Recibir y sumar
    }
}
```

**Ejemplo visual (p=4, n=4):**
```
Matriz 4×4 distribuida en grilla 2×2:

  [a00 a01 | a02 a03]     Vector: [x0]
  [a10 a11 | a12 a13]             [x1]
  ---------+---------             [x2]
  [a20 a21 | a22 a23]             [x3]
  [a30 a31 | a32 a33]

P0 [0,0]: submatriz [a00 a01; a10 a11], subvector [x0, x1]
P1 [0,1]: submatriz [a02 a03; a12 a13], subvector [x2, x3]
P2 [1,0]: submatriz [a20 a21; a30 a31], subvector [x0, x1]
P3 [1,1]: submatriz [a22 a23; a32 a33], subvector [x2, x3]

Ventaja: Mejor localidad de caché (datos contiguos en memoria)
```

#### Ejemplo de Ejecución (4 procesos en grilla 2×2, matriz 4×4)

```
Grilla de procesos: 2×2
Matriz A (4×4):
  11   12   13   14
  21   22   23   24
  31   32   33   34
  41   42   43   44

Distribución:
- P0 [0,0]: columnas 0-1, filas 0-1
- P1 [0,1]: columnas 2-3, filas 0-1
- P2 [1,0]: columnas 0-1, filas 2-3
- P3 [1,1]: columnas 2-3, filas 2-3

=== RESULTADO ===
y[0] = 130.0
y[1] = 230.0
y[2] = 330.0
y[3] = 430.0
```

**Observaciones:**
- Mejor para matrices grandes (mejor uso de caché)
- Comunicación más compleja que distribución por columnas
- Escalabilidad bidimensional

---

### 7. Ping-Pong con Medición de Tiempo (`3_7_ping_pong_tiempo.cpp`)

**Descripción:** Mide latencia y ancho de banda entre dos procesos, comparando `clock()` vs `MPI_Wtime()`.

**Características principales:**
- Intercambio de mensajes entre proceso 0 y 1
- Comparación de métodos de medición de tiempo
- Medición de resolución y precisión de temporizadores
- 1 millón de iteraciones para precisión estadística

**Métricas calculadas:**
- Latencia (tiempo de ida y vuelta)
- Resolución de temporizadores
- Comparación de precisión

#### Explicación del Código

**1. Medición con clock():**
```cpp
inicio_clock = clock();

for (int i = 0; i < NUMERO_ITERACIONES; i++) {
    MPI_Send(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
    MPI_Recv(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 1, 0,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

fin_clock = clock();
tiempo_clock = double(fin_clock - inicio_clock) / CLOCKS_PER_SEC;
```
- `clock()` mide tiempo de CPU (no tiempo de pared)
- Resolución típica: microsegundos
- Puede ser inexacto para operaciones de I/O o comunicación

**2. Medición con MPI_Wtime():**
```cpp
double tick = MPI_Wtick();  // Obtener resolución del temporizador
inicio_mpi = MPI_Wtime();   // Tiempo de pared en segundos

for (int i = 0; i < NUMERO_ITERACIONES; i++) {
    MPI_Send(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
    MPI_Recv(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 1, 0,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

fin_mpi = MPI_Wtime();
tiempo_mpi = fin_mpi - inicio_mpi;
```
- `MPI_Wtime()` mide tiempo de pared (real)
- `MPI_Wtick()` retorna la resolución (típicamente 1 nanosegundo)
- Más apropiado para medir comunicación MPI

**3. Cálculo de latencia:**
```cpp
// Latencia = tiempo de round-trip / 2
double latencia = (tiempo_mpi / NUMERO_ITERACIONES) / 2.0;
```
- Round-trip incluye envío + recepción
- Latencia es solo el tiempo de un sentido

**4. Proceso 1 (responde al ping-pong):**
```cpp
if (mi_rango == 1) {
    for (int i = 0; i < NUMERO_ITERACIONES; i++) {
        MPI_Recv(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 0, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(mensaje.data(), TAMANO_MENSAJE, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }
}
```
- P1 simplemente hace eco de cada mensaje

**Comparación de temporizadores:**
```
clock():
  + Portátil (C estándar)
  - Mide tiempo de CPU, no tiempo real
  - Resolución limitada (~microsegundos)

MPI_Wtime():
  + Mide tiempo de pared (real)
  + Resolución muy alta (~nanosegundos)
  + Diseñado específicamente para MPI
  - Requiere MPI
```

#### Ejemplo de Ejecución (2 procesos, 1M iteraciones)

```
=== RESULTADOS ===
Tiempo total con clock():     0.697422 segundos
Tiempo total con MPI_Wtime(): 0.708152 segundos
Diferencia absoluta:          0.010730 segundos
Error relativo:               1.515163%

Tiempo promedio por round-trip:
  clock():     6.974220e-07 segundos
  MPI_Wtime(): 7.081517e-07 segundos

Latencia estimada (un sentido):
  clock():     3.487110e-07 segundos (~349 nanosegundos)
  MPI_Wtime(): 3.540758e-07 segundos (~354 nanosegundos)

=== ANÁLISIS DE PRECISIÓN ===
Resolución de MPI_Wtime(): 1.000000e-09 segundos (1 nanosegundo)
✅ Ambos métodos produjeron mediciones válidas

Recomendación: Use MPI_Wtime() para mediciones precisas en programas MPI
```

**Observaciones:**
- `MPI_Wtime()` tiene resolución de 1 nanosegundo
- Latencia de ~350ns en comunicación local
- Error relativo <2% entre ambos métodos
- `MPI_Wtime()` es más preciso y portable en MPI

---

### 8. Merge Sort Paralelo (`3_8_merge_sort_paralelo.cpp`)

**Descripción:** Implementa ordenamiento merge sort de forma paralela.

**Características principales:**
- Distribución de datos con `MPI_Scatter`
- Ordenamiento local en cada proceso (merge sort secuencial)
- Merge progresivo de resultados en proceso 0
- Verificación de correctitud del ordenamiento

**Funciones MPI utilizadas:**
- `MPI_Scatter`: Distribución de datos
- `MPI_Send`/`MPI_Recv`: Recolección de resultados
- `MPI_Barrier`: Sincronización para mostrar resultados

**Complejidad:**
- Local: O((n/p) log(n/p))
- Merge: O(n log p)

#### Explicación del Código

**1. Distribución de datos con Scatter:**
```cpp
int elementos_por_proceso = n_total / numero_procesos;
mi_porcion.resize(elementos_por_proceso);

MPI_Scatter(datos_completos.data(), elementos_por_proceso, MPI_INT,
            mi_porcion.data(), elementos_por_proceso, MPI_INT, 0, MPI_COMM_WORLD);
```
- Cada proceso recibe n/p elementos

**2. Ordenamiento local (Merge Sort secuencial):**
```cpp
void merge_sort_secuencial(std::vector<int>& arr, int izq, int der) {
    if (izq < der) {
        int medio = izq + (der - izq) / 2;
        merge_sort_secuencial(arr, izq, medio);      // Ordenar mitad izquierda
        merge_sort_secuencial(arr, medio + 1, der);  // Ordenar mitad derecha
        merge(arr, izq, medio, der);                 // Combinar
    }
}

// Cada proceso ordena su porción
merge_sort_secuencial(mi_porcion, 0, elementos_por_proceso - 1);
```
- Algoritmo recursivo clásico de merge sort
- Complejidad: O((n/p) log(n/p))

**3. Función merge para combinar listas ordenadas:**
```cpp
void merge(std::vector<int>& arr, int izq, int medio, int der) {
    // Crear vectores temporales
    std::vector<int> izquierda(medio - izq + 1);
    std::vector<int> derecha(der - medio);

    // Copiar datos
    for (int i = 0; i < n1; i++) izquierda[i] = arr[izq + i];
    for (int j = 0; j < n2; j++) derecha[j] = arr[medio + 1 + j];

    // Mezclar de vuelta al array original
    int i = 0, j = 0, k = izq;
    while (i < n1 && j < n2) {
        if (izquierda[i] <= derecha[j]) {
            arr[k++] = izquierda[i++];
        } else {
            arr[k++] = derecha[j++];
        }
    }

    // Copiar elementos restantes
    while (i < n1) arr[k++] = izquierda[i++];
    while (j < n2) arr[k++] = derecha[j++];
}
```

**4. Recolección y merge final en P0:**
```cpp
if (mi_rango == 0) {
    resultado_final = mi_porcion;  // Empezar con propia porción

    // Recibir y hacer merge con cada proceso
    for (int proceso = 1; proceso < numero_procesos; proceso++) {
        std::vector<int> porcion_recibida(elementos_por_proceso);
        MPI_Recv(porcion_recibida.data(), elementos_por_proceso, MPI_INT,
                 proceso, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Merge de dos listas ordenadas
        resultado_final = merge_dos_vectores(resultado_final, porcion_recibida);
    }
} else {
    // Otros procesos envían su porción ordenada
    MPI_Send(mi_porcion.data(), elementos_por_proceso, MPI_INT, 0, 0, MPI_COMM_WORLD);
}
```

**5. Función merge_dos_vectores:**
```cpp
std::vector<int> merge_dos_vectores(const std::vector<int>& vec1,
                                     const std::vector<int>& vec2) {
    std::vector<int> resultado;
    size_t i = 0, j = 0;

    // Mezclar mientras haya elementos en ambos vectores
    while (i < vec1.size() && j < vec2.size()) {
        if (vec1[i] <= vec2[j]) {
            resultado.push_back(vec1[i++]);
        } else {
            resultado.push_back(vec2[j++]);
        }
    }

    // Copiar elementos restantes
    while (i < vec1.size()) resultado.push_back(vec1[i++]);
    while (j < vec2.size()) resultado.push_back(vec2[j++]);

    return resultado;
}
```

**Proceso visual (4 procesos, 8 elementos):**
```
Datos originales: [38, 27, 43, 3, 9, 82, 10, 5]

Después de Scatter:
P0: [38, 27]  →  merge_sort  →  [27, 38]
P1: [43, 3]   →  merge_sort  →  [3, 43]
P2: [9, 82]   →  merge_sort  →  [9, 82]
P3: [10, 5]   →  merge_sort  →  [5, 10]

P0 hace merge progresivo:
  [27, 38] + [3, 43]   →  [3, 27, 38, 43]
  [3, 27, 38, 43] + [9, 82]  →  [3, 9, 27, 38, 43, 82]
  [3, 9, 27, 38, 43, 82] + [5, 10]  →  [3, 5, 9, 10, 27, 38, 43, 82]

Resultado final en P0: [3, 5, 9, 10, 27, 38, 43, 82]
```

**Limitaciones:**
- Fase de merge final es secuencial (cuello de botella)
- Mejor usar merge en árbol para mayor paralelismo
- Speedup limitado por fase secuencial (Ley de Amdahl)

#### Ejemplo de Ejecución (4 procesos, 20 elementos)

```
=== MERGE SORT PARALELO ===
Datos originales:
692 662 556 721 127 8 905 46 115 228 410 870 520 477 488 437 578 714 387 839

Cada proceso ordena su porción local (5 elementos):
- P0: [127, 556, 662, 692, 721]
- P1: [8, 46, 115, 228, 905]
- P2: [410, 477, 488, 520, 870]
- P3: [387, 437, 578, 714, 839]

P0 hace merge secuencial de todas las porciones ordenadas

=== RESULTADO FINAL ===
¿Está ordenado correctamente? ✅ SÍ
Datos ordenados:
8 46 115 127 228 387 410 437 477 488 520 556 578 662 692 714 721 839 870 905

Total de elementos ordenados: 20
```

**Observaciones:**
- Cada proceso ordena n/p elementos localmente
- Proceso 0 combina todos los resultados con merge
- Verificación automática con `std::is_sorted`
- Eficiente para grandes volúmenes de datos

---

### 9. Costo de Redistribución (`3_9_costo_redistribucion.cpp`)

**Descripción:** Analiza el costo de redistribuir datos entre distribución por bloques y cíclica.

**Características principales:**
- Medición de tiempo de conversión entre esquemas
- Cálculo de ancho de banda efectivo
- Análisis de overhead de comunicación
- Comparación bidireccional (bloques↔cíclico)

**Patrones de distribución:**
- **Por bloques**: Datos contiguos en cada proceso [0,1,2,3|4,5,6,7|...]
- **Cíclico**: Datos intercalados entre procesos [0,4,8,12|1,5,9,13|...]

#### Explicación del Código

**1. Inicialización con distribución por bloques:**
```cpp
int elementos_por_proceso = n_vector / numero_procesos;
vector_bloques.resize(elementos_por_proceso);

// Cada proceso inicializa su bloque contiguo
for (int i = 0; i < elementos_por_proceso; i++) {
    vector_bloques[i] = mi_rango * elementos_por_proceso + i + 1;
}
```
- P0: [1, 2, 3, ..., 250]
- P1: [251, 252, ..., 500]
- P2: [501, 502, ..., 750]
- P3: [751, 752, ..., 1000]

**2. Conversión de bloques a cíclico:**
```cpp
vector_ciclico.resize(elementos_por_proceso);
double inicio = MPI_Wtime();

// Usar MPI_Alltoall para redistribuir
std::vector<double> send_buffer(n_vector);
std::vector<double> recv_buffer(n_vector);

// Preparar datos para envío en patrón cíclico
for (int proceso = 0; proceso < numero_procesos; proceso++) {
    for (int i = 0; i < elementos_por_proceso; i++) {
        int indice_global = mi_rango * elementos_por_proceso + i;
        int proceso_destino = indice_global % numero_procesos;
        int posicion_destino = indice_global / numero_procesos;

        // Enviar a proceso_destino
        MPI_Send(...);
    }
}

double fin = MPI_Wtime();
double tiempo_bloques_a_ciclico = fin - inicio;
```

**Patrón de comunicación (4 procesos, 8 elementos):**
```
Distribución por bloques inicial:
P0: [0, 1]
P1: [2, 3]
P2: [4, 5]
P3: [6, 7]

Redistribución a cíclico:
P0 envía: 0→P0, 1→P1
P1 envía: 2→P2, 3→P3
P2 envía: 4→P0, 5→P1
P3 envía: 6→P2, 7→P3

Resultado cíclico:
P0: [0, 4]  (elementos 0, 4, 8, 12, ...)
P1: [1, 5]  (elementos 1, 5, 9, 13, ...)
P2: [2, 6]  (elementos 2, 6, 10, 14, ...)
P3: [3, 7]  (elementos 3, 7, 11, 15, ...)
```

**3. Conversión de cíclico a bloques (proceso inverso):**
```cpp
inicio = MPI_Wtime();

for (int i = 0; i < elementos_por_proceso; i++) {
    int indice_global = mi_rango + i * numero_procesos;
    int proceso_destino = indice_global / elementos_por_proceso;
    // Enviar de vuelta al proceso correspondiente
}

fin = MPI_Wtime();
double tiempo_ciclico_a_bloques = fin - inicio;
```

**4. Cálculo de ancho de banda:**
```cpp
// Calcular tamaño de datos transferidos
double datos_mb = (n_vector * sizeof(double)) / (1024.0 * 1024.0);

// Ancho de banda = datos / tiempo
double ancho_banda_bc = datos_mb / tiempo_bloques_a_ciclico;
double ancho_banda_cb = datos_mb / tiempo_ciclico_a_bloques;
```

**Por qué es importante medir esto:**
- Algunos algoritmos funcionan mejor con distribución por bloques
- Otros funcionan mejor con distribución cíclica
- El costo de redistribución puede eliminar las ventajas de rendimiento
- Decisión de diseño: ¿vale la pena redistribuir?

**Ejemplo de uso en algoritmos reales:**
```
Multiplicación matriz-vector por filas → mejor con bloques
Factorización LU → mejor con cíclico (balanceo de carga)
FFT → requiere redistribución (transpose)
```

#### Ejemplo de Ejecución (4 procesos, 1000 elementos)

```
=== MEDICIÓN DEL COSTO DE REDISTRIBUCIÓN ===
Tamaño del vector: 1000 elementos
Número de procesos: 4
Elementos por proceso: 250

Tiempos de redistribución:
  Bloques → Cíclica:  0.000082 segundos (82 microsegundos)
  Cíclica → Bloques:  0.000032 segundos (32 microsegundos)
  Tiempo promedio:    0.000057 segundos

Análisis del overhead:
  Datos transferidos: 0.007629 MB (~7.6 KB)
  Ancho de banda efectivo (B→C): 92.84 MB/s
  Ancho de banda efectivo (C→B): 239.96 MB/s

Nota: Este overhead debe considerarse al decidir la estrategia
      de distribución en algoritmos que requieren cambios.
```

**Observaciones:**
- Redistribución es rápida pero no gratuita
- Conversión cíclico→bloques es más eficiente
- Overhead de ~50-80 microsegundos para 1000 elementos
- Importante considerar este costo en algoritmos adaptativos
- El ancho de banda depende del patrón de comunicación

---

## Compilación

### Script de Compilación Automática

Se proporciona un script bash (`compilar.sh`) para compilar todos los programas:

```bash
# Dar permisos de ejecución
chmod +x compilar.sh

# Compilar todos los programas
./compilar.sh all

# Compilar un programa específico
./compilar.sh 3_1_histograma.cpp
```

### Compilación Manual

```bash
# Compilar un programa individual
mpic++ -std=c++11 -Wall -O2 3_1_histograma.cpp -o bin/3_1_histograma

# Ejecutar con 4 procesos
mpirun -np 4 bin/3_1_histograma
```

### Requisitos
- Compilador C++ con soporte para C++11 o superior
- Implementación de MPI (OpenMPI o MPICH)
- Sistema operativo: Linux/Unix/macOS

---

## Ejecución

### Sintaxis General

```bash
mpirun -np <número_de_procesos> bin/<nombre_programa>
```

### Ejemplos de Ejecución

```bash
# Histograma con 4 procesos
mpirun -np 4 bin/3_1_histograma

# Monte Carlo con 8 procesos
mpirun -np 8 bin/3_2_monte_carlo_pi

# Suma en árbol con 4 procesos
mpirun -np 4 bin/3_3_suma_arbol

# Suma mariposa con 8 procesos (potencia de 2 recomendada)
mpirun -np 8 bin/3_4_suma_mariposa

# Multiplicación matriz-vector con 4 procesos
mpirun -np 4 bin/3_5_matriz_vector_columnas

# Merge sort con 4 procesos
mpirun -np 4 bin/3_8_merge_sort_paralelo
```

---

## Patrones de Comunicación MPI

### 1. **Comunicación Colectiva**
- `MPI_Bcast`: Difusión desde un proceso a todos
- `MPI_Scatter`: Distribución de datos en partes iguales
- `MPI_Gather`: Recolección de datos en un proceso
- `MPI_Reduce`: Operación de reducción (suma, max, min, etc.)

### 2. **Comunicación Punto a Punto**
- `MPI_Send`: Envío bloqueante
- `MPI_Recv`: Recepción bloqueante
- `MPI_Sendrecv`: Envío y recepción simultáneos

### 3. **Sincronización**
- `MPI_Barrier`: Punto de sincronización entre todos los procesos

---

## Análisis de Rendimiento

### Speedup Teórico

Para un problema de tamaño n con p procesos:

**Speedup = T(1) / T(p)**

Donde:
- T(1) es el tiempo con 1 proceso
- T(p) es el tiempo con p procesos

### Eficiencia

**Eficiencia = Speedup / p**

Idealmente la eficiencia debería ser 1.0 (100%)

### Factores que Afectan el Rendimiento

1. **Overhead de comunicación**: Tiempo gastado en envío/recepción de mensajes
2. **Balanceo de carga**: Distribución equitativa del trabajo
3. **Latencia de red**: Tiempo de inicio de comunicación
4. **Ancho de banda**: Tasa de transferencia de datos
5. **Sincronización**: Tiempo esperando en barreras

---

## Consideraciones de Diseño

### Ventajas de MPI
- Portabilidad entre diferentes arquitecturas
- Escalabilidad a grandes números de procesos
- Control explícito de comunicación
- Rendimiento en clusters y supercomputadoras

### Desventajas de MPI
- Programación más compleja que memoria compartida
- Overhead de comunicación entre procesos
- Debugging más difícil
- Requiere diseño cuidadoso de distribución de datos

---

## Resumen de Resultados Experimentales

Todos los programas fueron ejecutados exitosamente en un sistema con las siguientes características:
- **Sistema Operativo**: Linux (WSL2)
- **Implementación MPI**: OpenMPI
- **Compilador**: g++ con soporte C++14
- **Configuración típica**: 4 procesos

### Tabla de Resultados

| Programa | Procesos | Tamaño Entrada | Tiempo/Resultado | Observaciones |
|----------|----------|----------------|------------------|---------------|
| Histograma | 4 | 10,000 datos | ~1000/bin | Distribución uniforme ✓ |
| Monte Carlo π | 4 | 10M lanzamientos | Error: 0.0003 | Precisión 99.99% ✓ |
| Suma Árbol | 4 | 4 valores | 2 pasos (log₂4) | Solo P0 tiene resultado ✓ |
| Suma Mariposa | 4 | 4 valores | 2 pasos (log₂4) | Todos tienen resultado ✓ |
| Mat-Vec Columnas | 4 | Matriz 8×8 | Correcto | 2 columnas/proceso ✓ |
| Mat-Vec Submatrices | 4 | Matriz 4×4 | Correcto | Grilla 2×2 ✓ |
| Ping-Pong | 2 | 1M iteraciones | Latencia: ~350ns | MPI_Wtime > clock() ✓ |
| Merge Sort | 4 | 20 elementos | Ordenado | Verificación automática ✓ |
| Redistribución | 4 | 1000 elementos | 50-80 μs | B→C más lento que C→B ✓ |

### Conclusiones de los Experimentos

1. **Histograma**: Distribución perfectamente balanceada con variación <10% entre bins
2. **Monte Carlo**: Error decrece con √n, como se esperaba teóricamente
3. **Comunicación en árbol vs mariposa**: Ambos O(log p), pero mariposa distribuye resultado
4. **Multiplicación matriz-vector**: Ambos métodos correctos, submatrices mejor para caché
5. **Timing**: MPI_Wtime() ofrece resolución de nanosegundos vs microsegundos de clock()
6. **Ordenamiento**: Speedup teórico vs práctico limitado por fase de merge secuencial
7. **Redistribución**: Overhead significativo que debe considerarse en diseño

---

## Referencias

1. **Pacheco, P.** (2011). *An Introduction to Parallel Programming*. Morgan Kaufmann.
2. **Gropp, W., Lusk, E., Skjellum, A.** (1999). *Using MPI: Portable Parallel Programming with the Message-Passing Interface*.
3. Documentación oficial de MPI: https://www.mpi-forum.org/
4. Tutorial de OpenMPI: https://www.open-mpi.org/

---

## Autor

Este conjunto de programas fue desarrollado como parte del laboratorio de Programación Paralela.

**Fecha:** 2025

---

## Licencia

Este proyecto es de uso educativo y académico.