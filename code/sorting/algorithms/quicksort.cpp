/*
 * Quick Sort basada en:
 * GeeksforGeeks, "C++ Program for Quick Sort".
 * https://www.geeksforgeeks.org/cpp/cpp-program-for-quicksort/
 *
 * Adaptada a los requerimientos de este proyecto.
 *
 * Mejoras de la implementación:
 * - Selección de pivote mediante mediana de tres.
 * - Partición de tres vías para manejar eficientemente
 *   elementos repetidos.
 * - Reducción de la profundidad de recursión procesando
 *   iterativamente la partición más grande.
 */

#include <algorithm>
#include <utility>
#include <vector>


// ============================================================
// Mediana de tres
//
// Devuelve el valor que queda en el medio entre:
//
//   arr[a], arr[b], arr[c]
//
// Esto evita utilizar sistemáticamente el último elemento
// como pivote.
// ============================================================

static int medianOfThree(
    const std::vector<int>& arr,
    int a,
    int b,
    int c
) {
    if (arr[a] < arr[b]) {

        if (arr[b] < arr[c]) {
            return arr[b];
        }

        if (arr[a] < arr[c]) {
            return arr[c];
        }

        return arr[a];

    } else {

        if (arr[a] < arr[c]) {
            return arr[a];
        }

        if (arr[b] < arr[c]) {
            return arr[c];
        }

        return arr[b];
    }
}


// ============================================================
// Quick Sort con partición de tres vías
//
// Después de la partición:
//
//   [ low ... lt-1 ]       < pivot
//   [ lt  ... gt   ]       = pivot
//   [ gt+1 ... high ]      > pivot
//
// Esto evita hacer recursión sobre todos los elementos que
// tienen el mismo valor que el pivote.
// ============================================================

void quickSort(
    std::vector<int>& arr,
    int low,
    int high
) {

    while (low < high) {

        const int mid =
            low + (high - low) / 2;


        const int pivot =
            medianOfThree(
                arr,
                low,
                mid,
                high
            );


        // ----------------------------------------------------
        // Partición de tres vías
        // ----------------------------------------------------

        int lt = low;

        int i = low;

        int gt = high;


        while (i <= gt) {

            if (arr[i] < pivot) {

                std::swap(
                    arr[lt],
                    arr[i]
                );

                ++lt;
                ++i;

            } else if (arr[i] > pivot) {

                std::swap(
                    arr[i],
                    arr[gt]
                );

                --gt;

            } else {

                ++i;
            }
        }


        // ----------------------------------------------------
        // Tenemos:
        //
        // [low, lt-1] < pivot
        // [lt, gt]    = pivot
        // [gt+1,high] > pivot
        //
        // Procesamos recursivamente la partición menor.
        // La mayor se deja para la siguiente iteración.
        // ----------------------------------------------------

        const int leftSize =
            lt - low;

        const int rightSize =
            high - gt;


        if (leftSize < rightSize) {

            // Recursión sobre la izquierda.

            if (low < lt - 1) {

                quickSort(
                    arr,
                    low,
                    lt - 1
                );
            }


            // Continuamos iterativamente con la derecha.

            low = gt + 1;

        } else {

            // Recursión sobre la derecha.

            if (gt + 1 < high) {

                quickSort(
                    arr,
                    gt + 1,
                    high
                );
            }


            // Continuamos iterativamente con la izquierda.

            high = lt - 1;
        }
    }
}