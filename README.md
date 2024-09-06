# Simple Trace-Based Cache Simulator

This configurable simulator is based on a [course homework](https://occs.oberlin.edu/~ctaylor/classes/210SP13/cache.html). It processes an **address trace** (a list of memory addresses) and simulates cache behavior, including cache hits and misses. The simulator calculates execution time for different cache configurations. It models a **write-back cache** with a **Least Recently Used (LRU)** replacement policy for associative caches.

The input traces represent memory accesses from the programs `art`, `mcf`, and `swim` from the `SPEC` benchmarks. These address traces follow the format:


```
# LS ADDRESS IC
```

Where:
- LS: Load/Store indicator (0 for load, 1 for store).
- ADDRESS: 8-character hexadecimal memory address.
- IC: Instruction count (number of instructions executed since the last memory access, including this one).

## Simulator Parameters
The simulator takes the following configurable parameters:
- **Associativity** (`-a`): Degree of associativity in the cache.
- **Block size** (`-s`): Cache block size in bytes.
- **Cache size** (`-l`): Cache size in kilobytes.
- **Miss penalty** (`-mp`): Number of cycles lost due to a cache miss.

## Usage
1. Clone the repository
2. Download and extract [traces.zip](https://occs.oberlin.edu/~ctaylor/classes/210SP13/traces.zip)
3. Compile and run the simulator with a tracefile (art.trace.gz, mcf.trace.gz, or swim.trace.gz). For example `g++ -o cachesim cachesim.cpp && gunzip -c art/file.trace.gz | ./cachesim -a 1 -s 16 -l 16 -mp 30`.

The command above outputs:
```
Cache parameters:
Cache Size (KB)                 16
Cache Associativity             1
Cache Block Size (bytes)        16
Miss penalty (cyc)              30

Simulation results:
        execution time 21857966 cycles
        instructions 5136716
        memory accesses 1957764
        overall miss rate 0.28
        read miss rate 0.30
        total CPI 4.26
dirty evictions 60540
load_misses 523277
store_misses 30062
load_hits 1208606
store_hits 195819
```
