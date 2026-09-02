#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/resource.h>
#endif

// ============================================================
// matrix_multiplication.cpp
//
// Programa principal para realizar las mediciones experimentales
// de los algoritmos de multiplicación de matrices:
//
//   - Naive
//   - Strassen
//
// Flujo:
//
//   1) Lee los archivos de data/matrix_input/
//   2) Agrupa los archivos _1 y _2 que pertenecen al mismo caso
//   3) Lee las matrices A y B
//   4) Ejecuta Naive y Strassen
//   5) Mide tiempo y memoria
//   6) Verifica la corrección del resultado
//   7) Guarda las matrices resultantes
//   8) Guarda las mediciones en un CSV
//
// Formato de entrada:
//
//   {n}_{t}_{d}_{m}_1.txt
//   {n}_{t}_{d}_{m}_2.txt
//
// Ejemplo:
//
//   16_densa_D0_a_1.txt
//   16_densa_D0_a_2.txt
//
// ============================================================

namespace fs = std::filesystem;

using Matriz = std::vector<std::vector<int>>;
using Reloj = std::chrono::high_resolution_clock;


// ------------------------------------------------------------
// Límite experimental
//
// Las matrices de n=1024 se omiten inicialmente para evitar
// tiempos de ejecución excesivos.
// Se medirán normalmente:
//     16, 64, 256
//
// Se podrá modificar fácilmente este límite posteriormente.
// ------------------------------------------------------------

static const std::size_t MAX_MATRIX_N = 256;


// ------------------------------------------------------------
// Declaraciones de los algoritmos
//
// Implementaciones ubicadas en:
//
//   algorithms/naive.cpp
//   algorithms/strassen.cpp
// ------------------------------------------------------------

Matriz naiveMultiply(
    const Matriz& A,
    const Matriz& B
);

Matriz strassen(
    const Matriz& A,
    const Matriz& B
);


// ------------------------------------------------------------
// Rutas
// ------------------------------------------------------------

static const fs::path BASE_DIR = fs::path("data");

static const fs::path CARPETA_ENTRADA =
    BASE_DIR / "matrix_input";

static const fs::path CARPETA_SALIDA =
    BASE_DIR / "matrix_output";

static const fs::path CARPETA_MEDICIONES =
    BASE_DIR / "measurements";

static const fs::path ARCHIVO_CSV =
    CARPETA_MEDICIONES / "matrix_measurements.csv";


// ------------------------------------------------------------
// Metadatos de un archivo
//
// Formato:
//
//   n_tipo_dominio_muestra_lado
//
// Ejemplo:
//
//   16_densa_D0_a_1
// ------------------------------------------------------------

struct MetadatosArchivo {

    std::size_t n = 0;

    std::string tipo;

    std::string dominio;

    std::string muestra;

    std::string lado;
};


// ------------------------------------------------------------
// Clave que identifica un caso completo
//
// Un caso está compuesto por:
//
//   n + tipo + dominio + muestra
//
// y posee dos archivos:
//
//   _1 -> matriz A
//   _2 -> matriz B
// ------------------------------------------------------------

struct ClaveCaso {

    std::size_t n = 0;

    std::string tipo;

    std::string dominio;

    std::string muestra;


    bool operator<(const ClaveCaso& other) const {

        return std::tie(
            n,
            tipo,
            dominio,
            muestra
        )
        <
        std::tie(
            other.n,
            other.tipo,
            other.dominio,
            other.muestra
        );
    }
};


// ------------------------------------------------------------
// Representación de un archivo asociado a un caso
// ------------------------------------------------------------

struct CasoArchivo {

    fs::path ruta;

    MetadatosArchivo meta;
};


// ------------------------------------------------------------
// Resultado de una medición
// ------------------------------------------------------------

struct Medicion {

    std::string caso;

    std::string algoritmo;

    std::size_t n = 0;

    std::string tipo;

    std::string dominio;

    std::string muestra;

    long long tiempo_us = 0;

    double tiempo_ms = 0.0;

    long long memoria_kb = -1;

    bool resultado_correcto = false;

    std::string archivo_salida;
};


