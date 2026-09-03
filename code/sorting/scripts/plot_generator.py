import os
import pandas as pd
import matplotlib.pyplot as plt


# ============================================================
# plot_generator.py - Sorting
#
# Lee:
#   data/measurements/sorting_measurements.csv
#
# Genera:
#   data/plots/
#
# Los gráficos muestran:
#   - tiempo vs tamaño
#   - memoria vs tamaño
#   - comparación por tipo de arreglo
# ============================================================


# ------------------------------------------------------------
# Rutas
# ------------------------------------------------------------

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

BASE_DIR = os.path.abspath(
    os.path.join(SCRIPT_DIR, "..")
)

MEASUREMENTS_DIR = os.path.join(
    BASE_DIR,
    "data",
    "measurements"
)

PLOTS_DIR = os.path.join(
    BASE_DIR,
    "data",
    "plots"
)

CSV_PATH = os.path.join(
    MEASUREMENTS_DIR,
    "sorting_measurements.csv"
)


# ------------------------------------------------------------
# Crear carpeta de salida
# ------------------------------------------------------------

os.makedirs(PLOTS_DIR, exist_ok=True)


# ------------------------------------------------------------
# Leer CSV
# ------------------------------------------------------------

if not os.path.exists(CSV_PATH):
    raise FileNotFoundError(
        f"No se encontró el archivo de mediciones:\n{CSV_PATH}"
    )

df = pd.read_csv(CSV_PATH)


# ------------------------------------------------------------
# Validar columnas
# ------------------------------------------------------------

columnas_requeridas = {
    "algoritmo",
    "n",
    "tipo",
    "dominio",
    "muestra",
    "tiempo_ms",
    "memoria_kb",
    "resultado_correcto"
}

faltantes = columnas_requeridas - set(df.columns)

if faltantes:
    raise ValueError(
        "Faltan columnas en el CSV: "
        + ", ".join(sorted(faltantes))
    )


# ------------------------------------------------------------
# Convertir columnas
# ------------------------------------------------------------

df["n"] = pd.to_numeric(df["n"])
df["tiempo_ms"] = pd.to_numeric(df["tiempo_ms"])
df["memoria_kb"] = pd.to_numeric(df["memoria_kb"])


# ------------------------------------------------------------
# Comprobar resultados correctos
# ------------------------------------------------------------

incorrectos = df[
    df["resultado_correcto"].astype(str).str.lower() != "true"
]

if not incorrectos.empty:

    print(
        "ADVERTENCIA: se encontraron "
        f"{len(incorrectos)} resultados incorrectos."
    )


# ============================================================
# 1. TIEMPO VS N POR TIPO DE ARREGLO
# ============================================================

for tipo in sorted(df["tipo"].unique()):

    datos_tipo = df[
        df["tipo"] == tipo
    ]

    plt.figure()

    for algoritmo in sorted(
        datos_tipo["algoritmo"].unique()
    ):

        datos_algoritmo = datos_tipo[
            datos_tipo["algoritmo"] == algoritmo
        ]

        agrupado = (
            datos_algoritmo
            .groupby("n")["tiempo_ms"]
            .mean()
            .sort_index()
        )

        plt.plot(
            agrupado.index,
            agrupado.values,
            marker="o",
            label=algoritmo
        )

    plt.xscale("log")
    plt.yscale("log")

    plt.xlabel("Tamaño del arreglo (n)")
    plt.ylabel("Tiempo (ms)")

    plt.title(
        f"Tiempo de ejecución según tamaño - {tipo}"
    )

    plt.legend()
    plt.grid(True)

    nombre = (
        f"sorting_tiempo_vs_n_{tipo}.png"
    )

    plt.savefig(
        os.path.join(
            PLOTS_DIR,
            nombre
        ),
        dpi=300,
        bbox_inches="tight"
    )

    plt.close()


# ============================================================
# 2. MEMORIA VS N POR TIPO DE ARREGLO
# ============================================================

for tipo in sorted(df["tipo"].unique()):

    datos_tipo = df[
        df["tipo"] == tipo
    ]

    plt.figure()

    for algoritmo in sorted(
        datos_tipo["algoritmo"].unique()
    ):

        datos_algoritmo = datos_tipo[
            datos_tipo["algoritmo"] == algoritmo
        ]

        agrupado = (
            datos_algoritmo
            .groupby("n")["memoria_kb"]
            .mean()
            .sort_index()
        )

        plt.plot(
            agrupado.index,
            agrupado.values,
            marker="o",
            label=algoritmo
        )

    plt.xscale("log")

    plt.xlabel("Tamaño del arreglo (n)")
    plt.ylabel("Memoria (KB)")

    plt.title(
        f"Uso de memoria según tamaño - {tipo}"
    )

    plt.legend()
    plt.grid(True)

    nombre = (
        f"sorting_memoria_vs_n_{tipo}.png"
    )

    plt.savefig(
        os.path.join(
            PLOTS_DIR,
            nombre
        ),
        dpi=300,
        bbox_inches="tight"
    )

    plt.close()


# ============================================================
# 3. TIEMPO POR TIPO DE ENTRADA
# ============================================================

for n in sorted(df["n"].unique()):

    datos_n = df[
        df["n"] == n
    ]

    plt.figure()

    tipos = sorted(
        datos_n["tipo"].unique()
    )

    for algoritmo in sorted(
        datos_n["algoritmo"].unique()
    ):

        datos_algoritmo = datos_n[
            datos_n["algoritmo"] == algoritmo
        ]

        agrupado = (
            datos_algoritmo
            .groupby("tipo")["tiempo_ms"]
            .mean()
        )

        valores = [
            agrupado.get(tipo, float("nan"))
            for tipo in tipos
        ]

        plt.plot(
            tipos,
            valores,
            marker="o",
            label=algoritmo
        )

    plt.xlabel("Tipo de arreglo")
    plt.ylabel("Tiempo (ms)")

    plt.title(
        f"Tiempo según tipo de entrada - n={n}"
    )

    plt.legend()
    plt.grid(True)

    nombre = (
        f"sorting_tiempo_por_tipo_n{n}.png"
    )

    plt.savefig(
        os.path.join(
            PLOTS_DIR,
            nombre
        ),
        dpi=300,
        bbox_inches="tight"
    )

    plt.close()


print(
    "Gráficos de sorting generados correctamente."
)

print(
    f"Directorio de salida: {PLOTS_DIR}"
)