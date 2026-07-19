#include "terrain.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

bool terrain_create(dsr_terrain *t, u32 width, u32 height, f32 scale_xz, f32 scale_y) {
    memset(t, 0, sizeof(*t));

    if (width  < TERRAIN_MIN_SIZE || width  > TERRAIN_MAX_SIZE) return false;
    if (height < TERRAIN_MIN_SIZE || height > TERRAIN_MAX_SIZE) return false;

    t->width   = width;
    t->height  = height;
    t->scale_xz = scale_xz;
    t->scale_y  = scale_y;

    size_t h_sz = (size_t)width * height * sizeof(f32);
    t->terrain_arena = arena_create(h_sz + h_sz / 2 + 4096);
    t->heights = (f32*)arena_alloc(&t->terrain_arena, h_sz, 4);
    if (!t->heights) { arena_destroy(&t->terrain_arena); return false; }

    /* Default flat */
    memset(t->heights, 0, h_sz);
    t->min_height = 0;
    t->max_height = 0;

    return true;
}

bool terrain_load_raw(dsr_terrain *t, const char *raw_path, u32 width, u32 height, f32 scale_y) {
    if (!terrain_create(t, width, height, 1.0f, scale_y)) return false;

    FILE *f = fopen(raw_path, "rb");
    if (!f) { terrain_destroy(t); return false; }

    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    rewind(f);

    size_t h_sz = (size_t)width * height * sizeof(f32);
    if (file_size == h_sz) {
        /* f32 raw */
        fread(t->heights, 1, h_sz, f);
    } else if (file_size == (size_t)width * height * 2) {
        /* u16 raw */
        u16 *u16buf = (u16*)malloc(file_size);
        if (!u16buf) { fclose(f); terrain_destroy(t); return false; }
        fread(u16buf, 1, file_size, f);
        for (size_t i = 0; i < (size_t)width * height; i++) {
            t->heights[i] = (f32)u16buf[i] / 65535.0f * scale_y;
        }
        free(u16buf);
    } else {
        fclose(f);
        terrain_destroy(t);
        return false;
    }

    fclose(f);

    /* Compute bounds */
    t->min_height = t->heights[0];
    t->max_height = t->heights[0];
    for (size_t i = 1; i < (size_t)width * height; i++) {
        if (t->heights[i] < t->min_height) t->min_height = t->heights[i];
        if (t->heights[i] > t->max_height) t->max_height = t->heights[i];
    }

    return true;
}

void terrain_load_heights(dsr_terrain *t, const f32 *heights) {
    size_t h_sz = (size_t)t->width * t->height * sizeof(f32);
    memcpy(t->heights, heights, h_sz);

    t->min_height = t->heights[0];
    t->max_height = t->heights[0];
    for (size_t i = 1; i < (size_t)t->width * t->height; i++) {
        if (t->heights[i] < t->min_height) t->min_height = t->heights[i];
        if (t->heights[i] > t->max_height) t->max_height = t->heights[i];
    }
}

f32 terrain_sample_height(dsr_terrain *t, f32 world_x, f32 world_z) {
    /* Bilinear interpolation */
    f32 u = world_x / t->scale_xz;
    f32 v = world_z / t->scale_xz;

    i32 x0 = (i32)floorf(u);
    i32 y0 = (i32)floorf(v);
    i32 x1 = x0 + 1;
    i32 y1 = y0 + 1;

    /* Clamp */
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x1 >= (i32)t->width)  x1 = (i32)t->width - 1;
    if (y1 >= (i32)t->height) y1 = (i32)t->height - 1;

    f32 fx = u - floorf(u);
    f32 fy = v - floorf(v);

    f32 h00 = t->heights[y0 * t->width + x0];
    f32 h10 = t->heights[y0 * t->width + x1];
    f32 h01 = t->heights[y1 * t->width + x0];
    f32 h11 = t->heights[y1 * t->width + x1];

    f32 top = h00 + (h10 - h00) * fx;
    f32 bot = h01 + (h11 - h01) * fx;
    return top + (bot - top) * fy;
}

vec3 terrain_sample_normal(dsr_terrain *t, f32 world_x, f32 world_z) {
    f32 h = terrain_sample_height(t, world_x, world_z);
    f32 hx = terrain_sample_height(t, world_x + t->scale_xz, world_z);
    f32 hz = terrain_sample_height(t, world_x, world_z + t->scale_xz);

    vec3 n;
    n.x = -(hx - h);
    n.y =  t->scale_xz;
    n.z = -(hz - h);

    f32 len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    return n;
}

void terrain_destroy(dsr_terrain *t) {
    arena_destroy(&t->terrain_arena);
    memset(t, 0, sizeof(*t));
}
