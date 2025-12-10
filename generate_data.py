import multiprocessing
import os
import subprocess

# --- CONFIGURATION ---
GEM5_BIN = "build/RISCV/gem5.opt"
CONFIG_SCRIPT = "run_spectre.py"
OUTPUT_ROOT = "dataset_samples"
ALGORITHMS = ["aes", "qsort", "floyd", "crc"]
MODES = [
    "benign",
    "spectre_v1",
    "spectre_v2",
]  # v1 = Conditional, v2 = Indirect
SAMPLES_PER_MODE = (
    5  # Start with 5 samples each to test. Increase to 100+ later.
)


def run_simulation(algo, mode, iteration):
    """Runs a single Gem5 simulation."""

    # Define binary path
    binary_name = f"{algo}_{mode}"
    binary_path = os.path.join("benchmarks", algo, binary_name)

    # Check if binary exists
    print(binary_path)
    if not os.path.exists(binary_path):
        print(f"Skipping {binary_name} (File not found)")
        return

    # Define output directory for this run
    # Format: dataset_samples/aes/v1/run_0
    outdir = os.path.join(OUTPUT_ROOT, algo, mode, f"run_{iteration}")

    # Gem5 Command
    cmd = [
        GEM5_BIN,
        f"--outdir={outdir}",
        CONFIG_SCRIPT,
        binary_path,
        "SecretPassword",  # Argument for the binary
    ]

    try:
        # Run quietly (stdout to DEVNULL) to avoid cluttering terminal
        subprocess.run(
            cmd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=True,
        )
        print(f"Completed: {algo} [{mode}] - Run {iteration}")
    except subprocess.CalledProcessError:
        print(f"FAILED:    {algo} [{mode}] - Run {iteration}")


def main():
    tasks = []

    # Create list of tasks
    for algo in ALGORITHMS:
        for mode in MODES:
            for i in range(SAMPLES_PER_MODE):
                tasks.append((algo, mode, i))

    print(f"Starting generation of {len(tasks)} samples...")
    print(f"Algorithms: {ALGORITHMS}")
    print(f"Modes: {MODES}")

    # Run in parallel (Use 8-10 cores for your M4 chip)
    with multiprocessing.Pool(processes=8) as pool:
        pool.starmap(run_simulation, tasks)

    print("\nAll simulations finished.")
    print(f"Data stored in: {os.path.abspath(OUTPUT_ROOT)}")


if __name__ == "__main__":
    main()
