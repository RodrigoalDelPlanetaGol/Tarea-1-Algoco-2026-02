/*
 * Naive matrix multiplication basada en:
 * GeeksforGeeks, "Multiply Two Matrices in C++".
 * https://www.geeksforgeeks.org/cpp/cpp-matrix-multiplication/
 *
 * Adaptada a las matrices cuadradas de este proyecto
 */

#include <vector>

using Matrix = std::vector<std::vector<int>>;

Matrix naiveMultiply(const Matrix& A, const Matrix& B) {
    int n = static_cast<int>(A.size());

    Matrix C(n, std::vector<int>(n, 0));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return C;
}