#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
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
// matrix_multiplication.cpp
//
// Programa principal para realizar las mediciones experimentales
// de:
//
//   - Naive
//   - Strassen
//
// Casos:
//
//   n = 16
//   n = 64
//   n = 256
//
// Se omite:
//
//   n = 1024
//
// debido al tiempo excesivo de ejecución.
// ============================================================


namespace fs = std::filesystem;

using Matriz = std::vector<std::vector<int>>;
using Reloj = std::chrono::high_resolution_clock;




static const std::size_t MAX_MATRIX_N = 256;




Matriz naiveMultiply(
    const Matriz& A,
    const Matriz& B
);

Matriz strassen(
    const Matriz& A,
    const Matriz& B
);




static const fs::path BASE_DIR = fs::path("data");

static const fs::path CARPETA_ENTRADA =
    BASE_DIR / "matrix_input";

static const fs::path CARPETA_SALIDA =
    BASE_DIR / "matrix_output";

static const fs::path CARPETA_MEDICIONES =
    BASE_DIR / "measurements";

static const fs::path ARCHIVO_CSV =
    CARPETA_MEDICIONES / "matrix_measurements.csv";




struct MetadatosArchivo {

    std::size_t n = 0;

    std::string tipo;

    std::string dominio;

    std::string muestra;

    std::string lado;
};




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




struct CasoArchivo {

    fs::path ruta;

    MetadatosArchivo meta;
};


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




struct ResultadoEjecucion {

    Matriz matriz_resultado;

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

        for (
            std::size_t j = 0;
            j < fila.size();
            ++j
        ) {

            salida << fila[j];

            if (
                j + 1 <
                fila.size()
            ) {

                salida << ' ';
            }
        }

        salida << '\n';
    }
}




