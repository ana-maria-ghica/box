#define _GNU_SOURCE
#include "util.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define PATH_LEN_MAX 4096

void setup_rootfs() {
    int ret;
    char* new_root = "./overlay/merged";

    // creez directorul overlay ----------------------------------------
    ret = mkdir("./overlay", 0777);
    CHECK_IGNORE_EEXIST(ret, "mkdir");

    // creez cele 3 directoare: upper, work, merged + directorul lower == Alpine
    ret = mkdir("./overlay/upper", 0777);
    CHECK_IGNORE_EEXIST(ret, "mkdir");

    ret = mkdir("./overlay/work", 0777);
    CHECK_IGNORE_EEXIST(ret, "mkdir");

    ret = mkdir("./overlay/merged", 0777);
    CHECK_IGNORE_EEXIST(ret, "mkdir");

    // montez directorul overlay ----------------------------------------
    ret = mount("overlay", "./overlay/merged", "overlay", 0, "lowerdir=./alpine,upperdir=./overlay/upper,workdir=./overlay/work");
    CHECK(ret);

    // rootfs-ul nou: trebuie sa fie mount point
    ret = mount(new_root, new_root, NULL, MS_BIND | MS_REC, NULL);
    CHECK(ret);

    // creez directorul old_root
    char old_root[PATH_LEN_MAX];
    // snprintf(old_root, sizeof(old_root), "%s/old_root", rootfs);
    snprintf(old_root, sizeof(old_root), "%s/old_root", new_root);   // new_root, nu rootfs

    ret = mkdir(old_root, 0777);
    CHECK_IGNORE_EEXIST(ret, "mkdir");

    // intru in new_root
    ret = chdir(new_root);
    CHECK(ret);

    // pivotez new_root
    ret = syscall(SYS_pivot_root, ".", "old_root");
    CHECK(ret);

    // reintru in "/" (adica in new_root)
    ret = chdir("/");
    CHECK(ret);

    // demontez old_root
    ret = umount2("/old_root", MNT_DETACH);
    CHECK(ret);

    // sterg old_root
    ret = rmdir("/old_root");
    CHECK(ret);
}



/*
    Varianta initiala de mount rootfs fara y filesystem
    int ret;
    char* new_root = rootfs;

    // rootfs-ul nou: trebuie sa fie mount point
    ret = mount(new_root, new_root, NULL, MS_BIND | MS_REC, NULL);
    CHECK(ret);

    // creez directorul old_root
    char old_root[PATH_LEN_MAX];
    snprintf(old_root, sizeof(old_root), "%s/old_root", rootfs);

    ret = mkdir(old_root, 0777);
    CHECK_IGNORE_EEXIST(ret, "mkdir");

    // intru in new_root
    ret = chdir(new_root);
    CHECK(ret);

    // pivotez new_root
    ret = syscall(SYS_pivot_root, ".", "old_root");
    CHECK(ret);

    // reintru in "/" (adica in new_root)
    ret = chdir("/");
    CHECK(ret);

    // demontez old_root
    ret = umount2("/old_root", MNT_DETACH);
    CHECK(ret);

    // sterg old_root
    ret = rmdir("/old_root");
    CHECK(ret);

*/