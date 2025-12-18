# O3_Spectre_Config.py
# Configuration gem5 pour RISC-V O3 CPU avec Cache L1/L2 et latence DRAM accrue,
# optimisée pour la mesure de canaux auxiliaires.

import os

import m5
from m5.objects import *

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = "timing"
system.mem_ranges = [AddrRange("512MiB")]
system.cpu = RiscvO3CPU()

system.membus = SystemXBar()  # Bus système (bas niveau)


# --- 1. DÉFINITIONS DES CLASSES DE CACHES (Nettoyées) ---
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
    tag_latency = 60  # Ancienne valeur: 20
    data_latency = 60  # Ancienne valeur: 20
    response_latency = 60  # Ancienne valeur: 20
    size = "256kB"
    assoc = 8


# --- 2. INSTANCIATION ET CORRECTION DES PARAMÈTRES (tgts_per_mshr) ---
# Définir mshrs et tgts_per_mshr directement à l'instanciation pour éviter les erreurs.
system.cpu.dcache = L1_DCache(mshrs=20, tgts_per_mshr=12)
system.cpu.icache = L1_ICache(mshrs=20, tgts_per_mshr=12)
system.l2cache = L2Cache(mshrs=20, tgts_per_mshr=16)
system.l2bus = L2XBar(width=32)


# --- 3. CONNEXIONS CPU/CACHES ---
system.cpu.icache_port = system.cpu.icache.cpu_side
system.cpu.dcache_port = system.cpu.dcache.cpu_side

system.cpu.createInterruptController()

# L1 se connecte au bus L2
system.cpu.icache.mem_side = system.l2bus.cpu_side_ports
system.cpu.dcache.mem_side = system.l2bus.cpu_side_ports

# L2 se connecte au bus L2 et au bus système
system.l2cache.cpu_side = system.l2bus.mem_side_ports
system.l2cache.mem_side = system.membus.cpu_side_ports


# --- 4. SOLUTION ROBUSTE: INJECTION DE LA LATENCE DRAM (SOLUTION ROBUSTE) ---

# CRÉATION D'UN BUS INTERMÉDIAIRE POUR INJECTER LA LATENCE (100 cycles à 1GHz)
system.mem_latency_bus = NoncoherentXBar()
# Utiliser 'frontend_latency' sur le bus est la méthode la plus fiable pour forcer le délai
system.mem_latency_bus.frontend_latency = 100
system.mem_latency_bus.width = 64

# Définir les latences de propagation et de réponse :
system.mem_latency_bus.forward_latency = 1
system.mem_latency_bus.response_latency = 1

# Définition du Contrôleur Mémoire (MemCtrl)
system.mem_ctrl = MemCtrl()

# On utilise le modèle DDR3 standard (car il n'accepte pas les paramètres de latence manuels)
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]

# CONNEXION 1 : Le bus système se connecte au bus à latence
system.membus.mem_side_ports = system.mem_latency_bus.cpu_side_ports

# CONNEXION 2 : Le contrôleur mémoire se connecte au bus à latence
system.mem_ctrl.port = system.mem_latency_bus.mem_side_ports


# --- CONNEXIONS FINALES ET WORKLOAD ---
system.system_port = system.membus.cpu_side_ports

thispath = os.path.dirname(os.path.realpath(__file__))
# Le binaire doit s'appeler 'calibration_final.riscv' et être à la racine de gem5.
binary = os.path.join(
    thispath,
    "../../../",
    "calibration.riscv",  # Chemin corrigé
)

system.workload = SEWorkload.init_compatible(binary)

process = Process()
process.cmd = [binary]
system.cpu.workload = process
system.cpu.createThreads()

root = Root(full_system=False, system=system)
m5.instantiate()

print(f"Beginning simulation!")
exit_event = m5.simulate()
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
