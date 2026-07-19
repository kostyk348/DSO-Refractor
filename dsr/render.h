#ifndef DSR_RENDER_H
#define DSR_RENDER_H

#include "core.h"
#include "mesh.h"
#include "terrain.h"

/* Renderer abstraction (OpenGL 3.3+)
 * DSO philosophy: SoA vertex layout, arena-allocated draw buffers
 */

typedef struct dsr_renderer dsr_renderer;

#define RENDER_MAX_MATERIALS 16

typedef struct {
    u32 index_offset;   /* in *byte* offset into the EBO (for glDrawRangeElements) */
    u32 index_count;
    u32 texture_id;     /* OpenGL texture name, 0 = no texture */
} render_material;

typedef struct {
    u32 vao, vbo, ebo;
    u32 index_count;    /* total indices */
    u32 material_count;
    render_material materials[RENDER_MAX_MATERIALS];
} render_mesh;

typedef struct {
    u32 tex_id;
    i32 width, height;
} render_texture;

/* Create renderer */
dsr_renderer *render_create(void *window_handle);

/* Destroy renderer */
void render_destroy(dsr_renderer *r);

/* Begin/end frame */
void render_begin(dsr_renderer *r, f32 view_width, f32 view_height);
void render_end(dsr_renderer *r);

/* Camera: sets projection * view matrix used by all draw calls */
void render_set_camera(dsr_renderer *r, vec3 pos, vec3 target, f32 fov);

/* Draw mesh */
render_mesh render_mesh_upload(dsr_renderer *r, dsr_mesh *m);
void render_mesh_draw(dsr_renderer *r, render_mesh *rm, mat4 *transform);

/* Draw terrain */
void render_terrain_upload(dsr_renderer *r, dsr_terrain *t, render_mesh *out_mesh);
void render_terrain_draw(dsr_renderer *r, dsr_terrain *t, render_mesh *rm, u32 tex_id);

/* Load texture from file */
render_texture render_texture_load(dsr_renderer *r, const char *path);

/* Debug wireframe toggle */
void render_set_wireframe(dsr_renderer *r, bool wireframe);

/* Debug: draw a colored triangle at position */
void render_debug_triangle(dsr_renderer *r, f32 x, f32 y, f32 z, f32 r_col, f32 g_col, f32 b_col);
/* Debug: draw a large cyan quad at position (ignores depth) */
void render_debug_quad(dsr_renderer *r, f32 x, f32 y, f32 z, f32 size);

/* Load DDS texture from raw buffer, returns OpenGL texture name */
u32 render_texture_load_dds(const u8 *data, size_t size);

/* Bind a texture to a unit */
void render_texture_bind(u32 tex, u32 unit);

/* --- Matrix utilities (for building instance transforms) --- */
void mat4_identity(mat4 *out);
void mat4_translate(mat4 *out, f32 x, f32 y, f32 z);
void mat4_rotate_x(mat4 *out, f32 deg);
void mat4_rotate_y(mat4 *out, f32 deg);
void mat4_rotate_z(mat4 *out, f32 deg);
void mat4_scale(mat4 *out, f32 x, f32 y, f32 z);
void mat4_mul(mat4 *out, mat4 *a, mat4 *b);

#endif /* DSR_RENDER_H */
