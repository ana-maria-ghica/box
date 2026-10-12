#define _GNU_SOURCE
#include "security.h"
#include "util.h"


#include <linux/prctl.h>
#include <sys/prctl.h>
#include <linux/capability.h>
#include <linux/filter.h>
#include <linux/audit.h>
#include <linux/seccomp.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <unistd.h>

void restrict_process() {
    int ret;

    // PR_SET_NO_NEW_PRIVS -> procesul [si nici copilul] nu poate avea privilegii mai mari de-a lungul programului
    ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    CHECK(ret);

    // lista cu capabilitatile de eliminat
    ret = prctl(PR_CAPBSET_DROP, CAP_SYS_ADMIN, 0, 0, 0);
    CHECK(ret);

    ret = prctl(PR_CAPBSET_DROP, CAP_SYS_BOOT, 0, 0, 0);
    CHECK(ret);

    ret = prctl(PR_CAPBSET_DROP, CAP_SYS_MODULE, 0, 0, 0);
    CHECK(ret);

    ret = prctl(PR_CAPBSET_DROP, CAP_SYS_RAWIO, 0, 0, 0);
    CHECK(ret);

    ret = prctl(PR_CAPBSET_DROP, CAP_SYS_TIME, 0, 0, 0);
    CHECK(ret);

    // bpf - implementarea mea -----

    struct sock_filter filters[] = {
    // incarca arhitectura
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch)),

    // compar daca lucrez pe aceeasi arhitectura
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),

    // daca arhitectura e gresita => kill
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

    // incarc nr syscall-ului
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),

    // compar nr syscall-ului cu syscall-urile interzise

    // syscall reboot
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_reboot, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

    // syscall kernel load
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_kexec_load, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

    // syscall load kernel module
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_init_module, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

    // syscall delete kernel module
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_delete_module, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

    // syscall control swap on
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_swapon, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

    // syscall control swapoff
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_swapoff, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

    // syscall control mount
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_mount, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

    // syscall unmount
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_umount2, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

    // syscall ptrace
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_ptrace, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),   

    // lasa sa functioneze syscall-urile bune
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)
    };

    struct sock_fprog prog;
    prog.len = sizeof(filters) / sizeof(filters[0]);
    prog.filter = filters;

    // syscall de seccomp
    ret = syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog);
    CHECK(ret);
}


