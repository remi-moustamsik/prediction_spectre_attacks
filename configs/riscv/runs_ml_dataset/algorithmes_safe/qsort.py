from typing import List


def qsort(arr: List[int]) -> List[int]:
    if len(arr) <= 1:
        return arr[:]
    pivot = arr[len(arr) // 2]
    left = [x for x in arr if x < pivot]
    middle = [x for x in arr if x == pivot]
    right = [x for x in arr if x > pivot]
    return qsort(left) + middle + qsort(right)


def run_qsort():
    print("=== Quicksort demo ===")
    arr = [5, 3, 8, 1, 2, 9, 4]
    print(f"Tableau original : {arr}")
    sorted_arr = qsort(arr)
    print(f"Tableau trié     : {sorted_arr}")


if __name__ == "__main__":
    run_qsort()
