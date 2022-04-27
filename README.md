# Cloud Shell

Simulation of a pay-per-use system, typically used in cloud environments, where the vendor provides resources (cpu, memory, etc.) and the user pays for the resources used.

User accesses bash where he can execute commandas. The shell monitors the processes executed by the user and deducts it from the user balance. Once the balance runs out, processes are forced to terminate.

Monitoring is done through the /proc virtual folder, pidstat.

Each cloudshell periodically sends accounting information to a server. This server manages the resources used and the balance of each user.

Project executed in the context of Operating Systems course @ uminho