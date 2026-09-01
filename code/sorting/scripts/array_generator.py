import numpy as np
import os

def generar_arreglo(n, tipo, dominio):
    if dominio == "D1":
        valores = np.arange(10)
    elif dominio == "D7":
        valores = np.arange(10**7 + 1)
    else:
        raise ValueError("Dominio no reconocido")
    
    if tipo == "ascendente":
        return np.sort(np.random.choice(valores, n, replace=True))
    elif tipo == "descendente":
        return np.sort(np.random.choice(valores, n, replace=True))[::-1]
    elif tipo == "aleatorio":
        return np.random.choice(valores, n, replace=True)
    else:
        raise ValueError("Tipo de ordenamiento no reconocido")

def guardar_arreglo(nombre_archivo, arreglo):
    base_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(base_dir, "..", "data", "array_input")

    os.makedirs(output_dir, exist_ok=True)

    output_path = os.path.join(output_dir, nombre_archivo)

    with open(output_path, "w") as f:
        f.write(" ".join(map(str, arreglo)))

def generar_archivos():
    N = [10**1, 10**3, 10**5, 10**7] 
    T = ["ascendente", "descendente", "aleatorio"]
    D = ["D1", "D7"]
    M = ["a", "b", "c"]
    
    for n in N:
        for t in T:
            for d in D:
                for m in M:
                    nombre_archivo = f"{n}_{t}_{d}_{m}.txt"
                    arreglo = generar_arreglo(n, t, d)
                    guardar_arreglo(nombre_archivo, arreglo)
                    print(f"Generado: {nombre_archivo}")

if __name__ == "__main__":
    generar_archivos()
