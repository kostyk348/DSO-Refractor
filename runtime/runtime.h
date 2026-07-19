#ifndef DSR_RUNTIME_H
#define DSR_RUNTIME_H

/*
 * Hot-reload runtime layer.
 * Loads the game engine as a shared library and reloads on change.
 * DSO philosophy: iteration speed matters.
 */

typedef struct {
    void *dl_handle;
    char *lib_path;
    i64   last_modified;

    /* Function pointers */
    void (*init)(void);
    void (*update)(double dt);
    void (*render)(void);
    void (*shutdown)(void);
    void (*reload)(void);
} dsr_runtime;

/* Start runtime with engine shared library */
int  runtime_start(dsr_runtime *rt, const char *lib_path);

/* Check for hot-reload */
void runtime_check_reload(dsr_runtime *rt);

/* Shutdown runtime */
void runtime_stop(dsr_runtime *rt);

#endif /* DSR_RUNTIME_H */