// ------------------------------------------------------------
// Extraer metadatos desde el nombre
//
// Ejemplo:
//
//   16_densa_D0_a_1.txt
//
// grupos:
//
//   1 -> 16
//   2 -> densa
//   3 -> D0
//   4 -> a
//   5 -> 1
// ------------------------------------------------------------

static MetadatosArchivo extraer_metadatos(
    const fs::path& ruta
) {

    MetadatosArchivo meta;

    const std::string base =
        ruta.stem().string();


    const std::regex patron(
        R"(^([0-9]+)_([^_]+)_(D[0-9]+)_([a-zA-Z])_([12])$)"
    );


    std::smatch coincidencia;


    if (
        std::regex_match(
            base,
            coincidencia,
            patron
        )
    ) {

        meta.n =
            static_cast<std::size_t>(
                std::stoull(
                    coincidencia[1].str()
                )
            );

        meta.tipo =
            coincidencia[2].str();

        meta.dominio =
            coincidencia[3].str();

        meta.muestra =
            coincidencia[4].str();

        meta.lado =
            coincidencia[5].str();
    }


    return meta;
}


// ------------------------------------------------------------
// Convertir metadatos a clave de caso
// ------------------------------------------------------------

static ClaveCaso clave_de(
    const MetadatosArchivo& meta
) {

    return {
        meta.n,
        meta.tipo,
        meta.dominio,
        meta.muestra
    };
}


// ------------------------------------------------------------
// Leer matriz desde archivo
//
// Los archivos generados no incluyen dimensiones en la primera
// línea. Contienen directamente las filas de la matriz.
// ------------------------------------------------------------

static Matriz leer_matriz(
    const fs::path& ruta,
    std::size_t n
) {

    std::ifstream entrada(ruta);


    if (!entrada) {

        throw std::runtime_error(
            "No se pudo abrir el archivo: " +
            ruta.string()
        );
    }


    Matriz A(
        n,
        std::vector<int>(
            n,
            0
        )
    );


    for (std::size_t i = 0; i < n; ++i) {

        for (std::size_t j = 0; j < n; ++j) {

            if (!(entrada >> A[i][j])) {

                throw std::runtime_error(
                    "No se pudo leer la matriz completa desde: " +
                    ruta.string()
                );
            }
        }
    }


    return A;
}


// ------------------------------------------------------------
// Escribir matriz
//
// Se utiliza el mismo formato de los archivos de entrada:
// únicamente los valores de la matriz.
// ------------------------------------------------------------

static void escribir_matriz(
    const fs::path& ruta,
    const Matriz& matriz
) {

    std::ofstream salida(
        ruta,
        std::ios::trunc
    );


    if (!salida) {

        throw std::runtime_error(
            "No se pudo crear el archivo: " +
            ruta.string()
        );
    }


    for (const auto& fila : matriz) {

        for (std::size_t j = 0;
             j < fila.size();
             ++j) {

            salida << fila[j];

            if (j + 1 < fila.size()) {
                salida << ' ';
            }
        }

        salida << '\n';
    }
}


// ------------------------------------------------------------
// Comparar dos matrices
// ------------------------------------------------------------

static bool matrices_iguales(
    const Matriz& A,
    const Matriz& B
) {

    if (A.size() != B.size()) {
        return false;
    }


    for (std::size_t i = 0;
         i < A.size();
         ++i) {

        if (A[i].size() != B[i].size()) {
            return false;
        }


        for (std::size_t j = 0;
             j < A[i].size();
             ++j) {

            if (A[i][j] != B[i][j]) {
                return false;
            }
        }
    }


    return true;
}


// ------------------------------------------------------------
// Medición de memoria
// ------------------------------------------------------------

#ifdef _WIN32

static long long memoria_actual_kb() {

    PROCESS_MEMORY_COUNTERS_EX info{};


    if (
        GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<
                PROCESS_MEMORY_COUNTERS*
            >(&info),
            sizeof(info)
        )
    ) {

        return static_cast<long long>(
            info.WorkingSetSize / 1024ULL
        );
    }


    return -1;
}

#elif defined(__linux__)

static long long memoria_actual_kb() {

    struct rusage uso{};


    if (
        getrusage(
            RUSAGE_SELF,
            &uso
        ) == 0
    ) {

        return static_cast<long long>(
            uso.ru_maxrss
        );
    }


    return -1;
}

