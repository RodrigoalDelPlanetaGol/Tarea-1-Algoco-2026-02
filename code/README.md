# Documentación

## Entrega

La entrega se realiza vía **aula.usm.cl** en formato `.zip`.

## Multiplicación de matrices

### Programa principal

El programa principal se encuentra en:

matrix_multiplication/matrix_multiplication.cpp

Este programa lee los datos de entrada de matrices desde `data/matrix_input/`, ejecuta los algoritmos Naive y Strassen, mide el tiempo de ejecución y el uso de memoria y guarda las matrices resultantes en `data/matrix_output/` y las mediciones en `data/matrix_measurements/matrix_measurements.csv`

Para compilar y ejecutar el programa en Windows, se puede utilizar el botón de 'Run Code' (Ctrl+Alt+N) o bien, en Linux en una terminal con cd en `code/matrix_multiplication/` ejecutar el comando 'make' y luego 'make run' (Por favor, antes de ejecutar 'make' o 'make run', ejecutar 'make clean') Se recomienda encarecidamente optar por la ultima opción.

Como observación, los casos con n = 1024 fueron omitidos por su alto coste de tiempo y recursos.

### Scripts

Los scripts utilizados para generar y procesar los datos se encuentran en `matrix_multiplication/scripts/`.

'matrix_generator.py' genera los archivos de entrada utilizados para los códigos y los guarda en data/matrix_input/.

Se realizó una modificación en este script para utilizar rutas basadas en la ubicación del propio archivo, de modo que la generación de datos funcione independientemente del directorio desde el cual se ejecute el programa.

'plot_generator.py' lee las mediciones generadas por el programa principal (de `data/matrix_measurements/matrix_measurements.csv` para ser especificos) y genera los gráficos correspondientes, los cuales se guardan en data/plots/.

## Ordenamiento de arreglo unidimensional

Algoritmos: MergeSort, QuickSort, PatienceSort, std::sort.

### Programa principal

El programa principal se encuentra en:

sorting/sorting.cpp

Este programa lee los archivoss de los arreglos desde `data/array_input/`, ejecuta los cuatro algoritmos sobre los mismos casos de prueba, mide el tiempo de ejecución y el uso de memoria, y guarda los resultados en `data/array_output/`. Las mediciones se guardan en: `data/measurements/sorting_measurements.csv`

Para compilar y ejecutar el programa en Windows, se puede utilizar el botón de 'Run Code' (Ctrl+Alt+N) o bien, en Linux en una terminal con cd en `code/sorting/` ejecutar el comando 'make' y luego 'make run' (Por favor, antes de ejecutar 'make' o 'make run', ejecutar 'make clean') Se recomienda encarecidamente optar por la ultima opción.

Como observación, para el caso de 'QuickSort', los casos con tamaños extremadamente grandes, como los de n = 10^7, se omiten debido a su largo tiempo de ejecución.

### Scripts

Los scripts utilizados para generar los datos y los gráficos se encuentran en `sorting/scripts/`.

'array_generator.py' genera los arreglos utilizados como entrada para los códigos y los guarda en `data/array_input/`.

Al igual que en el generador de matrices, se modificaron las rutas del script para que la generación de datos no dependa del directorio desde el cual se ejecute.

'plot_generator.py' utiliza el archivo 'sorting_measurements.csv' para generar los gráficos de los experimentos y guardarlos en `data/plots/`.