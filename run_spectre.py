# run_spectre.py
import os
import sys

import m5
from m5.objects import *

# 1. Check for arguments
if len(sys.argv) < 2:
    print("Usage: build/RISCV/gem5.opt run_spectre.py <path_to_binary> [args]")
    sys.exit(1)

binary = sys.argv[1]
# Get arguments for the binary (like the secret password), if any
binary_args = sys.argv[1:]

# 2. Initialize System
system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = "timing"
system.mem_ranges = [AddrRange("512MB")]

# 3. CRITICAL: Use Out-of-Order CPU for Speculative Execution
# The report confirms SimpleCPU fails[cite: 813]; O3CPU succeeds [cite: 860]
system.cpu = RiscvO3CPU()

# 4. Configure Caches (Essential for side-channel timing)
system.cpu.icache = Cache(
    size="32kB",
    assoc=2,
    tag_latency=2,
    data_latency=2,
    response_latency=2,
    mshrs=4,
    tgts_per_mshr=20,
)
system.cpu.dcache = Cache(
    size="32kB",
    assoc=2,
    tag_latency=2,
    data_latency=2,
    response_latency=2,
    mshrs=4,
    tgts_per_mshr=20,
)
system.l2bus = L2XBar()

# Hook up caches
system.cpu.icache.cpu_side = system.cpu.icache_port
system.cpu.dcache.cpu_side = system.cpu.dcache_port
system.cpu.icache.mem_side = system.l2bus.cpu_side_ports
system.cpu.dcache.mem_side = system.l2bus.cpu_side_ports

# 5. Connect Interrupts (Required for RISC-V SE mode)
system.cpu.createInterruptController()

# 6. Memory Bus & Controller
system.membus = SystemXBar()
system.l2bus.mem_side_ports = system.membus.cpu_side_ports
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports
system.system_port = system.membus.cpu_side_ports

# 7. Set the Workload
system.workload = SEWorkload.init_compatible(binary)

process = Process()
process.cmd = binary_args  # Passes binary + password to the simulated process
system.cpu.workload = process
system.cpu.createThreads()

# 8. Instantiate and Run
root = Root(full_system=False, system=system)
m5.instantiate()

print(f"Beginning simulation for {binary}...")
exit_event = m5.simulate()
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