static bool matrices_iguales(
    const Matriz& A,
    const Matriz& B
) {

    if (
        A.size() !=
        B.size()
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < A.size();
        ++i
    ) {

        if (
            A[i].size() !=
            B[i].size()
        ) {

            return false;
        }


        for (
            std::size_t j = 0;
            j < A[i].size();
            ++j
        ) {

            if (
                A[i][j] !=
                B[i][j]
            ) {

                return false;
            }
        }
    }


    return true;
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


    while (
        enviados <
        cantidad
    ) {

        const ssize_t resultado =
            write(
                fd,
                datos + enviados,
                cantidad - enviados
            );


        if (
            resultado <= 0
        ) {

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


    while (
        recibidos <
        cantidad
    ) {

        const ssize_t resultado =
            read(
                fd,
                datos + recibidos,
                cantidad - recibidos
            );


        if (
            resultado <= 0
        ) {

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

    const Matriz& A,

    const Matriz& B,

    const std::function<Matriz(
        const Matriz&,
        const Matriz&
    )>& funcion

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


    

    if (pid == 0) {

        close(
            pipe_resultado[0]
        );


        try {

            

            const Matriz matriz_A =
                A;

            const Matriz matriz_B =
                B;


            

            const auto inicio =
                Reloj::now();


            

            Matriz resultado =
                funcion(
                    matriz_A,
                    matriz_B
                );


            

            const auto fin =
                Reloj::now();


            const long long tiempo_us =
                std::chrono::duration_cast<
                    std::chrono::microseconds
                >(
                    fin - inicio
                ).count();


            

            const std::uint64_t n =
                static_cast<std::uint64_t>(
                    resultado.size()
                );


            const std::uint64_t columnas =
                resultado.empty()
                    ? 0
                    : static_cast<std::uint64_t>(
                        resultado[0].size()
                    );


            

            escribir_todo(
                pipe_resultado[1],
                &n,
                sizeof(n)
            );


            escribir_todo(
                pipe_resultado[1],
                &columnas,
                sizeof(columnas)
            );


            

            escribir_todo(
                pipe_resultado[1],
                &tiempo_us,
                sizeof(tiempo_us)
            );


            

            for (
                std::size_t i = 0;
                i < resultado.size();
                ++i
            ) {

                if (
                    !resultado[i].empty()
                ) {

                    escribir_todo(
                        pipe_resultado[1],
                        resultado[i].data(),
                        resultado[i].size() *
                        sizeof(int)
                    );
                }
            }


            close(
                pipe_resultado[1]
            );


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




    std::uint64_t n = 0;

    std::uint64_t columnas = 0;


    leer_todo(
        pipe_resultado[0],
        &n,
        sizeof(n)
    );


    leer_todo(
        pipe_resultado[0],
        &columnas,
        sizeof(columnas)
    );




    long long tiempo_us = 0;


    leer_todo(
        pipe_resultado[0],
        &tiempo_us,
        sizeof(tiempo_us)
    );




    Matriz resultado(
        static_cast<std::size_t>(n),
        std::vector<int>(
            static_cast<std::size_t>(columnas)
        )
    );


    for (
        std::size_t i = 0;
        i < resultado.size();
        ++i
    ) {

        if (
            !resultado[i].empty()
        ) {

            leer_todo(
                pipe_resultado[0],
                resultado[i].data(),
                resultado[i].size() *
                sizeof(int)
            );
        }
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


    if (
        terminado < 0
    ) {

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


    ResultadoEjecucion ejecucion;


    ejecucion.matriz_resultado =
        std::move(resultado);


    ejecucion.tiempo_us =
        tiempo_us;


    

    ejecucion.memoria_kb =
        static_cast<long long>(
            uso.ru_maxrss
        );


    return ejecucion;


#else

    

    const auto inicio =
        Reloj::now();


    Matriz resultado =
        funcion(
            A,
            B
        );


    const auto fin =
        Reloj::now();


    ResultadoEjecucion ejecucion;


    ejecucion.matriz_resultado =
        std::move(resultado);


    ejecucion.tiempo_us =
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

        ejecucion.memoria_kb =
            static_cast<long long>(
                info.PeakWorkingSetSize /
                1024ULL
            );
    }

#endif


    return ejecucion;

#endif
}




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

    const Matriz* referencia,

    const fs::path& carpeta_salida

) {

    

    const ResultadoEjecucion ejecucion =
        ejecutar_experimentalmente(
            A,
            B,
            funcion
        );


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
        ejecucion.tiempo_us;


    medicion.tiempo_ms =
        static_cast<double>(
            ejecucion.tiempo_us
        ) / 1000.0;


    medicion.memoria_kb =
        ejecucion.memoria_kb;


    

    if (
        referencia == nullptr
    ) {

        medicion.resultado_correcto =
            true;
    }
    else {

        medicion.resultado_correcto =
            matrices_iguales(
                ejecucion.matriz_resultado,
                *referencia
            );
    }


    

    const fs::path archivo_salida =
        carpeta_salida /
        (
            nombre_caso +
            "_" +
            nombre_algoritmo +
            ".txt"
        );


    escribir_matriz(
        archivo_salida,
        ejecucion.matriz_resultado
    );


    medicion.archivo_salida =
        archivo_salida.string();


    return medicion;
}




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


static void escribir_medicion_csv(
    std::ofstream& salida,
    const Medicion& medicion
) {

    salida
        << medicion.caso << ','
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




static std::vector<CasoArchivo> listar_archivos(
    const fs::path& carpeta
) {

    if (
        !fs::exists(carpeta)
    ) {

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

        if (
            !entrada.is_regular_file()
        ) {

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
                << "[WARN] Nombre no reconocido: "
                << entrada.path().filename().string()
                << '\n';

            continue;
        }


        archivos.push_back({
            entrada.path(),
            meta
        });
    }


    

    std::sort(
        archivos.begin(),
        archivos.end(),

        [](
            const CasoArchivo& a,
            const CasoArchivo& b
        ) {

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




int main() {

    try {

        std::cout
            << "========================================\n"
            << "   MEDICIONES - MATRIX MULTIPLICATION\n"
            << "========================================\n\n";


        std::cout
            << "Límite experimental: n <= "
            << MAX_MATRIX_N
            << '\n';


#ifdef __linux__

        std::cout
            << "Plataforma de medición: Linux\n"
            << "Memoria: pico RSS por proceso hijo\n\n";

#elif defined(_WIN32)

        std::cout
            << "Plataforma: Windows\n"
            << "Advertencia: las mediciones definitivas "
               "de memoria deben realizarse en Linux.\n\n";

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
            << '\n';


        if (
            archivos.empty()
        ) {

            std::cerr
                << "No se encontraron archivos.\n";

            return 1;
        }


       

        std::map<
            ClaveCaso,
            std::pair<
                fs::path,
                fs::path
            >
        > pares;


        for (
            const auto& archivo :
            archivos
        ) {

            const ClaveCaso clave =
                clave_de(
                    archivo.meta
                );


            if (
                archivo.meta.lado ==
                "1"
            ) {

                pares[clave].first =
                    archivo.ruta;
            }
            else if (
                archivo.meta.lado ==
                "2"
            ) {

                pares[clave].second =
                    archivo.ruta;
            }
        }


        std::cout
            << "Casos detectados: "
            << pares.size()
            << '\n';


        

        std::size_t casos_procesados = 0;

        std::size_t casos_omitidos = 0;

        std::size_t mediciones_realizadas = 0;


        

        for (
            const auto& [clave, par] :
            pares
        ) {

            const std::string nombre_caso =
                std::to_string(
                    clave.n
                )
                + "_"
                + clave.tipo
                + "_"
                + clave.dominio
                + "_"
                + clave.muestra;


            

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


            

            if (
                clave.n >
                MAX_MATRIX_N
            ) {

                ++casos_omitidos;


                std::cout
                    << "[" 
                    << (
                        casos_procesados +
                        casos_omitidos
                    )
                    << "/"
                    << pares.size()
                    << "] "
                    << nombre_caso
                    << " -> OMITIDO"
                    << " (n="
                    << clave.n
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


            

            const Matriz A =
                leer_matriz(
                    par.first,
                    clave.n
                );


            const Matriz B =
                leer_matriz(
                    par.second,
                    clave.n
                );


            

            std::cout
                << "    Ejecutando naive... ";


            const Medicion medicion_naive =
                medir_algoritmo(

                    nombre_caso,

                    clave,

                    A,

                    B,

                    "naive",

                    naiveMultiply,

                    nullptr,

                    CARPETA_SALIDA
                );


            std::cout
                << medicion_naive.tiempo_ms
                << " ms, "
                << medicion_naive.memoria_kb
                << " KB";


            if (
                medicion_naive.resultado_correcto
            ) {

                std::cout
                    << " [OK]";
            }
            else {

                std::cout
                    << " [ERROR]";
            }


            std::cout << '\n';


            escribir_medicion_csv(
                csv,
                medicion_naive
            );


            csv.flush();


            ++mediciones_realizadas;


            

            const Matriz referencia =
                leer_matriz(
                    fs::path(
                        medicion_naive.archivo_salida
                    ),
                    clave.n
                );


            

            std::cout
                << "    Ejecutando strassen... ";


            const Medicion medicion_strassen =
                medir_algoritmo(

                    nombre_caso,

                    clave,

                    A,

                    B,

                    "strassen",

                    strassen,

                    &referencia,

                    CARPETA_SALIDA
                );


            std::cout
                << medicion_strassen.tiempo_ms
                << " ms, "
                << medicion_strassen.memoria_kb
                << " KB";


            if (
                medicion_strassen.resultado_correcto
            ) {

                std::cout
                    << " [OK]";
            }
            else {

                std::cout
                    << " [ERROR]";
            }


            std::cout << '\n';


            escribir_medicion_csv(
                csv,
                medicion_strassen
            );


            csv.flush();


            ++mediciones_realizadas;


            std::cout << '\n';
        }


        

        std::cout
            << "========================================\n"
            << "                FINALIZADO\n"
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