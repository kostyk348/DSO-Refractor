#ifndef DSR_TERRAIN_H
#define DSR_TERRAIN_H

#include "core.h"

/* Refractor Engine heightmap terrain
 * BF1942 uses RAW heightmaps + detail textures + color maps
 */

#define TERRAIN_MAX_SIZE 2048
#define TERRAIN_MIN_SIZE 64

typedef struct {
    u32    width;           /* in vertices */
    u32    height;
    f32    scale_xz;        /* world units per vertex */
    f32    scale_y;         /* height multiplier */
    f32   *heights;         /* [width * height] */
    f32    min_height;
    f32    max_height;
    /* Color map (optional) */
    u8    *color_map;       /* [width * height * 3] RGB */
    /* Low-res shadows */
    u8    *lightmap;        /* [width * height] */
    /* Detail texture layers */
    u32    layer_count;
    char   layer_names[4][64];
    /* Internal */
    arena  terrain_arena;
} dsr_terrain;

/* Create flat terrain */
bool terrain_create(dsr_terrain *t, u32 width, u32 height, f32 scale_xz, f32 scale_y);

/* Load heightmap from RAW file (f32 or u16) */
bool terrain_load_raw(dsr_terrain *t, const char *raw_path, u32 width, u32 height, f32 scale_y);

/* Load heightmap from f32 buffer */
void terrain_load_heights(dsr_terrain *t, const f32 *heights);

/* Sample height at world position */
f32 terrain_sample_height(dsr_terrain *t, f32 world_x, f32 world_z);

/* Get normal at world position */
vec3 terrain_sample_normal(dsr_terrain *t, f32 world_x, f32 world_z);

/* Destroy terrain */
void terrain_destroy(dsr_terrain *t);

#endif /* DSR_TERRAIN_H */