#else

static long long memoria_actual_kb() {
    return -1;
}

#endif


// ------------------------------------------------------------
// Medir un algoritmo
// ------------------------------------------------------------
//
// La medición considera solamente la ejecución del algoritmo.
//
// La escritura del archivo de salida queda FUERA de la medición.
// ------------------------------------------------------------

static Medicion medir_algoritmo(

    const std::string& nombre_caso,

    const ClaveCaso& caso,

    const Matriz& A,

    const Matriz& B,

    const std::string& nombre_algoritmo,

    const std::function<Matriz(
        const Matriz&,
        const Matriz&
    )>& funcion,

    const Matriz& referencia,

    const fs::path& carpeta_salida

) {

    // --------------------------------------------------------
    // Medición de memoria antes
    // --------------------------------------------------------

    const long long memoria_antes =
        memoria_actual_kb();


    // --------------------------------------------------------
    // Inicio
    // --------------------------------------------------------

    const auto inicio =
        Reloj::now();


    // --------------------------------------------------------
    // EJECUCIÓN DEL ALGORITMO
    // --------------------------------------------------------

    Matriz resultado =
        funcion(A, B);


    // --------------------------------------------------------
    // Fin
    // --------------------------------------------------------

    const auto fin =
        Reloj::now();


    // --------------------------------------------------------
    // Medición de memoria después
    // --------------------------------------------------------

    const long long memoria_despues =
        memoria_actual_kb();


    Medicion medicion;


    medicion.caso =
        nombre_caso;


    medicion.algoritmo =
        nombre_algoritmo;


    medicion.n =
        caso.n;


    medicion.tipo =
        caso.tipo;


    medicion.dominio =
        caso.dominio;


    medicion.muestra =
        caso.muestra;


    medicion.tiempo_us =
        std::chrono::duration_cast<
            std::chrono::microseconds
        >(fin - inicio).count();


    medicion.tiempo_ms =
        std::chrono::duration<double, std::milli>(
            fin - inicio
        ).count();


    // --------------------------------------------------------
    // Memoria
    // --------------------------------------------------------

    if (
        memoria_antes >= 0 &&
        memoria_despues >= 0
    ) {

        medicion.memoria_kb =
            std::max(
                0LL,
                memoria_despues -
                memoria_antes
            );
    }


    // --------------------------------------------------------
    // Verificación de corrección
    // --------------------------------------------------------

    medicion.resultado_correcto =
        matrices_iguales(
            resultado,
            referencia
        );


    // --------------------------------------------------------
    // Nombre de salida
    // --------------------------------------------------------

    const std::string base =
        nombre_caso;


    const fs::path archivo_salida =
        carpeta_salida /
        (
            base +
            "_" +
            nombre_algoritmo +
            ".txt"
        );


    // La escritura NO está dentro de la medición.

    escribir_matriz(
        archivo_salida,
        resultado
    );


    medicion.archivo_salida =
        archivo_salida.string();


    return medicion;
}


// ------------------------------------------------------------
// Escribir encabezado CSV
// ------------------------------------------------------------

static void escribir_encabezado_csv(
    std::ofstream& salida
) {

    salida
        << "caso,"
        << "algoritmo,"
        << "n,"
        << "tipo,"
        << "dominio,"
        << "muestra,"
        << "tiempo_us,"
        << "tiempo_ms,"
        << "memoria_kb,"
        << "resultado_correcto,"
        << "archivo_salida\n";
}


// ------------------------------------------------------------
// Escribir medición en CSV
// ------------------------------------------------------------

static void escribir_medicion_csv(
    std::ofstream& salida,
    const Medicion& m
) {

    salida
        << m.caso << ','
        << m.algoritmo << ','
        << m.n << ','
        << m.tipo << ','
        << m.dominio << ','
        << m.muestra << ','
        << m.tiempo_us << ','
        << m.tiempo_ms << ','
        << m.memoria_kb << ','
        << (
            m.resultado_correcto
                ? "true"
                : "false"
        )
        << ','
        << m.archivo_salida
        << '\n';
}


// ------------------------------------------------------------
// Listar todos los archivos .txt
// ------------------------------------------------------------

