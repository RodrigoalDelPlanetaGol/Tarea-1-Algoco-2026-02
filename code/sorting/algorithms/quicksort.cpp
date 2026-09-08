/*
 * Quick Sort basada en:
 * GeeksforGeeks, "C++ Program for Quick Sort".
 * https://www.geeksforgeeks.org/cpp/cpp-program-for-quicksort/
 *
 * Adaptada a los requerimientos de este proyecto con ajustes para el manejo de arreglos grandes.
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


        const int leftSize =
            lt - low;

        const int rightSize =
            high - gt;


        if (leftSize < rightSize) {


            if (low < lt - 1) {

                quickSort(
                    arr,
                    low,
                    lt - 1
                );
            }




            low = gt + 1;

        } else {



            if (gt + 1 < high) {

                quickSort(
                    arr,
                    gt + 1,
                    high
                );
            }




            high = lt - 1;
        }
    }
}