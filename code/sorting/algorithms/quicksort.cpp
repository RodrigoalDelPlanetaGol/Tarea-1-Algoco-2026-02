/*
 * Quick Sort basada en:
 * GeeksforGeeks, "C++ Program for Quick Sort".
 * https://www.geeksforgeeks.org/cpp/cpp-program-for-quicksort/
 *
 * Adaptada a los requerimientos de este proyecto
 */


#include <vector>
#include <utility>
#include <iostream>

int partition(std::vector<int>& arr, int low, int high) {
    int pivot = arr[high];

    int i = low - 1;

    for (int j = low; j < high; ++j) {
        if (arr[j] <= pivot) {
            ++i;
            std::swap(arr[i], arr[j]);
        }
    }

    std::swap(arr[i + 1], arr[high]);

    return i + 1;
}

void quickSort(std::vector<int>& arr, int low, int high) {
    if (low >= high)
        return;

    int pivotIndex = partition(arr, low, high);

    quickSort(arr, low, pivotIndex - 1);
    quickSort(arr, pivotIndex + 1, high);
}