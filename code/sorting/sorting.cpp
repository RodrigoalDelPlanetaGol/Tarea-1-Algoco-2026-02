#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

// ============================================================
// sorting.cpp
//
// Programa principal para realizar las mediciones experimentales
// de:
//
//   - Merge Sort
//   - Quick Sort
//   - Patience Sort
//   - std::sort
//
// Cada algoritmo recibe una copia independiente del arreglo.
//
//
// ============================================================

namespace fs = std::filesystem;

using Arreglo = std::vector<int>;
using Reloj = std::chrono::high_resolution_clock;


// ============================================================
// Límite para Quick Sort
//
// Los casos con n > 1.000.000 se omiten para Quick Sort.
// ============================================================

static const std::size_t QUICK_SORT_MAX_N = 1'000'000;



void mergeSort(
    Arreglo& arr,
    int left,
    int right
);

void quickSort(
    Arreglo& arr,
    int low,
    int high
);

void patienceSort(
    Arreglo& arr
);

std::vector<int> sortArray(
    std::vector<int>& arr
);


static const fs::path BASE_DIR = fs::path("data");

static const fs::path CARPETA_ENTRADA =
    BASE_DIR / "array_input";

static const fs::path CARPETA_SALIDA =
    BASE_DIR / "array_output";

static const fs::path CARPETA_MEDICIONES =
    BASE_DIR / "measurements";

static const fs::path ARCHIVO_CSV =
    CARPETA_MEDICIONES / "sorting_measurements.csv";




struct MetadatosArchivo {

    std::size_t n = 0;

    std::string tipo;

    std::string dominio;

    std::string muestra;
};



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



struct ResultadoEjecucion {

    Arreglo arreglo_ordenado;

    long long tiempo_us = 0;

    long long memoria_kb = -1;
};




static MetadatosArchivo extraer_metadatos(
    const fs::path& ruta
) {

    MetadatosArchivo meta;

    const std::string base =
        ruta.stem().string();

    const std::regex patron(
        R"(^([0-9]+)_([^_]+)_(D[0-9]+)_([a-zA-Z])$)"
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
    }

    return meta;
}



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

    for (
        std::size_t i = 0;
        i < arreglo.size();
        ++i
    ) {

        salida << arreglo[i];

        if (
            i + 1 <
            arreglo.size()
        ) {

            salida << ' ';
        }
    }

    salida << '\n';
}




static bool arreglo_ordenado(
    const Arreglo& arreglo
) {

    return std::is_sorted(
        arreglo.begin(),
        arreglo.end()
    );
}




static void ejecutar_merge(
    Arreglo& arreglo
) {

    if (!arreglo.empty()) {

        mergeSort(
            arreglo,
            0,
            static_cast<int>(
                arreglo.size()
            ) - 1
        );
    }
}


static void ejecutar_quick(
    Arreglo& arreglo
) {

    if (!arreglo.empty()) {

        quickSort(
            arreglo,
            0,
            static_cast<int>(
                arreglo.size()
            ) - 1
        );
    }
}


static void ejecutar_patience(
    Arreglo& arreglo
) {

    patienceSort(arreglo);
}


static void ejecutar_stdsort(
    Arreglo& arreglo
) {

    sortArray(arreglo);
}




#ifdef __linux__

static void escribir_todo(
    int fd,
    const void* buffer,
    std::size_t cantidad
) {

    const char* datos =
        static_cast<const char*>(buffer);

    std::size_t enviados = 0;

    while (enviados < cantidad) {

        const ssize_t resultado =
            write(
                fd,
                datos + enviados,
                cantidad - enviados
            );

        if (resultado <= 0) {

            throw std::runtime_error(
                "Error escribiendo en pipe."
            );
        }

        enviados +=
            static_cast<std::size_t>(
                resultado
            );
    }
}


static void leer_todo(
    int fd,
    void* buffer,
    std::size_t cantidad
) {

    char* datos =
        static_cast<char*>(buffer);

    std::size_t recibidos = 0;

    while (recibidos < cantidad) {

        const ssize_t resultado =
            read(
                fd,
                datos + recibidos,
                cantidad - recibidos
            );

        if (resultado <= 0) {

            throw std::runtime_error(
                "Error leyendo desde pipe."
            );
        }

        recibidos +=
            static_cast<std::size_t>(
                resultado
            );
    }
}

#endif


