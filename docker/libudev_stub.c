/*
 * Minimal libudev stub for Vivado 2021.2 on Ubuntu 22.04.
 *
 * Vivado's FLEXlm license manager (libXil_lmgr11.so) calls
 * udev_enumerate_scan_devices() to find MAC addresses for node-locked
 * license validation. Ubuntu 22.04's libudev.so.1 crashes (realloc invalid
 * pointer) when called from the old license manager binary.
 *
 * This stub re-implements the minimum udev API using /sys/class/net directly,
 * avoiding the crash entirely while still returning valid MAC address data.
 *
 * Build:
 *   gcc -shared -fPIC -o libudev_stub.so libudev_stub.c -ldl
 *
 * Use:
 *   LD_PRELOAD=/path/to/libudev_stub.so vivado ...
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* ─── internal list entry ─────────────────────────────────────────────────── */
struct udev_list_entry {
    char *name;
    char *value;
    struct udev_list_entry *next;
};

static struct udev_list_entry *entry_new(const char *name, const char *value) {
    struct udev_list_entry *e = calloc(1, sizeof(*e));
    e->name  = name  ? strdup(name)  : NULL;
    e->value = value ? strdup(value) : NULL;
    return e;
}

static void entry_free_list(struct udev_list_entry *e) {
    while (e) {
        struct udev_list_entry *n = e->next;
        free(e->name);
        free(e->value);
        free(e);
        e = n;
    }
}

/* ─── udev ────────────────────────────────────────────────────────────────── */
struct udev {
    int ref;
};

struct udev *udev_new(void) {
    struct udev *u = calloc(1, sizeof(*u));
    u->ref = 1;
    return u;
}
struct udev *udev_ref(struct udev *u) {
    if (u) u->ref++;
    return u;
}
void udev_unref(struct udev *u) {
    if (u && --u->ref == 0) free(u);
}

/* ─── udev_enumerate ──────────────────────────────────────────────────────── */
struct udev_enumerate {
    struct udev *udev;
    int ref;
    char *match_subsystem;
    struct udev_list_entry *entries;
};

struct udev_enumerate *udev_enumerate_new(struct udev *u) {
    struct udev_enumerate *e = calloc(1, sizeof(*e));
    e->udev = u;
    e->ref  = 1;
    return e;
}
struct udev_enumerate *udev_enumerate_ref(struct udev_enumerate *e) {
    if (e) e->ref++;
    return e;
}
void udev_enumerate_unref(struct udev_enumerate *e) {
    if (!e) return;
    if (--e->ref == 0) {
        free(e->match_subsystem);
        entry_free_list(e->entries);
        free(e);
    }
}
int udev_enumerate_add_match_subsystem(struct udev_enumerate *e, const char *sub) {
    if (!e) return -1;
    free(e->match_subsystem);
    e->match_subsystem = sub ? strdup(sub) : NULL;
    return 0;
}
int udev_enumerate_add_match_sysname(struct udev_enumerate *e, const char *s) {
    (void)e; (void)s; return 0;
}
int udev_enumerate_add_match_property(struct udev_enumerate *e,
                                      const char *p, const char *v) {
    (void)e; (void)p; (void)v; return 0;
}
int udev_enumerate_add_match_tag(struct udev_enumerate *e, const char *t) {
    (void)e; (void)t; return 0;
}
int udev_enumerate_add_match_is_initialized(struct udev_enumerate *e) {
    (void)e; return 0;
}

/*
 * This is the function that crashes in Ubuntu 22.04.
 * We implement it by reading /sys/class/net directly.
 */
int udev_enumerate_scan_devices(struct udev_enumerate *e) {
    if (!e) return -1;

    entry_free_list(e->entries);
    e->entries = NULL;

    /* Only "net" subsystem is relevant for MAC-based license checks */
    if (e->match_subsystem && strcmp(e->match_subsystem, "net") != 0)
        return 0;

    const char *base = "/sys/class/net";
    DIR *dir = opendir(base);
    if (!dir) return 0;

    struct udev_list_entry *last = NULL;
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (de->d_name[0] == '.') continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", base, de->d_name);

        struct udev_list_entry *entry = entry_new(path, NULL);
        if (last) last->next = entry;
        else       e->entries = entry;
        last = entry;
    }
    closedir(dir);
    return 0;
}

int udev_enumerate_scan_subsystems(struct udev_enumerate *e) {
    (void)e; return 0;
}

struct udev_list_entry *udev_enumerate_get_list_entry(struct udev_enumerate *e) {
    return e ? e->entries : NULL;
}

/* ─── udev_list_entry ─────────────────────────────────────────────────────── */
struct udev_list_entry *udev_list_entry_get_next(struct udev_list_entry *e) {
    return e ? e->next : NULL;
}
const char *udev_list_entry_get_name(struct udev_list_entry *e) {
    return e ? e->name : NULL;
}
const char *udev_list_entry_get_value(struct udev_list_entry *e) {
    return e ? e->value : NULL;
}