static std::vector<CasoArchivo> listar_archivos(
    const fs::path& carpeta
) {

    if (!fs::exists(carpeta)) {

        throw std::runtime_error(
            "No existe la carpeta de entrada: " +
            carpeta.string()
        );
    }


    std::vector<CasoArchivo> archivos;


    for (
        const auto& entrada :
        fs::directory_iterator(carpeta)
    ) {

        if (!entrada.is_regular_file()) {
            continue;
        }


        if (
            entrada.path().extension()
            != ".txt"
        ) {
            continue;
        }


        const MetadatosArchivo meta =
            extraer_metadatos(
                entrada.path()
            );


        if (
            meta.n == 0 ||
            meta.lado.empty()
        ) {

            std::cerr
                << "[WARN] Archivo con nombre no reconocido: "
                << entrada.path().filename().string()
                << '\n';

            continue;
        }


        archivos.push_back({
            entrada.path(),
            meta
        });
    }


    // --------------------------------------------------------
    // Ordenar por:
    //
    // n
    // tipo
    // dominio
    // muestra
    // lado
    // --------------------------------------------------------

    std::sort(
        archivos.begin(),
        archivos.end(),

        [](const CasoArchivo& a,
           const CasoArchivo& b) {

            if (
                a.meta.n !=
                b.meta.n
            ) {

                return
                    a.meta.n <
                    b.meta.n;
            }


            if (
                a.meta.tipo !=
                b.meta.tipo
            ) {

                return
                    a.meta.tipo <
                    b.meta.tipo;
            }


            if (
                a.meta.dominio !=
                b.meta.dominio
            ) {

                return
                    a.meta.dominio <
                    b.meta.dominio;
            }


            if (
                a.meta.muestra !=
                b.meta.muestra
            ) {

                return
                    a.meta.muestra <
                    b.meta.muestra;
            }


            return
                a.meta.lado <
                b.meta.lado;
        }
    );


    return archivos;
}


// ------------------------------------------------------------
// MAIN
// ------------------------------------------------------------