static ResultadoEjecucion ejecutar_experimentalmente(

    const Arreglo& entrada,

    const std::function<void(Arreglo&)>& funcion

) {

#ifdef __linux__

    int pipe_resultado[2];

    if (
        pipe(pipe_resultado) != 0
    ) {

        throw std::runtime_error(
            "No se pudo crear el pipe."
        );
    }


    const pid_t pid =
        fork();


    if (pid < 0) {

        close(pipe_resultado[0]);
        close(pipe_resultado[1]);

        throw std::runtime_error(
            "No se pudo crear el proceso hijo."
        );
    }


    // ========================================================
    // HIJO
    // ========================================================

    if (pid == 0) {

        close(
            pipe_resultado[0]
        );


        try {

            

            Arreglo trabajo =
                entrada;


            

            const auto inicio =
                Reloj::now();


            funcion(trabajo);


            const auto fin =
                Reloj::now();


            const long long tiempo_us =
                std::chrono::duration_cast<
                    std::chrono::microseconds
                >(
                    fin - inicio
                ).count();


            

            const std::uint64_t tamano =
                static_cast<std::uint64_t>(
                    trabajo.size()
                );


            escribir_todo(
                pipe_resultado[1],
                &tamano,
                sizeof(tamano)
            );


            escribir_todo(
                pipe_resultado[1],
                &tiempo_us,
                sizeof(tiempo_us)
            );


            

            if (tamano > 0) {

                escribir_todo(
                    pipe_resultado[1],
                    trabajo.data(),
                    trabajo.size() *
                    sizeof(int)
                );
            }


            close(
                pipe_resultado[1]
            );


            // Finalización normal
            _exit(0);
        }

        catch (...) {

            close(
                pipe_resultado[1]
            );

            _exit(1);
        }
    }


    

    close(
        pipe_resultado[1]
    );


    

    std::uint64_t tamano = 0;

    leer_todo(
        pipe_resultado[0],
        &tamano,
        sizeof(tamano)
    );


    

    long long tiempo_us = 0;

    leer_todo(
        pipe_resultado[0],
        &tiempo_us,
        sizeof(tiempo_us)
    );


    

    Arreglo resultado(
        static_cast<std::size_t>(
            tamano
        )
    );


    if (tamano > 0) {

        leer_todo(
            pipe_resultado[0],
            resultado.data(),
            resultado.size() *
            sizeof(int)
        );
    }


    close(
        pipe_resultado[0]
    );


    

    int estado = 0;

    struct rusage uso{};

    const pid_t terminado =
        wait4(
            pid,
            &estado,
            0,
            &uso
        );


    if (terminado < 0) {

        throw std::runtime_error(
            "Error esperando al proceso hijo."
        );
    }


    if (
        !WIFEXITED(estado) ||
        WEXITSTATUS(estado) != 0
    ) {

        throw std::runtime_error(
            "El proceso hijo terminó con error."
        );
    }


    ResultadoEjecucion resultado_ejecucion;

    resultado_ejecucion.arreglo_ordenado =
        std::move(resultado);

    resultado_ejecucion.tiempo_us =
        tiempo_us;


    // Linux entrega ru_maxrss en KB.

    resultado_ejecucion.memoria_kb =
        static_cast<long long>(
            uso.ru_maxrss
        );


    return resultado_ejecucion;


#else

    

    Arreglo trabajo =
        entrada;


    const auto inicio =
        Reloj::now();


    funcion(trabajo);


    const auto fin =
        Reloj::now();


    ResultadoEjecucion resultado;

    resultado.arreglo_ordenado =
        std::move(trabajo);


    resultado.tiempo_us =
        std::chrono::duration_cast<
            std::chrono::microseconds
        >(
            fin - inicio
        ).count();


#ifdef _WIN32

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

        resultado.memoria_kb =
            static_cast<long long>(
                info.PeakWorkingSetSize /
                1024ULL
            );
    }

#endif


    return resultado;

#endif
}




static Medicion medir_algoritmo(

    const std::string& nombre_archivo,

    const MetadatosArchivo& meta,

    const Arreglo& entrada,

    const std::string& nombre_algoritmo,

    const std::function<void(Arreglo&)>& funcion,

    const fs::path& carpeta_salida

) {

    const ResultadoEjecucion ejecucion =
        ejecutar_experimentalmente(
            entrada,
            funcion
        );


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


    medicion.tiempo_us =
        ejecucion.tiempo_us;


    medicion.tiempo_ms =
        static_cast<double>(
            ejecucion.tiempo_us
        ) / 1000.0;


    medicion.memoria_kb =
        ejecucion.memoria_kb;


    medicion.resultado_correcto =
        arreglo_ordenado(
            ejecucion.arreglo_ordenado
        );


   

    const std::string base =
        fs::path(
            nombre_archivo
        ).stem().string();


    const fs::path archivo_salida =
        carpeta_salida /
        (
            base +
            "_" +
            nombre_algoritmo +
            ".txt"
        );


    escribir_arreglo(
        archivo_salida,
        ejecucion.arreglo_ordenado
    );


    medicion.archivo_salida =
        archivo_salida.string();


    return medicion;
}




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


