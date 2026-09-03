"""
Generador de gráficos para los experimentos de multiplicación
de matrices.

Lee:
    data/measurements/matrix_measurements.csv

Genera:
    data/plots/matrix_multiplication/

Algoritmos:
    - naive
    - strassen
"""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


EXPECTED_ALGORITHMS = [
    "naive",
    "strassen",
]


def parse_args() -> argparse.Namespace:

    parser = argparse.ArgumentParser(
        description=(
            "Generador de gráficos para "
            "multiplicación de matrices."
        )
    )

    parser.add_argument(
        "--csv",
        type=Path,
        required=True,
        help="Ruta al CSV de mediciones.",
    )

    parser.add_argument(
        "--out",
        type=Path,
        required=True,
        help="Carpeta de salida.",
    )

    return parser.parse_args()


def load_measurements(
    csv_path: Path,
) -> pd.DataFrame:

    if not csv_path.exists():

        raise FileNotFoundError(
            f"No existe el CSV: {csv_path}"
        )

    df = pd.read_csv(
        csv_path
    )

    required = {
        "caso",
        "algoritmo",
        "n",
        "tipo",
        "dominio",
        "muestra",
        "tiempo_ms",
        "memoria_kb",
        "resultado_correcto",
    }

    missing = (
        required -
        set(df.columns)
    )

    if missing:

        raise ValueError(
            "Faltan columnas en el CSV: "
            + ", ".join(
                sorted(missing)
            )
        )

    df = df.copy()

    df["n"] = pd.to_numeric(
        df["n"]
    )

    df["tiempo_ms"] = pd.to_numeric(
        df["tiempo_ms"]
    )

    df["memoria_kb"] = pd.to_numeric(
        df["memoria_kb"]
    )

    return df


def plot_time_vs_n(
    df: pd.DataFrame,
    out_dir: Path,
    tipo: str | None = None,
) -> None:

    if tipo is None:

        data = df

        filename = (
            "tiempo_ms_vs_n.png"
        )

        title = (
            "Multiplicación de matrices: "
            "tiempo vs tamaño"
        )

    else:

        data = df[
            df["tipo"] == tipo
        ]

        filename = (
            f"tiempo_ms_vs_n_{tipo}.png"
        )

        title = (
            "Multiplicación de matrices: "
            f"tiempo vs tamaño - {tipo}"
        )

    grouped = (
        data
        .groupby(
            ["algoritmo", "n"],
            as_index=False
        )["tiempo_ms"]
        .mean()
        .sort_values(
            ["algoritmo", "n"]
        )
    )

    plt.figure(
        figsize=(10, 6)
    )

    for algorithm in EXPECTED_ALGORITHMS:

        subset = grouped[
            grouped["algoritmo"]
            == algorithm
        ]

        if subset.empty:
            continue

        plt.plot(
            subset["n"],
            subset["tiempo_ms"],
            marker="o",
            label=algorithm,
        )

    plt.xlabel(
        "Dimensión de la matriz (n)"
    )

    plt.ylabel(
        "Tiempo promedio (ms)"
    )

    plt.title(
        title
    )

    plt.xscale("log")
    plt.yscale("log")

    plt.grid(
        True,
        which="both",
        alpha=0.3,
    )

    plt.legend()
    plt.tight_layout()

    plt.savefig(
        out_dir / filename,
        dpi=300,
    )

    plt.close()


def plot_memory_vs_n(
    df: pd.DataFrame,
    out_dir: Path,
    tipo: str | None = None,
) -> None:

    data = df[
        df["memoria_kb"].notna()
    ].copy()

    if tipo is None:

        filename = (
            "memoria_kb_vs_n.png"
        )

        title = (
            "Multiplicación de matrices: "
            "memoria vs tamaño"
        )

    else:

        data = data[
            data["tipo"] == tipo
        ]

        filename = (
            f"memoria_kb_vs_n_{tipo}.png"
        )

        title = (
            "Multiplicación de matrices: "
            f"memoria vs tamaño - {tipo}"
        )

    if data.empty:
        return

    grouped = (
        data
        .groupby(
            ["algoritmo", "n"],
            as_index=False
        )["memoria_kb"]
        .mean()
        .sort_values(
            ["algoritmo", "n"]
        )
    )

    plt.figure(
        figsize=(10, 6)
    )

    for algorithm in EXPECTED_ALGORITHMS:

        subset = grouped[
            grouped["algoritmo"]
            == algorithm
        ]

        if subset.empty:
            continue

        plt.plot(
            subset["n"],
            subset["memoria_kb"],
            marker="o",
            label=algorithm,
        )

    plt.xlabel(
        "Dimensión de la matriz (n)"
    )

    plt.ylabel(
        "Memoria promedio (KB)"
    )

    plt.title(
        title
    )

    plt.xscale("log")

    plt.grid(
        True,
        which="both",
        alpha=0.3,
    )

    plt.legend()
    plt.tight_layout()

    plt.savefig(
        out_dir / filename,
        dpi=300,
    )

    plt.close()


