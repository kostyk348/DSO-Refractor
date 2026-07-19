#include "../dsr/dsr.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static bool has_texture_ext(const char *path) {
    const char *exts[] = {".dds", ".bmp", ".tga", ".jpg", ".png", ".raw"};
    size_t len = strlen(path);
    for (size_t e = 0; e < sizeof(exts)/sizeof(exts[0]); e++) {
        size_t elen = strlen(exts[e]);
        if (len >= elen) {
            bool match = true;
            for (size_t i = 0; i < elen; i++) {
                char c = path[len - elen + i];
                char ec = exts[e][i];
                if (tolower((unsigned char)c) != ec) { match = false; break; }
            }
            if (match) return true;
        }
    }
    return false;
}

static bool name_contains(const char *path, const char *keyword) {
    return strstr(path, keyword) != NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: texture_list <rfa_file> [filter]\n");
        fprintf(stderr, "  filter: 'all' (default), 'terrain', 'sky'\n");
        return 1;
    }
    const char *filter = argc > 2 ? argv[2] : "all";

    rfa_archive rfa;
    rfa_error err = rfa_open(&rfa, argv[1]);
    if (err != RFA_OK) {
        fprintf(stderr, "Failed to open %s\n", argv[1]);
        return 1;
    }

    printf("=== %s (%u entries) ===\n", rfa.path, rfa.entry_count);

    int count = 0;
    for (u32 i = 0; i < rfa.entry_count; i++) {
        rfa_entry *e = &rfa.entries[i];

        if (strcmp(filter, "terrain") == 0) {
            if (!has_texture_ext(e->path)) continue;
            if (!name_contains(e->path, "terrain") &&
                !name_contains(e->path, "Tx") &&
                !name_contains(e->path, "detail") &&
                !name_contains(e->path, "Terrain") &&
                !name_contains(e->path, "color") &&
                !name_contains(e->path, "sand") &&
                !name_contains(e->path, "grass") &&
                !name_contains(e->path, "Material") &&
                !name_contains(e->path, "Heightmap") &&
                !name_contains(e->path, "Lightmap") &&
                !name_contains(e->path, "Palette") &&
                !name_contains(e->path, "Pathfinding")) continue;
        } else if (strcmp(filter, "sky") == 0) {
            if (!has_texture_ext(e->path) && !name_contains(e->path, ".rcm")) continue;
            if (!name_contains(e->path, "sky") && !name_contains(e->path, "Sky") &&
                !name_contains(e->path, "env_") && !name_contains(e->path, "ENVMAP")) continue;
        } else {
            if (!has_texture_ext(e->path)) continue;
        }

        printf("  %s\n", e->path);
        count++;
    }

    printf("  Total: %d texture files\n", count);
    rfa_close(&rfa);
    return 0;
}