static void escribir_medicion_csv(
    std::ofstream& salida,
    const Medicion& medicion
) {

    salida
        << medicion.archivo_entrada << ','
        << medicion.algoritmo << ','
        << medicion.n << ','
        << medicion.tipo << ','
        << medicion.dominio << ','
        << medicion.muestra << ','
        << medicion.tiempo_us << ','
        << medicion.tiempo_ms << ','
        << medicion.memoria_kb << ','
        << (
            medicion.resultado_correcto
                ? "true"
                : "false"
        )
        << ','
        << medicion.archivo_salida
        << '\n';
}




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


        archivos.push_back(
            entrada.path()
        );
    }


    std::sort(
        archivos.begin(),
        archivos.end(),

        [](
            const fs::path& a,
            const fs::path& b
        ) {

            const auto meta_a =
                extraer_metadatos(a);

            const auto meta_b =
                extraer_metadatos(b);


            if (
                meta_a.n !=
                meta_b.n
            ) {

                return
                    meta_a.n <
                    meta_b.n;
            }


            return
                a.filename().string() <
                b.filename().string();
        }
    );


    return archivos;
}




int main() {

    try {

        std::cout
            << "========================================\n"
            << "       MEDICIONES EXPERIMENTALES\n"
            << "                SORTING\n"
            << "========================================\n\n";


#ifdef __linux__

        std::cout
            << "Plataforma de medicion: Linux\n"
            << "Memoria: pico RSS por proceso hijo\n\n";

#elif defined(_WIN32)

        std::cout
            << "Plataforma: Windows\n"
            << "ADVERTENCIA: las mediciones definitivas de "
               "memoria deben realizarse en Linux.\n\n";

#endif


        

        fs::create_directories(
            CARPETA_SALIDA
        );

        fs::create_directories(
            CARPETA_MEDICIONES
        );


        

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


        escribir_encabezado_csv(
            csv
        );


        

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


        

        std::size_t archivos_procesados = 0;

        std::size_t mediciones_realizadas = 0;

        std::size_t ejecuciones_omitidas = 0;


        for (
            const auto& ruta :
            archivos
        ) {

            const std::string nombre_archivo =
                ruta.filename().string();


            const MetadatosArchivo meta =
                extraer_metadatos(ruta);


            if (meta.n == 0) {

                std::cerr
                    << "[WARN] Nombre no reconocido: "
                    << nombre_archivo
                    << '\n';

                continue;
            }


            Arreglo arreglo =
                leer_arreglo(ruta);


            if (arreglo.empty()) {

                std::cerr
                    << "[WARN] Archivo vacío: "
                    << nombre_archivo
                    << '\n';

                continue;
            }


            if (
                arreglo.size() !=
                meta.n
            ) {

                std::cerr
                    << "[WARN] Tamaño inconsistente en "
                    << nombre_archivo
                    << ": nombre="
                    << meta.n
                    << ", datos="
                    << arreglo.size()
                    << '\n';
            }


            ++archivos_procesados;


            std::cout
                << "["
                << archivos_procesados
                << "/"
                << archivos.size()
                << "] "
                << nombre_archivo
                << " (n="
                << arreglo.size()
                << ")\n";


            

            for (
                const auto& algoritmo :
                algoritmos
            ) {


                

                if (
                    algoritmo.nombre == "quick" &&
                    arreglo.size() >
                    QUICK_SORT_MAX_N
                ) {

                    ++ejecuciones_omitidas;


                    std::cout
                        << "    "
                        << algoritmo.nombre
                        << " -> OMITIDO"
                        << " (n="
                        << arreglo.size()
                        << " > "
                        << QUICK_SORT_MAX_N
                        << ")\n";


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

                        algoritmo.funcion,

                        CARPETA_SALIDA
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


                std::cout
                    << ", "
                    << medicion.memoria_kb
                    << " KB";


                if (
                    medicion.resultado_correcto
                ) {

                    std::cout
                        << " [OK]";
                }
                else {

                    std::cout
                        << " [ERROR]";
                }


                std::cout << '\n';
            }


            std::cout << '\n';
        }


        

        std::cout
            << "========================================\n"
            << "                FINALIZADO\n"
            << "========================================\n"
            << "Archivos procesados: "
            << archivos_procesados
            << '\n'
            << "Mediciones realizadas: "
            << mediciones_realizadas
            << '\n'
            << "Ejecuciones omitidas: "
            << ejecuciones_omitidas
            << '\n'
            << "CSV generado: "
            << ARCHIVO_CSV.string()
            << '\n';


        return 0;

    }
    catch (
        const std::exception& e
    ) {

        std::cerr
            << "ERROR: "
            << e.what()
            << '\n';

        return 1;
    }
}