# box

A small container runtime written from scratch in C, using Linux kernel
primitives directly. `box` starts a process in an isolated environment — its
own filesystem, process tree, hostname, and network — with bounded resources
and a restricted system-call surface. It is, in spirit, a minimal reimplementation
of the machinery that sits underneath Docker (closer to `runc` than to the full
Docker toolchain).

I wrote it to understand containers at the level where they are actually built:
the system calls, not the tooling. There are no container libraries involved —
everything is done against the raw kernel API.

## What it does

Running a container isolates the process along several independent axes:

- **Processes** — the container gets its own PID namespace, so its main process
  is PID 1 and cannot see host processes.
- **Filesystem** — the root is pivoted (`pivot_root`) into an Alpine root
  filesystem layered with **OverlayFS**, so the base image stays read-only and
  per-container writes land in a separate upper layer. A fresh `/proc`, `/sys`,
  and a minimal `/dev` are mounted inside.
- **Resources** — memory, CPU share, and process count are capped through
  **cgroups v2**. A fork bomb inside the container stops at the PID limit instead
  of taking down the host.
- **Privileges** — dangerous capabilities are dropped, `no_new_privs` is set, and
  a hand-written **seccomp-BPF** filter blocks a set of dangerous system calls
  (`reboot`, `kexec_load`, module loading, `mount`, `ptrace`, …).
- **Networking** — each container gets its own network namespace connected to the
  host through a **veth** pair, with a configurable IP and a default route.

On top of running containers, `box` can manage them: list the running ones, enter
one that is already running, and stop one from the outside.

## Building

Requires a recent Linux kernel (cgroups v2, OverlayFS), `gcc`, and `iproute2`
(for the `ip` command used during network setup).

```
make
```

You also need a root filesystem to run. The included script fetches a minimal
Alpine one:

```
./scripts/fetch-rootfs.sh     # downloads Alpine minirootfs into ./alpine
```

## Usage

Most operations touch privileged kernel interfaces, so `box` is run with `sudo`
(see the note on rootless mode below).

Start a container:

```
sudo ./box run \
    --name web \
    --rootfs ./alpine \
    --hostname sandbox \
    --pids 50 --mem 64M --cpu 0.5 \
    --net 10.0.0.2/24 \
    /bin/sh
```

Manage running containers:

```
sudo ./box ps                 # list running containers
sudo ./box exec web /bin/sh   # open a shell inside an existing container
sudo ./box stop web           # stop a container
```

Inside the container, `hostname` reports `sandbox`, `ps` shows only the
container's own processes, and `cat /etc/os-release` shows Alpine — not the host.

## How it works

The runtime splits cleanly into a **parent** (the host side, which sets things up)
and a **child** (the process that becomes the container). The parent calls `clone`
with the namespace flags, configures what has to be configured from the outside
(cgroups, the host end of the network), and signals the child over a pipe; the
child waits on that pipe, then builds its own world (mounts, pivot_root, hostname,
privilege restriction) before executing the target program.

That pipe matters more than it looks: several things — moving the child into its
cgroup, moving a veth endpoint into its network namespace — can only be done by the
parent, from the outside, after the child exists but before it runs. The pipe is
what keeps the child paused until that setup is finished.

A few decisions worth calling out:

- **PID 1 is special.** A shell running directly as PID 1 does not reap orphaned
  processes and ignores most signals. So `box` inserts a tiny init process: it
  becomes PID 1, forks the real program as its child, reaps zombies in a loop, and
  forwards `SIGTERM`/`SIGINT` down to the child. This is the same problem Docker
  solves with `tini`.
- **`setns` for `exec`.** Entering a running container is not the same as creating
  one. `box exec` opens the namespace handles under `/proc/<pid>/ns/` and joins them
  with `setns`. The mount namespace has to be joined *last*, because once you switch
  into the container's filesystem view, the host's `/proc/<pid>/ns/` paths are no
  longer reachable.
- **Order of privilege restriction.** Capabilities and seccomp are applied at the
  very end, right before `execve` — after all the privileged setup (mounting,
  networking) is done, so the restrictions don't block the runtime's own work.

## Rootless mode and the AppArmor limitation

`box` implements rootless containers through user namespaces: a `--rootless` path
adds `CLONE_NEWUSER` and writes the `uid_map`/`gid_map` mappings (with the required
`setgroups=deny` step) so that the container's root maps to the unprivileged host
user. The mapping itself works — a process reports `uid=0` inside the container
while remaining the normal user on the host.

Full rootless execution is **blocked on the development environment** by Ubuntu's
`apparmor_restrict_unprivileged_userns` policy (enabled by default since Ubuntu
24.04), which restricts mount operations inside unprivileged user namespaces. This
is a distribution-level security policy rather than an implementation limitation;
the same code path is expected to work on systems without that restriction. The
default (rooted) path is the one exercised throughout.

## Testing

An integration test suite exercises the observable behaviour of each isolation
mechanism, including the negative cases that matter most — that a fork bomb is
contained and that the seccomp filter is actually loaded:

```
make test
```

## Comparison with Docker

`box` reimplements the core container *runtime* — the layer that turns kernel
primitives into a running, isolated process. It deliberately does not cover the
rest of the Docker toolchain:

| Area | box | Docker |
| --- | --- | --- |
| Process/filesystem/network isolation | yes | yes |
| Resource limits (cgroups v2) | yes | yes |
| Layered filesystem (OverlayFS) | yes | yes |
| seccomp + capabilities sandbox | yes (hand-written filter) | yes (larger default profile) |
| Lifecycle (ps / exec / stop) | yes | yes |
| Rootless | implemented, environment-blocked | yes |
| Outbound internet (NAT) | not implemented | yes |
| Image build / registry | not implemented | yes |
| Multi-host orchestration | out of scope | (via Kubernetes, etc.) |

## References

The implementation leans on a handful of primary sources:

- Michael Kerrisk, *The Linux Programming Interface* — processes, `clone`, mounts,
  capabilities, signals.
- The LWN "Namespaces in operation" series, for the namespace mechanics.
- Liz Rice, *Container Security*, for the overall isolation and sandboxing model.
- The kernel documentation for cgroups v2, OverlayFS, and seccomp, plus the
  relevant `man` pages (`namespaces(7)`, `pivot_root(2)`, `seccomp(2)`, …).

## License

MIT. See [LICENSE](LICENSE).