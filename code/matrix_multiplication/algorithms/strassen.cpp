/*
 * Strassen matrix multiplication basada en:
 * GeeksforGeeks, "Strassen's Matrix Multiplication".
 * https://www.geeksforgeeks.org/dsa/strassens-matrix-multiplication/
 *
 *
 * Adaptada a matrices cuadradas de dimensión de potencias de dos.
 */

#include <vector>

using Matrix = std::vector<std::vector<int>>;

Matrix addMatrix(const Matrix& A, const Matrix& B) {
    int n = static_cast<int>(A.size());

    Matrix C(n, std::vector<int>(n));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            C[i][j] = A[i][j] + B[i][j];
        }
    }

    return C;
}

Matrix subtractMatrix(const Matrix& A, const Matrix& B) {
    int n = static_cast<int>(A.size());

    Matrix C(n, std::vector<int>(n));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            C[i][j] = A[i][j] - B[i][j];
        }
    }

    return C;
}

Matrix strassen(const Matrix& A, const Matrix& B) {
    int n = static_cast<int>(A.size());

    // Base case.
    if (n == 1) {
        return {{A[0][0] * B[0][0]}};
    }

    int half = n / 2;

    Matrix A11(half, std::vector<int>(half));
    Matrix A12(half, std::vector<int>(half));
    Matrix A21(half, std::vector<int>(half));
    Matrix A22(half, std::vector<int>(half));

    Matrix B11(half, std::vector<int>(half));
    Matrix B12(half, std::vector<int>(half));
    Matrix B21(half, std::vector<int>(half));
    Matrix B22(half, std::vector<int>(half));

    for (int i = 0; i < half; ++i) {
        for (int j = 0; j < half; ++j) {
            A11[i][j] = A[i][j];
            A12[i][j] = A[i][j + half];
            A21[i][j] = A[i + half][j];
            A22[i][j] = A[i + half][j + half];

            B11[i][j] = B[i][j];
            B12[i][j] = B[i][j + half];
            B21[i][j] = B[i + half][j];
            B22[i][j] = B[i + half][j + half];
        }
    }

    Matrix M1 = strassen(
        addMatrix(A11, A22),
        addMatrix(B11, B22)
    );

    Matrix M2 = strassen(
        addMatrix(A21, A22),
        B11
    );

    Matrix M3 = strassen(
        A11,
        subtractMatrix(B12, B22)
    );

    Matrix M4 = strassen(
        A22,
        subtractMatrix(B21, B11)
    );

    Matrix M5 = strassen(
        addMatrix(A11, A12),
        B22
    );

    Matrix M6 = strassen(
        subtractMatrix(A21, A11),
        addMatrix(B11, B12)
    );

    Matrix M7 = strassen(
        subtractMatrix(A12, A22),
        addMatrix(B21, B22)
    );

    Matrix C11 = addMatrix(
        subtractMatrix(
            addMatrix(M1, M4),
            M5
        ),
        M7
    );

    Matrix C12 = addMatrix(M3, M5);

    Matrix C21 = addMatrix(M2, M4);

    Matrix C22 = addMatrix(
        subtractMatrix(
            addMatrix(M1, M3),
            M2
        ),
        M6
    );

    Matrix C(n, std::vector<int>(n));

    for (int i = 0; i < half; ++i) {
        for (int j = 0; j < half; ++j) {
            C[i][j] = C11[i][j];
            C[i][j + half] = C12[i][j];
            C[i + half][j] = C21[i][j];
            C[i + half][j + half] = C22[i][j];
        }
    }

    return C;
}