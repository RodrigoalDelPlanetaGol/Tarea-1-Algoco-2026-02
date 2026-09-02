#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/resource.h>
#endif

// ============================================================
// sorting.cpp
//
// Programa principal para realizar las mediciones experimentales
// de los algoritmos de ordenamiento:
//
//   - Merge Sort
//   - Quick Sort
//   - Patience Sort
//   - std::sort
//
// Flujo:
//
//   1) Lee los archivos de data/array_input/
//   2) Extrae los metadatos desde el nombre del archivo
//   3) Ejecuta cada algoritmo sobre una copia del mismo arreglo
//   4) Mide tiempo y memoria
//   5) Verifica que el resultado esté correctamente ordenado
//   6) Guarda los arreglos ordenados en data/array_output/
//   7) Guarda las mediciones en data/measurements/
// ============================================================

namespace fs = std::filesystem;

using Arreglo = std::vector<int>;
using Reloj = std::chrono::high_resolution_clock;

// ------------------------------------------------------------
// Declaraciones de los algoritmos
// ------------------------------------------------------------
//
// Estas funciones están implementadas en:
//
//   algorithms/mergesort.cpp
//   algorithms/quicksort.cpp
//   algorithms/patiencesort.cpp
//   algorithms/sort.cpp
//
// ------------------------------------------------------------

void mergeSort(Arreglo& arr, int left, int right);
void quickSort(Arreglo& arr, int low, int high);
void patienceSort(Arreglo& arr);
std::vector<int> sortArray(std::vector<int>& arr);


// ------------------------------------------------------------
// Rutas
// ------------------------------------------------------------

static const fs::path BASE_DIR = fs::path("data");

static const fs::path CARPETA_ENTRADA =
    BASE_DIR / "array_input";

static const fs::path CARPETA_SALIDA =
    BASE_DIR / "array_output";

static const fs::path CARPETA_MEDICIONES =
    BASE_DIR / "measurements";

static const fs::path ARCHIVO_CSV =
    CARPETA_MEDICIONES / "sorting_measurements.csv";

// Quick Sort no se mide para arreglos extremadamente grandes,
// debido al tiempo de ejecución observado en estos casos.
static const std::size_t QUICK_SORT_MAX_N = 1'000'000;
// ------------------------------------------------------------
// Metadatos del archivo
//
// Formato:
//
//   {n}_{t}_{d}_{m}.txt
//
// Ejemplo:
//
//   1000_ascendente_D1_a.txt
//
// Donde:
//
//   n = cantidad de elementos
//   t = tipo de arreglo
//   d = dominio
//   m = muestra
// ------------------------------------------------------------

struct MetadatosArchivo {

    std::size_t n = 0;

    std::string tipo;

    std::string dominio;

    std::string muestra;
};


// ------------------------------------------------------------
// Resultado de una medición
// ------------------------------------------------------------

struct Medicion {

