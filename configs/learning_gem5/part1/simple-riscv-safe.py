import os

import m5
from m5.objects import *

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = "timing"
system.mem_ranges = [AddrRange("512MiB")]

# --- CPU O3 + prédicteur de branches explicite ---
system.cpu = RiscvO3CPU()

system.cpu.branchPred = BiModeBP()

# (optionnel) un peu plus de profondeur pour favoriser la spéculation
# system.cpu.numROBEntries = 192
# system.cpu.numIQEntries  = 64
# system.cpu.numLoadQueueEntries  = 32
# system.cpu.numStoreQueueEntries = 32

system.membus = SystemXBar()


class L1_DCache(Cache):
    tag_latency = 4
    data_latency = 4
    response_latency = 4
    size = "32kB"
    assoc = 8


class L1_ICache(Cache):
    tag_latency = 4
    data_latency = 4
    response_latency = 4
    size = "32kB"
    assoc = 8


class L2Cache(Cache):
    tag_latency = 20
    data_latency = 20
    response_latency = 20
    size = "256kB"
    assoc = 8


system.cpu.dcache = L1_DCache(mshrs=4, tgts_per_mshr=8)
system.cpu.icache = L1_ICache(mshrs=4, tgts_per_mshr=8)
system.l2cache = L2Cache(mshrs=16, tgts_per_mshr=8)
system.l2bus = L2XBar(width=32)

system.cpu.icache_port = system.cpu.icache.cpu_side
system.cpu.dcache_port = system.cpu.dcache.cpu_side

system.cpu.createInterruptController()

system.cpu.icache.mem_side = system.l2bus.cpu_side_ports
system.cpu.dcache.mem_side = system.l2bus.cpu_side_ports

system.l2cache.cpu_side = system.l2bus.mem_side_ports
system.l2cache.mem_side = system.membus.cpu_side_ports

system.mem_latency_bus = CoherentXBar()
system.mem_latency_bus.frontend_latency = 200
system.mem_latency_bus.forward_latency = 200
system.mem_latency_bus.response_latency = 200
system.mem_latency_bus.snoop_response_latency = 200
system.mem_latency_bus.width = 64

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]

system.membus.mem_side_ports = system.mem_latency_bus.cpu_side_ports
system.mem_ctrl.port = system.mem_latency_bus.mem_side_ports

system.system_port = system.membus.cpu_side_ports

thispath = os.path.dirname(os.path.realpath(__file__))
binary = os.path.join(
    thispath,
    "../../../",
    "./bench_spectre_like.riscv",
)

system.workload = SEWorkload.init_compatible(binary)

process = Process()
process.cmd = [binary]
system.cpu.workload = process
system.cpu.createThreads()

root = Root(full_system=False, system=system)
m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate()
m5.stats.dump()
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
