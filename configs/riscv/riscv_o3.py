# configs/riscv/riscv_o3.py

from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.resources.resource import BinaryResource
from gem5.simulate.simulator import Simulator


def main() -> None:
    # 1) Hiérarchie de caches : L1I + L1D privés, L2 privé
    cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
        l1d_size="32KiB",
        l1i_size="32KiB",
        l2_size="256KiB",
    )

    # 2) Mémoire DDR4 simple
    memory = SingleChannelDDR4_2400(size="512MiB")

    # 3) Processeur RISC-V O3 (out-of-order)
    processor = SimpleProcessor(
        cpu_type=CPUTypes.O3,
        isa=ISA.RISCV,
        num_cores=1,
    )

    # 4) Board "simple"
    board = SimpleBoard(
        clk_freq="3GHz",
        processor=processor,
        memory=memory,
        cache_hierarchy=cache_hierarchy,
    )

    # 5) Workload : binaire RISC-V de notre micro-benchmark
    #    Assure-toi que bench_spectre_like.riscv est dans le répertoire courant
    binary = BinaryResource(local_path="bench_spectre_like.riscv")
    board.set_se_binary_workload(binary=binary)

    # 6) Simulation
    simulator = Simulator(board=board)

    print(">>> Début de la simulation RISC-V O3 (Spectre-like bench)")
    simulator.run()
    print(">>> Fin de la simulation RISC-V O3")


# Point d'entrée utilisé par gem5
if __name__ == "__m5_main__":
    main()