    std::string archivo_entrada;

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
// Extraer metadatos desde el nombre del archivo
// ------------------------------------------------------------

static MetadatosArchivo extraer_metadatos(
    const fs::path& ruta
) {

    MetadatosArchivo meta;

    const std::string base = ruta.stem().string();

    /*
     * Ejemplo:
     *
     * 1000_ascendente_D1_a
     *
     * grupos:
     *
     * 1 -> 1000
     * 2 -> ascendente
     * 3 -> D1
     * 4 -> a
     */

    const std::regex patron(
        R"(^([0-9]+)_([^_]+)_(D[0-9]+)_([a-zA-Z])$)"
    );

    std::smatch coincidencia;

    if (std::regex_match(base, coincidencia, patron)) {

        meta.n =
            static_cast<std::size_t>(
                std::stoull(coincidencia[1].str())
            );

        meta.tipo =
            coincidencia[2].str();

        meta.dominio =
            coincidencia[3].str();

        meta.muestra =
            coincidencia[4].str();
    }

    return meta;
}


// ------------------------------------------------------------
// Leer arreglo desde archivo
//
// Los archivos generados por array_generator.py contienen los
// elementos del arreglo separados por espacios.
// ------------------------------------------------------------

static Arreglo leer_arreglo(
    const fs::path& ruta
) {

    std::ifstream entrada(ruta);

    if (!entrada) {

        throw std::runtime_error(
            "No se pudo abrir el archivo: " +
            ruta.string()
        );
    }

    Arreglo arreglo;

    long long valor;

    while (entrada >> valor) {

        arreglo.push_back(
            static_cast<int>(valor)
        );
    }

    return arreglo;
}


// ------------------------------------------------------------
// Escribir arreglo ordenado
// ------------------------------------------------------------

static void escribir_arreglo(
    const fs::path& ruta,
    const Arreglo& arreglo
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

    for (std::size_t i = 0;
         i < arreglo.size();
         ++i) {

        salida << arreglo[i];

        if (i + 1 < arreglo.size()) {
            salida << ' ';
        }
    }

    salida << '\n';
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

    if (getrusage(RUSAGE_SELF, &uso) == 0) {

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
// Funciones wrapper
//
// Todas reciben exactamente:
//
//     Arreglo&
//
// Esto permite utilizar el mismo mecanismo de medición para
// los cuatro algoritmos.
// ------------------------------------------------------------

static void ejecutar_merge(Arreglo& arreglo) {

    if (!arreglo.empty()) {

        mergeSort(
            arreglo,
            0,
            static_cast<int>(arreglo.size()) - 1
        );
    }
}


static void ejecutar_quick(Arreglo& arreglo) {

    if (!arreglo.empty()) {

        quickSort(
            arreglo,
            0,
            static_cast<int>(arreglo.size()) - 1
        );
    }
}


static void ejecutar_patience(Arreglo& arreglo) {

    patienceSort(arreglo);
}


static void ejecutar_stdsort(Arreglo& arreglo) {

    sortArray(arreglo);
}


// ------------------------------------------------------------
// Medir un algoritmo
// ------------------------------------------------------------

static Medicion medir_algoritmo(

    const std::string& nombre_archivo,

    const MetadatosArchivo& meta,

    const Arreglo& entrada,

    const std::string& nombre_algoritmo,

    const std::function<void(Arreglo&)>& funcion

) {

    // --------------------------------------------------------
    // Trabajamos sobre una copia.
    //
    // Así todos los algoritmos reciben exactamente el mismo
    // arreglo original.
    // --------------------------------------------------------

    Arreglo trabajo = entrada;


    // --------------------------------------------------------
    // Medición de memoria previa
    // --------------------------------------------------------

    const long long memoria_antes =
        memoria_actual_kb();


    // --------------------------------------------------------
    // Inicio de medición
    // --------------------------------------------------------

    const auto inicio =
        Reloj::now();


    // --------------------------------------------------------
    // EJECUCIÓN DEL ALGORITMO
    // --------------------------------------------------------

    funcion(trabajo);


    // --------------------------------------------------------
    // Fin de medición
    // --------------------------------------------------------

    const auto fin =
        Reloj::now();


    // --------------------------------------------------------
    // Memoria posterior
    // --------------------------------------------------------

    const long long memoria_despues =
        memoria_actual_kb();


    // --------------------------------------------------------
    // Construir resultado
    // --------------------------------------------------------

    Medicion medicion;

    medicion.archivo_entrada =
        nombre_archivo;

    medicion.algoritmo =
        nombre_algoritmo;

    medicion.n =
        entrada.size();

    medicion.tipo =
        meta.tipo;

    medicion.dominio =
        meta.dominio;

    medicion.muestra =
        meta.muestra;


    // Tiempo en microsegundos

    medicion.tiempo_us =
        std::chrono::duration_cast<
            std::chrono::microseconds
        >(fin - inicio).count();


    // Tiempo en milisegundos

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
                memoria_despues - memoria_antes
            );
    }


    // --------------------------------------------------------
    // Verificar que el algoritmo realmente ordenó
    // --------------------------------------------------------

    medicion.resultado_correcto =
        std::is_sorted(
            trabajo.begin(),
            trabajo.end()
        );


    // --------------------------------------------------------
    // Nombre del archivo de salida
    //
    // Ejemplo:
    //
    // 1000_ascendente_D1_a_merge.txt
    // --------------------------------------------------------

    const std::string base =
        fs::path(nombre_archivo).stem().string();

    const fs::path archivo_salida =
        CARPETA_SALIDA /
        (
            base +
            "_" +
            nombre_algoritmo +
            ".txt"
        );


    // La escritura NO forma parte de la medición

    escribir_arreglo(
        archivo_salida,
        trabajo
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
        << "archivo_entrada,"
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
// Escribir medición
// ------------------------------------------------------------

static void escribir_medicion_csv(
    std::ofstream& salida,
    const Medicion& m
) {

    salida
        << m.archivo_entrada << ','
        << m.algoritmo << ','
        << m.n << ','
        << m.tipo << ','
        << m.dominio << ','
        << m.muestra << ','
        << m.tiempo_us << ','
        << m.tiempo_ms << ','
        << m.memoria_kb << ','
        << (m.resultado_correcto ? "true" : "false")
        << ','
        << m.archivo_salida
        << '\n';
}


// ------------------------------------------------------------
// Listar archivos .txt
// ------------------------------------------------------------

static std::vector<fs::path> listar_archivos(
    const fs::path& carpeta
) {

    if (!fs::exists(carpeta)) {

        throw std::runtime_error(
            "No existe la carpeta de entrada: " +
            carpeta.string()
        );
    }


    std::vector<fs::path> archivos;


    for (const auto& entrada :
         fs::directory_iterator(carpeta)) {

        if (!entrada.is_regular_file()) {
            continue;
        }

        if (entrada.path().extension() != ".txt") {
            continue;
        }

        archivos.push_back(
            entrada.path()
        );
    }


    // Ordenar archivos por n y luego por nombre

    std::sort(
        archivos.begin(),
        archivos.end(),

        [](const fs::path& a, const fs::path& b) {

            const auto meta_a =
                extraer_metadatos(a);

            const auto meta_b =
                extraer_metadatos(b);


            if (meta_a.n != meta_b.n) {

                return meta_a.n < meta_b.n;
            }


            return
                a.filename().string() <
                b.filename().string();
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
            << "=====================================\n"
            << "   MEDICIONES - SORTING\n"
            << "=====================================\n\n";


        // ----------------------------------------------------
        // Crear carpetas de salida
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
            << "Archivos encontrados: "
            << archivos.size()
            << "\n\n";


        if (archivos.empty()) {

            std::cerr
                << "No se encontraron archivos .txt.\n";

            return 1;
        }


        // ----------------------------------------------------
        // Tabla de algoritmos
        // ----------------------------------------------------

        struct Algoritmo {

            std::string nombre;

            std::function<void(Arreglo&)> funcion;
        };


        const std::vector<Algoritmo> algoritmos = {

            {
                "merge",
                ejecutar_merge
            },

            {
                "quick",
                ejecutar_quick
            },

            {
                "patience",
                ejecutar_patience
            },

            {
                "stdsort",
                ejecutar_stdsort
            }
        };


        // ----------------------------------------------------
        // Procesar archivos
        // ----------------------------------------------------

        std::size_t archivos_procesados = 0;

        std::size_t mediciones_realizadas = 0;


        for (const auto& ruta :
             archivos) {


            const std::string nombre_archivo =
                ruta.filename().string();


            const MetadatosArchivo meta =
                extraer_metadatos(ruta);


            // ------------------------------------------------
            // Validar metadatos
            // ------------------------------------------------

            if (meta.n == 0) {

                std::cerr
                    << "[WARN] Nombre no reconocido: "
                    << nombre_archivo
                    << "\n";

                continue;
            }


            // ------------------------------------------------
            // Leer arreglo
            // ------------------------------------------------

            Arreglo arreglo =
                leer_arreglo(ruta);


            if (arreglo.empty()) {

                std::cerr
                    << "[WARN] Archivo vacío: "
                    << nombre_archivo
                    << "\n";

                continue;
            }


            if (arreglo.size() != meta.n) {

                std::cerr
                    << "[WARN] Tamaño inconsistente en "
                    << nombre_archivo
                    << ": nombre="
                    << meta.n
                    << ", datos="
                    << arreglo.size()
                    << "\n";
            }


            std::cout
                << "["
                << (++archivos_procesados)
                << "/"
                << archivos.size()
                << "] "
                << nombre_archivo
                << " (n="
                << arreglo.size()
                << ")\n";


            // ------------------------------------------------
            // Ejecutar los cuatro algoritmos
            // ------------------------------------------------

            for (const auto& algoritmo :
                 algoritmos) {

            // --------------------------------------------------------
            // Quick Sort: omitir tamaños extremadamente grandes
            // --------------------------------------------------------

            if(
                algoritmo.nombre == "quick" &&
                arreglo.size() > QUICK_SORT_MAX_N
            ){
                std::cout     
                    << " Omitiendo quick sort para ="
                    << arreglo.size()
                    << " (tiempo de ejecución excesivo esperado)\n";
                continue;
            }

                std::cout
                    << "    Ejecutando "
                    << algoritmo.nombre
                    << "... ";


                const Medicion medicion =
                    medir_algoritmo(

                        nombre_archivo,

                        meta,

                        arreglo,

                        algoritmo.nombre,

                        algoritmo.funcion
                    );


                escribir_medicion_csv(
                    csv,
                    medicion
                );


                csv.flush();


                ++mediciones_realizadas;


                std::cout
                    << medicion.tiempo_ms
                    << " ms";


                if (!medicion.resultado_correcto) {

                    std::cout
                        << " [ERROR: resultado incorrecto]";
                }
                else {

                    std::cout
                        << " [OK]";
                }


                std::cout << '\n';
            }


            std::cout << '\n';
        }


        // ----------------------------------------------------
        // Resumen
        // ----------------------------------------------------

        std::cout
            << "=====================================\n"
            << "           FINALIZADO\n"
            << "=====================================\n"
            << "Archivos procesados: "
            << archivos_procesados
            << "\n"
            << "Mediciones realizadas: "
            << mediciones_realizadas
            << "\n"
            << "CSV: "
            << ARCHIVO_CSV.string()
            << "\n";

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