int main() {

    try {

        std::cout
            << "========================================\n"
            << "   MEDICIONES - MATRIX MULTIPLICATION\n"
            << "========================================\n\n";


        std::cout
            << "Límite experimental: n <= "
            << MAX_MATRIX_N
            << "\n\n";


        // ----------------------------------------------------
        // Crear carpetas
        // ----------------------------------------------------

        fs::create_directories(
            CARPETA_SALIDA
        );

        fs::create_directories(
            CARPETA_MEDICIONES
        );


        // ----------------------------------------------------
        // Abrir CSV
        // ----------------------------------------------------

        std::ofstream csv(
            ARCHIVO_CSV,
            std::ios::out |
            std::ios::trunc
        );


        if (!csv) {

            throw std::runtime_error(
                "No se pudo crear el CSV: " +
                ARCHIVO_CSV.string()
            );
        }


        escribir_encabezado_csv(csv);


        // ----------------------------------------------------
        // Obtener archivos
        // ----------------------------------------------------

        const auto archivos =
            listar_archivos(
                CARPETA_ENTRADA
            );


        std::cout
            << "Archivos de entrada encontrados: "
            << archivos.size()
            << "\n";


        if (archivos.empty()) {

            std::cerr
                << "No se encontraron archivos de entrada.\n";

            return 1;
        }


        // ----------------------------------------------------
        // Agrupar archivos en pares
        // ----------------------------------------------------

        std::map<
            ClaveCaso,
            std::pair<
                fs::path,
                fs::path
            >
        > pares;


        for (const auto& archivo :
             archivos) {

            const ClaveCaso clave =
                clave_de(
                    archivo.meta
                );


            if (archivo.meta.lado == "1") {

                pares[clave].first =
                    archivo.ruta;
            }
            else {

                pares[clave].second =
                    archivo.ruta;
            }
        }


        std::cout
            << "Casos detectados: "
            << pares.size()
            << "\n\n";


        // ----------------------------------------------------
        // Procesar cada caso
        // ----------------------------------------------------

        std::size_t casos_procesados = 0;

        std::size_t mediciones_realizadas = 0;

        std::size_t casos_omitidos = 0;


        for (
            const auto& [clave, par] :
            pares
        ) {

            const std::string nombre_caso =
                std::to_string(clave.n) +
                "_" +
                clave.tipo +
                "_" +
                clave.dominio +
                "_" +
                clave.muestra;


            // ------------------------------------------------
            // Verificar que exista A y B
            // ------------------------------------------------

            if (
                par.first.empty() ||
                par.second.empty()
            ) {

                std::cerr
                    << "[WARN] Caso incompleto: "
                    << nombre_caso
                    << '\n';

                continue;
            }


            // ------------------------------------------------
            // Aplicar límite experimental
            // ------------------------------------------------

            if (
                clave.n >
                MAX_MATRIX_N
            ) {

                ++casos_omitidos;


                std::cout
                    << "["
                    << (casos_procesados +
                        casos_omitidos)
                    << "/"
                    << pares.size()
                    << "] "
                    << nombre_caso
                    << " -> OMITIDO"
                    << " (n="
                    << clave.n
                    << " > "
                    << MAX_MATRIX_N
                    << ")\n";


                continue;
            }


            ++casos_procesados;


            std::cout
                << "["
                << casos_procesados
                << "/"
                << pares.size()
                << "] "
                << nombre_caso
                << '\n';


            // ------------------------------------------------
            // Leer matrices
            // ------------------------------------------------

            Matriz A =
                leer_matriz(
                    par.first,
                    clave.n
                );


            Matriz B =
                leer_matriz(
                    par.second,
                    clave.n
                );


            // ------------------------------------------------
            // Naive de referencia
            //
            // Se calcula fuera de las mediciones.
            //
            // Sirve para comprobar que tanto Naive como
            // Strassen producen el resultado correcto.
            // ------------------------------------------------

            std::cout
                << "    Calculando referencia naive... ";


            const auto inicio_referencia =
                Reloj::now();


            const Matriz referencia =
                naiveMultiply(
                    A,
                    B
                );


            const auto fin_referencia =
                Reloj::now();


            const double tiempo_referencia =
                std::chrono::duration<double, std::milli>(
                    fin_referencia -
                    inicio_referencia
                ).count();


            std::cout
                << tiempo_referencia
                << " ms\n";


            // ------------------------------------------------
            // Medir Naive
            // ------------------------------------------------

            std::cout
                << "    Ejecutando naive... ";


            const Medicion m1 =
                medir_algoritmo(

                    nombre_caso,

                    clave,

                    A,

                    B,

                    "naive",

                    naiveMultiply,

                    referencia,

                    CARPETA_SALIDA
                );


            std::cout
                << m1.tiempo_ms
                << " ms";


            if (m1.resultado_correcto) {
                std::cout << " [OK]";
            }
            else {
                std::cout
                    << " [ERROR]";
            }


            std::cout << '\n';


            escribir_medicion_csv(
                csv,
                m1
            );


            csv.flush();


            ++mediciones_realizadas;


            // ------------------------------------------------
            // Medir Strassen
            // ------------------------------------------------

            std::cout
                << "    Ejecutando strassen... ";


            const Medicion m2 =
                medir_algoritmo(

                    nombre_caso,

                    clave,

                    A,

                    B,

                    "strassen",

                    strassen,

                    referencia,

                    CARPETA_SALIDA
                );


            std::cout
                << m2.tiempo_ms
                << " ms";


            if (m2.resultado_correcto) {
                std::cout << " [OK]";
            }
            else {
                std::cout
                    << " [ERROR]";
            }


            std::cout << '\n';


            escribir_medicion_csv(
                csv,
                m2
            );


            csv.flush();


            ++mediciones_realizadas;


            std::cout << '\n';
        }


        // ----------------------------------------------------
        // Resumen
        // ----------------------------------------------------

        std::cout
            << "========================================\n"
            << "              FINALIZADO\n"
            << "========================================\n"
            << "Casos procesados: "
            << casos_procesados
            << '\n'
            << "Casos omitidos: "
            << casos_omitidos
            << '\n'
            << "Mediciones realizadas: "
            << mediciones_realizadas
            << '\n'
            << "CSV: "
            << ARCHIVO_CSV.string()
            << '\n';


        return 0;
    }

    catch (const std::exception& e) {

        std::cerr
            << "ERROR: "
            << e.what()
            << '\n';

        return 1;
    }
}