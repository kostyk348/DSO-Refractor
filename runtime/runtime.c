#include "runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sys/stat.h>

static i64 file_mtime(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (i64)st.st_mtime;
}

int runtime_start(dsr_runtime *rt, const char *lib_path) {
    memset(rt, 0, sizeof(*rt));
    rt->lib_path = strdup(lib_path);
    if (!rt->lib_path) return -1;

    rt->dl_handle = dlopen(lib_path, RTLD_LAZY | RTLD_LOCAL);
    if (!rt->dl_handle) {
        fprintf(stderr, "DSR runtime: failed to load %s: %s\n", lib_path, dlerror());
        return -1;
    }

    rt->last_modified = file_mtime(lib_path);

    rt->init     = (void (*)(void))dlsym(rt->dl_handle, "dsr_init");
    rt->update   = (void (*)(double))dlsym(rt->dl_handle, "dsr_update");
    rt->render   = (void (*)(void))dlsym(rt->dl_handle, "dsr_render");
    rt->shutdown = (void (*)(void))dlsym(rt->dl_handle, "dsr_shutdown");
    rt->reload   = (void (*)(void))dlsym(rt->dl_handle, "dsr_reload");

    if (!rt->init || !rt->update || !rt->render || !rt->shutdown) {
        fprintf(stderr, "DSR runtime: missing symbols in %s\n", lib_path);
        dlclose(rt->dl_handle);
        rt->dl_handle = NULL;
        return -1;
    }

    rt->init();
    return 0;
}

void runtime_check_reload(dsr_runtime *rt) {
    if (!rt->dl_handle) return;

    i64 mtime = file_mtime(rt->lib_path);
    if (mtime <= rt->last_modified) return;

    printf("DSR runtime: reloading %s\n", rt->lib_path);

    if (rt->reload) rt->reload();
    if (rt->shutdown) rt->shutdown();

    dlclose(rt->dl_handle);

    rt->dl_handle = dlopen(rt->lib_path, RTLD_LAZY | RTLD_LOCAL);
    if (!rt->dl_handle) {
        fprintf(stderr, "DSR runtime: reload failed: %s\n", dlerror());
        return;
    }

    rt->last_modified = mtime;
    rt->init     = (void (*)(void))dlsym(rt->dl_handle, "dsr_init");
    rt->update   = (void (*)(double))dlsym(rt->dl_handle, "dsr_update");
    rt->render   = (void (*)(void))dlsym(rt->dl_handle, "dsr_render");
    rt->shutdown = (void (*)(void))dlsym(rt->dl_handle, "dsr_shutdown");
    rt->reload   = (void (*)(void))dlsym(rt->dl_handle, "dsr_reload");

    if (rt->init) rt->init();
}

void runtime_stop(dsr_runtime *rt) {
    if (rt->shutdown) rt->shutdown();
    if (rt->dl_handle) dlclose(rt->dl_handle);
    free(rt->lib_path);
    memset(rt, 0, sizeof(*rt));
}