/* ─── udev_device ─────────────────────────────────────────────────────────── */
struct udev_device {
    struct udev *udev;
    int ref;
    char *syspath;   /* e.g. /sys/class/net/eth0 */
    char *sysname;   /* e.g. eth0 */
    char *subsystem; /* e.g. net */
    struct udev_list_entry *sysattrs;
};

struct udev_device *udev_device_new_from_syspath(struct udev *u,
                                                  const char *syspath) {
    if (!syspath) return NULL;

    struct udev_device *d = calloc(1, sizeof(*d));
    d->udev    = u;
    d->ref     = 1;
    d->syspath = strdup(syspath);

    /* derive sysname = last path component */
    const char *last = strrchr(syspath, '/');
    d->sysname = strdup(last ? last + 1 : syspath);

    /* derive subsystem from path */
    if (strstr(syspath, "/class/net/"))
        d->subsystem = strdup("net");
    else
        d->subsystem = strdup("unknown");

    /* populate sysattrs: read "address" from /sys/class/net/<iface>/address */
    if (strcmp(d->subsystem, "net") == 0) {
        char addr_path[512];
        snprintf(addr_path, sizeof(addr_path), "%s/address", syspath);
        FILE *f = fopen(addr_path, "r");
        if (f) {
            char mac[32] = {0};
            if (fgets(mac, sizeof(mac), f)) {
                /* strip newline */
                size_t l = strlen(mac);
                while (l > 0 && (mac[l-1] == '\n' || mac[l-1] == '\r'))
                    mac[--l] = '\0';
                d->sysattrs = entry_new("address", mac);
            }
            fclose(f);
        }
    }

    return d;
}

struct udev_device *udev_device_ref(struct udev_device *d) {
    if (d) d->ref++;
    return d;
}
void udev_device_unref(struct udev_device *d) {
    if (!d) return;
    if (--d->ref == 0) {
        free(d->syspath);
        free(d->sysname);
        free(d->subsystem);
        entry_free_list(d->sysattrs);
        free(d);
    }
}

const char *udev_device_get_syspath(struct udev_device *d) {
    return d ? d->syspath : NULL;
}
const char *udev_device_get_sysname(struct udev_device *d) {
    return d ? d->sysname : NULL;
}
const char *udev_device_get_subsystem(struct udev_device *d) {
    return d ? d->subsystem : NULL;
}
const char *udev_device_get_devnode(struct udev_device *d) {
    (void)d; return NULL;
}
const char *udev_device_get_devtype(struct udev_device *d) {
    (void)d; return NULL;
}
const char *udev_device_get_property_value(struct udev_device *d,
                                            const char *key) {
    (void)d; (void)key; return NULL;
}
const char *udev_device_get_sysattr_value(struct udev_device *d,
                                           const char *attr) {
    if (!d || !attr) return NULL;
    struct udev_list_entry *e = d->sysattrs;
    while (e) {
        if (e->name && strcmp(e->name, attr) == 0)
            return e->value;
        e = e->next;
    }
    return NULL;
}
struct udev_list_entry *udev_device_get_properties_list_entry(
        struct udev_device *d) {
    (void)d; return NULL;
}
struct udev_list_entry *udev_device_get_sysattr_list_entry(
        struct udev_device *d) {
    return d ? d->sysattrs : NULL;
}
struct udev_device *udev_device_get_parent(struct udev_device *d) {
    (void)d; return NULL;
}
struct udev_device *udev_device_get_parent_with_subsystem_devtype(
        struct udev_device *d, const char *sub, const char *devtype) {
    (void)d; (void)sub; (void)devtype; return NULL;
}
int udev_device_has_tag(struct udev_device *d, const char *tag) {
    (void)d; (void)tag; return 0;
}
struct udev *udev_device_get_udev(struct udev_device *d) {
    return d ? d->udev : NULL;
}

/* ─── udev_monitor (stubs — not needed for license check) ────────────────── */
struct udev_monitor { int ref; };
struct udev_monitor *udev_monitor_new_from_netlink(struct udev *u,
                                                    const char *name) {
    (void)u; (void)name;
    struct udev_monitor *m = calloc(1, sizeof(*m));
    m->ref = 1;
    return m;
}
struct udev_monitor *udev_monitor_ref(struct udev_monitor *m) {
    if (m) m->ref++;
    return m;
}
void udev_monitor_unref(struct udev_monitor *m) {
    if (m && --m->ref == 0) free(m);
}
int udev_monitor_filter_add_match_subsystem_devtype(struct udev_monitor *m,
        const char *sub, const char *devtype) {
    (void)m; (void)sub; (void)devtype; return 0;
}
int udev_monitor_enable_receiving(struct udev_monitor *m) {
    (void)m; return 0;
}
int udev_monitor_get_fd(struct udev_monitor *m) {
    (void)m; return -1;
}
struct udev_device *udev_monitor_receive_device(struct udev_monitor *m) {
    (void)m; return NULL;
}
