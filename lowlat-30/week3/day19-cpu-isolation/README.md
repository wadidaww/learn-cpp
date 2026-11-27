# Day 19 · Owning a core: isolcpus, nohz_full, IRQ affinity, NUMA, C-states, jitter

**Why trading firms care:** a well-written hot path still has a terrible p99.9 if the OS interrupts it. Firms tune hosts so the trading threads own their cores. "How would you set up a Linux box for low latency?" is a common question, and a checklist answer with reasons impresses.

## 1. Concepts (45 min)

### Sources of jitter on a "busy" core
| Source | Typical size | Fix |
|---|---|---|
| Scheduler puts another task on your core | µs to ms | `isolcpus=` / cpusets, pin your thread (`taskset`, `pthread_setaffinity_np`) |
| Timer tick (scheduler tick, 250–1000 Hz) | ~1–10 µs each | `nohz_full=` (adaptive ticks) on the isolated cores, `rcu_nocbs=` |
| Device interrupts landing on your core | µs | Set `/proc/irq/*/smp_affinity` to housekeeping cores; stop or configure `irqbalance` |
| Kernel threads, RCU callbacks, workqueues | µs | `rcu_nocbs`, `kthread_cpus`/housekeeping, `workqueue` cpumask |
| CPU idle states (C-states) waking up | 1–100+ µs | `intel_idle.max_cstate=0 processor.max_cstate=1`, `idle=poll`, or `/dev/cpu_dma_latency` |
| Frequency changes (P-states, turbo transitions) | µs | `performance` governor, fixed frequency, sometimes turbo off for consistency |
| Hyperthread sibling activity | throughput loss, jitter | Disable SMT or leave the sibling idle |
| Page faults, TLB shootdowns | µs | Pre-fault and `mlockall`, huge pages, avoid `munmap` on other threads |
| SMIs (System Management Interrupts, firmware) | 10s–100s µs | BIOS settings; detect with `hwlatdetect`, count with `turbostat` / MSR 0x34 |
| NUMA remote memory | +50–100% latency | Allocate on the local node, NIC on the same socket |
| Transparent huge page compaction (khugepaged) | ms | THP `madvise` mode or off; explicit huge pages |

### Kernel command line of a typical trading host (example)
```
isolcpus=managed_irq,domain,2-7 nohz_full=2-7 rcu_nocbs=2-7 irqaffinity=0-1
intel_idle.max_cstate=0 processor.max_cstate=1 intel_pstate=disable nosoftlockup
transparent_hugepage=madvise hugepagesz=1G hugepages=8 mitigations=off   # mitigations=off only on a closed box, know why
```
Then: `tuned-adm profile latency-performance` (or `network-latency`), `cpupower frequency-set -g performance`, `systemctl stop irqbalance`, NIC IRQs pinned to housekeeping cores (or the same core as the polling thread when using kernel interrupts, depending on design), and `numactl --cpunodebind=0 --membind=0 ./app`.

### Verify, do not assume
- `cat /proc/cmdline`, `cat /sys/devices/system/cpu/isolated`, `cat /sys/devices/system/cpu/nohz_full`
- `cat /proc/interrupts` (watch the counts on your isolated cores stay flat)
- `turbostat`, `perf stat -e cs,migrations,faults`
- A **jitter probe**: spin on one core reading the TSC and record every gap above a threshold. The gaps are the moments the core was taken from you. That is today's bench.

## 2. Exercise

1. Run the jitter probe on a normal core and an idle core (and an isolated core if you can reboot a Linux box with `isolcpus=`):
   ```bash
   cmake --build build --target day19_bench
   ./build/day19_bench 0 10        # cpu 0, 10 seconds
   ./build/day19_bench 3 10
   ```
   It prints how many interruptions above 1 µs, 10 µs and 100 µs it saw, the longest one, and how much of the time the core was really yours.
2. While it runs, generate load elsewhere (`stress-ng --cpu 2` or a compile) and run again. Compare.
3. If you have a Linux machine you can reboot: add `isolcpus=3 nohz_full=3 rcu_nocbs=3` to the kernel command line, move IRQs off core 3, rerun on core 3, and write down the before/after table.
4. Rerun your day 7 SPSC benchmark with both threads on isolated cores and compare p99.9.

If you only have a VM or a laptop, do steps 1–2 and say so in your notes: a hypervisor adds its own jitter (steal time) that you cannot tune away from inside the guest.

**Done when:** you have the table, and you can recite the tuning checklist with one reason per line.

## 3. Say it out loud

<details><summary>1. How would you configure a Linux server for low latency?</summary>
Isolate trading cores (isolcpus or cpusets) with nohz_full and rcu_nocbs; move IRQs and kernel threads to housekeeping cores; disable deep C-states and fix the frequency with the performance governor; disable SMT or leave siblings idle; pin each thread and its memory on the NIC's NUMA node; huge pages, pre-faulted and mlocked memory; turn off irqbalance and THP compaction; then verify with a jitter probe and /proc/interrupts.
</details>

<details><summary>2. What does nohz_full do?</summary>
Stops the periodic scheduler tick on those CPUs when only one task is runnable, so a busy-polling thread is not interrupted 250–1000 times a second. Works with rcu_nocbs to move RCU callbacks off those CPUs too.
</details>

<details><summary>3. Why do C-states hurt latency?</summary>
A core in a deep idle state needs tens of microseconds to wake up. A polling thread never lets its core idle, but other cores (or the NIC interrupt core) might, and package-level C-states affect everything. Limit them.
</details>

<details><summary>4. Why does NUMA matter for the NIC?</summary>
The NIC is attached to one socket's PCIe lanes. Packets DMA into memory on that node; a thread on the other socket pays cross-socket latency for every packet. Keep the NIC, its memory and its threads on the same node.
</details>

<details><summary>5. What is an SMI and why can't you tune it away from Linux?</summary>
A System Management Interrupt is handled by firmware in SMM, invisible to the OS, and can stall a core for tens to hundreds of microseconds. Only BIOS settings or hardware choices reduce them; detect them with hwlatdetect or the SMI counter MSR.
</details>

<details><summary>6. Your p99.9 doubled after a kernel upgrade. Where do you look?</summary>
Did the boot parameters survive (cat /proc/cmdline)? New mitigations enabled? Changes to the tick, RCU, or idle driver defaults? IRQ affinity reset? Run the jitter probe and compare interrupt counts per core before and after.
</details>
