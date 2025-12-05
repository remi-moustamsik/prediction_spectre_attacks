from caches import (
    L1DCache,
    L1ICache,
)

import m5
from m5.objects import *
from m5.util import convert

from gem5.resources.resource import obtain_resource

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = "timing"
system.mem_ranges = [AddrRange("512MB")]

system.cpu = X86TimingSimpleCPU()

system.membus = SystemXBar()

# créer les L1
system.l1icache = L1ICache()
system.l1dcache = L1DCache()

# connecter au CPU
system.l1icache.connectCPU(system.cpu)
system.l1dcache.connectCPU(system.cpu)

# connecter au bus côté mémoire
system.l1icache.connectBus(system.membus)
system.l1dcache.connectBus(system.membus)


# system.cpu.icache_port = system.l1_cache.cpu_side

# system.cpu.icache_port = system.membus.cpu_side_ports

system.cpu.createInterruptController()
system.cpu.interrupts[0].pio = system.membus.mem_side_ports
system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

system.system_port = system.membus.cpu_side_ports

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

binary = "tests/test-progs/hello/bin/x86/linux/hello"

# for gem5 V21 and beyond
system.workload = SEWorkload.init_compatible(binary)

process = Process()
process.cmd = [binary]
system.cpu.workload = process
system.cpu.createThreads()

root = Root(full_system=False, system=system)
m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate()

print(
    f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}"
)
