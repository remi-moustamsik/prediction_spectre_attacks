from m5.objects import Cache


class L1Cache(Cache):
    assoc = 2
    tag_latency = data_latency = response_latency = 2
    mshrs = 4
    tgts_per_mshr = 20
    size = "32kB"

    # Méthode utilitaire pour connecter au bus
    def connectBus(self, bus):
        self.mem_side = bus.cpu_side_ports


class L1ICache(L1Cache):
    # Cache d'instructions
    def connectCPU(self, cpu):
        self.cpu_side = cpu.icache_port


class L1DCache(L1Cache):
    # Cache de données
    def connectCPU(self, cpu):
        self.cpu_side = cpu.dcache_port
