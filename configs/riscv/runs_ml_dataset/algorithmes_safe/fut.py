from typing import List


def fut_transform(signal: List[float]) -> List[float]:
    """
    FUT = transformée de type Walsh–Hadamard 1D.
    Longueur du signal : puissance de 2.
    """
    n = len(signal)
    if n & (n - 1) != 0:
        raise ValueError("La taille du signal doit être une puissance de 2.")

    out = signal[:]
    h = 1
    while h < n:
        step = h * 2
        for i in range(0, n, step):
            for j in range(i, i + h):
                x = out[j]
                y = out[j + h]
                out[j] = x + y
                out[j + h] = x - y
        h *= 2
    return out


def run_fut():
    print("=== FUT (Walsh–Hadamard) demo ===")
    signal = [1.0, 2.0, 3.0, 4.0]
    transformed = fut_transform(signal)
    print(f"Signal d'entrée : {signal}")
    print(f"Transformée    : {transformed}")


if __name__ == "__main__":
    run_fut()
