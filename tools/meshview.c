/*
 * meshview — .sm/.bundledmesh/.staticmesh viewer (CLI dump)
 * Usage: meshview <file.sm>
 */

#include "../dsr/dsr.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: meshview <meshfile>\n");
        return 1;
    }

    dsr_mesh m;
    if (!mesh_load_file(&m, argv[1])) {
        fprintf(stderr, "Failed to load mesh: %s\n", argv[1]);
        return 1;
    }

    mesh_dump(&m);
    mesh_destroy(&m);
    return 0;
}