def plot_time_by_type(
    df: pd.DataFrame,
    out_dir: Path,
) -> None:

    for n in sorted(
        df["n"].unique()
    ):

        data_n = df[
            df["n"] == n
        ]

        grouped = (
            data_n
            .groupby(
                ["algoritmo", "tipo"],
                as_index=False
            )["tiempo_ms"]
            .mean()
        )

        plt.figure(
            figsize=(10, 6)
        )

        for algorithm in EXPECTED_ALGORITHMS:

            subset = grouped[
                grouped["algoritmo"]
                == algorithm
            ]

            if subset.empty:
                continue

            plt.plot(
                subset["tipo"],
                subset["tiempo_ms"],
                marker="o",
                label=algorithm,
            )

        plt.xlabel(
            "Tipo de matriz"
        )

        plt.ylabel(
            "Tiempo promedio (ms)"
        )

        plt.title(
            "Multiplicación de matrices: "
            f"tiempo según tipo - n={n}"
        )

        plt.grid(
            True,
            alpha=0.3,
        )

        plt.legend()
        plt.tight_layout()

        plt.savefig(
            out_dir /
            f"tiempo_por_tipo_n{n}.png",
            dpi=300,
        )

        plt.close()


def plot_time_by_domain(
    df: pd.DataFrame,
    out_dir: Path,
) -> None:

    for n in sorted(
        df["n"].unique()
    ):

        data_n = df[
            df["n"] == n
        ]

        grouped = (
            data_n
            .groupby(
                ["algoritmo", "dominio"],
                as_index=False
            )["tiempo_ms"]
            .mean()
        )

        plt.figure(
            figsize=(10, 6)
        )

        for algorithm in EXPECTED_ALGORITHMS:

            subset = grouped[
                grouped["algoritmo"]
                == algorithm
            ]

            if subset.empty:
                continue

            plt.plot(
                subset["dominio"],
                subset["tiempo_ms"],
                marker="o",
                label=algorithm,
            )

        plt.xlabel(
            "Dominio"
        )

        plt.ylabel(
            "Tiempo promedio (ms)"
        )

        plt.title(
            "Multiplicación de matrices: "
            f"tiempo según dominio - n={n}"
        )

        plt.grid(
            True,
            alpha=0.3,
        )

        plt.legend()
        plt.tight_layout()

        plt.savefig(
            out_dir /
            f"tiempo_por_dominio_n{n}.png",
            dpi=300,
        )

        plt.close()


def main() -> None:

    args = parse_args()

    out_dir = args.out

    out_dir.mkdir(
        parents=True,
        exist_ok=True,
    )

    df = load_measurements(
        args.csv
    )

    incorrect = df[
        df["resultado_correcto"]
        .astype(str)
        .str.lower()
        != "true"
    ]

    if not incorrect.empty:

        print(
            f"ADVERTENCIA: {len(incorrect)} "
            "resultados incorrectos."
        )

    plot_time_vs_n(
        df,
        out_dir,
    )

    plot_memory_vs_n(
        df,
        out_dir,
    )

    for tipo in sorted(
        df["tipo"].dropna().unique()
    ):

        plot_time_vs_n(
            df,
            out_dir,
            tipo,
        )

        plot_memory_vs_n(
            df,
            out_dir,
            tipo,
        )

    plot_time_by_type(
        df,
        out_dir,
    )

    plot_time_by_domain(
        df,
        out_dir,
    )

    print(
        "Gráficos de matrices generados en: "
        f"{out_dir}"
    )


if __name__ == "__main__":
    main()