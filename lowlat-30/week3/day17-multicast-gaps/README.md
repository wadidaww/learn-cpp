# Day 17 · UDP multicast market data: sequence numbers, gaps, A/B arbitration

**Why trading firms care:** every exchange publishes market data as UDP multicast, usually on **two identical lines (A and B)** over separate networks, with a sequence number on every packet. Your feed handler takes whichever copy arrives first, drops the duplicate, and detects gaps when both lines lost a packet. This is a common design question ("design a feed handler") and a common coding question ("write the arbitration logic").

## 1. Concepts (45 min)

### Multicast in one page
- A sender transmits once to a **group address** (224.0.0.0/4, e.g. 239.1.1.1:30001). Switches and routers copy the packet to every port with a subscriber.
- A receiver **joins** the group: `setsockopt(IP_ADD_MEMBERSHIP, {group, interface})`. The kernel sends an IGMP join so the network starts forwarding the group to it.
- Bind the receiving socket to the group port (and usually `SO_REUSEADDR` so several processes can listen).
- Sender options: `IP_MULTICAST_TTL` (1 = stay on the local subnet), `IP_MULTICAST_LOOP` (receive your own packets on the same host, needed for loopback tests), `IP_MULTICAST_IF` (which NIC).
- No retransmission, no flow control. If your receiver is slow, the kernel socket buffer overflows and packets are dropped silently (`netstat -su`: "receive buffer errors"). Raise `SO_RCVBUF` (and `net.core.rmem_max`) and, above all, read fast.

### Exchange feed design (Nasdaq ITCH via MoldUDP64, CME MDP 3.0, Eurex EOBI...)
- Every packet has a **sequence number** (MoldUDP64: session + sequence of the first message + message count).
- **A/B lines:** identical streams on two multicast groups, often over separate switches. Packet loss on one line is usually covered by the other.
- **Arbitration:** process the next expected sequence from whichever line delivers it first; drop the later copy.
- **Gap:** both lines missed a sequence. Options: wait briefly (it may arrive late or out of order), then request a **retransmission** (TCP or unicast UDP request server) or rebuild from a **snapshot** channel and resume.
- While a gap is open, the book is not trustworthy: stop quoting that instrument.

### Arbitration rules (what you implement today)
Keep `expected` = next sequence to deliver, and a small **reorder window** of W slots for packets that arrive early.
1. `seq < expected`, or already buffered: duplicate, drop (count it).
2. `seq == expected`: deliver it, then deliver any buffered packets that are now consecutive.
3. `expected < seq < expected + W`: buffer it (copy into slot `seq % W`).
4. `seq >= expected + W`: the window cannot hold it, so the oldest missing sequence is declared a gap (see `skip_gap` below), repeated until `seq` fits.
5. `on_timeout()` (the caller's gap timer fired): if anything is buffered, declare the gap before the first buffered packet and continue from there.

`skip_gap`: if something is buffered, report the gap `[expected, first_buffered - 1]`, jump to `first_buffered` and drain consecutive packets. If nothing is buffered (rule 4 with an empty window), report `[expected, seq - 1]` and jump to `seq`.

## 2. Exercise (2 h)

In `solution.hpp`:
1. `LineArbiter<W, MaxPayload, Sink>` following the rules above. Sink provides `on_message(seq, payload)` and `on_gap(first, last)`. Count duplicates. No allocation (fixed slots).
2. Multicast socket helpers: `mcast_open_receiver(group, port)` (non-blocking, joined, `SO_REUSEADDR`), `mcast_open_sender()` (TTL 1, loopback on), `udp_send_to(fd, group, port, bytes)`, `udp_recv(fd, buf)` (non-blocking: returns -1 with errno EAGAIN when empty).

The tests check every arbitration rule, then run a **real loopback multicast test**: two groups (A and B), each line randomly drops 10% of packets, and your arbiter must deliver every sequence that at least one line carried, in order, and report the rest as gaps.

If the multicast test fails with "No such device" or similar, your machine has no multicast route on loopback; run `sudo ip route add 239.0.0.0/8 dev lo` (Linux) and try again.

**Done when:** tests pass, and you can draw the feed handler (2 NICs → 2 sockets → arbiter → decoder → book → SPSC → strategy) and explain what happens on a gap.

## 3. Say it out loud

<details><summary>1. Why do exchanges send A and B lines?</summary>
Redundancy without retransmission delay: the same packets on two independent paths, so a loss on one line is usually covered by the other. The receiver takes the first copy and drops the second.
</details>

<details><summary>2. You detect a gap. What now?</summary>
Wait a short, bounded time for the packet (it may be reordered or late on the other line). Then request it from the retransmission service, or recover from the snapshot channel, buffering live packets meanwhile. Mark affected instruments stale and stop trading them until the book is rebuilt.
</details>

<details><summary>3. How do you join a multicast group?</summary>
Create a UDP socket, set SO_REUSEADDR, bind to the group's port, then setsockopt(IPPROTO_IP, IP_ADD_MEMBERSHIP) with the group and local interface. The kernel sends an IGMP membership report so the switch forwards the group.
</details>

<details><summary>4. Where can multicast packets be lost on your own host?</summary>
NIC ring overflow (ethtool -S rx_missed/rx_no_buffer), kernel socket buffer overflow (netstat -su receive buffer errors), or your application not draining fast enough. Fix by reading faster, bigger buffers, interrupt/CPU affinity, or kernel bypass.
</details>

<details><summary>5. Why a bounded reorder window?</summary>
It caps memory and latency: you buffer a few early packets while waiting for a missing one, but you never wait forever or buffer unboundedly. Beyond the window (or after a timeout) you declare the gap and move on.
</details>

<details><summary>6. What is a snapshot channel?</summary>
A separate feed that periodically publishes the full book state with the sequence number it corresponds to. A late joiner or a receiver with a gap applies the snapshot, then replays live packets after that sequence.
</details>
