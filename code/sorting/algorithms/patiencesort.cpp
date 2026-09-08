/*
 * Patience Sort basada en:
 * Wikibooks, "Algorithm Implementation - Patience sort".
 * https://en.wikibooks.org/wiki/Algorithm_Implementation/Sorting/Patience_sort
 *
 * Adaptada para la libreria vector y a los requerimientos de este proyecto 
 */

#include <vector>
#include <queue>
#include <functional>
#include <iostream>

void patienceSort(std::vector<int>& arr) {
    std::vector<std::vector<int>> piles;

    for (int value : arr) {
        int left = 0;
        int right = static_cast<int>(piles.size());

        while (left < right) {
            int mid = left + (right - left) / 2;

            if (piles[mid].back() >= value)
                right = mid;
            else
                left = mid + 1;
        }

        if (left == static_cast<int>(piles.size())) {
            piles.push_back({value});
        } else {
            piles[left].push_back(value);
        }
    }

    using Node = std::pair<int, int>;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> heap;

    for (int i = 0; i < static_cast<int>(piles.size()); ++i) {
        heap.emplace(piles[i].back(), i);
        piles[i].pop_back();
    }

    int index = 0;

    while (!heap.empty()) {
        auto [value, pileIndex] = heap.top();
        heap.pop();

        arr[index++] = value;

        if (!piles[pileIndex].empty()) {
            heap.emplace(piles[pileIndex].back(), pileIndex);
            piles[pileIndex].pop_back();
        }
    }
}