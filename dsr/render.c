#include "render.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* OpenGL headers — platform-dependent */
#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#else
#  include <GL/glew.h>
#  include <GL/gl.h>
#endif

struct dsr_renderer {
    u32   shader_program;
    u32   terrain_shader;
    u32   vao_quad;
    bool  wireframe;
    f32   view_width;
    f32   view_height;
    /* Camera */
    mat4  view;
    mat4  proj;
    bool  camera_dirty;
    /* Uniform locations (main shader) */
    i32   loc_mvp;
    i32   loc_model;
    i32   loc_color;
    i32   loc_tex;
};

/* --- Minimal shaders --- */

static const char *vert_shader_src =
    "#version 330\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aNormal;\n"
    "layout(location=2) in vec2 aTexCoord;\n"
    "uniform mat4 uMVP;\n"
    "uniform mat4 uModel;\n"
    "out vec3 vNormal;\n"
    "out vec2 vTexCoord;\n"
    "void main() {\n"
    "  gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "  vNormal = mat3(uModel) * aNormal;\n"
    "  vTexCoord = aTexCoord;\n"
    "}\n";

static const char *frag_shader_src =
    "#version 330\n"
    "uniform vec4 uColor;\n"
    "uniform sampler2D uTex;\n"
    "uniform int uUseTex;\n"
    "in vec3 vNormal;\n"
    "in vec2 vTexCoord;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "  vec3 light = normalize(vec3(0.5, 0.8, 0.6));\n"
    "  float diff = max(dot(vNormal, light), 0.2);\n"
    "  vec4 color = uColor;\n"
    "  if (uUseTex != 0) color *= texture(uTex, vTexCoord);\n"
    "  FragColor = vec4(color.rgb * diff, color.a);\n"
    "}\n";

/* --- Terrain shader --- */

static const char *terrain_vert_src =
    "#version 330\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aNormal;\n"
    "layout(location=2) in vec2 aTexCoord;\n"
    "uniform mat4 uMVP;\n"
    "out vec3 fNormal;\n"
    "out vec2 fTexCoord;\n"
    "void main() {\n"
    "  gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "  fNormal = aNormal;\n"
    "  fTexCoord = aTexCoord;\n"
    "}\n";

static const char *terrain_frag_src =
    "#version 330\n"
    "uniform sampler2D uTex;\n"
    "uniform int uUseTex;\n"
    "in vec3 fNormal;\n"
    "in vec2 fTexCoord;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "  vec3 light = normalize(vec3(0.5, 0.8, 0.6));\n"
    "  float diff = max(dot(normalize(fNormal), light), 0.2);\n"
    "  vec4 color = vec4(0.3, 0.6, 0.2, 1.0);\n"
    "  if (uUseTex != 0) color = texture(uTex, fTexCoord * 64.0);\n"
    "  FragColor = vec4(color.rgb * diff, 1.0);\n"
    "}\n";

/* --- Shader compilation helper --- */

static u32 compile_shader(const char *vert_src, const char *frag_src) {
    u32 vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vert_src, NULL);
    glCompileShader(vs);

    u32 fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &frag_src, NULL);
    glCompileShader(fs);

    u32 prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    i32 success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, NULL, log);
        fprintf(stderr, "Shader link error: %s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

/* --- Math helpers --- */

static void mat4_multiply(mat4 *out, mat4 *a, mat4 *b) {
    mat4 t;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            int idx = c * 4 + r;
            t.e[idx] = 0;
            for (int k = 0; k < 4; k++)
                t.e[idx] += a->e[k * 4 + r] * b->e[c * 4 + k];
        }
    *out = t;
}

static void look_at(const vec3 *eye, const vec3 *target, const vec3 *up, mat4 *out) {
    vec3 fwd = { target->x - eye->x, target->y - eye->y, target->z - eye->z };
    f32 flen = sqrtf(fwd.x*fwd.x + fwd.y*fwd.y + fwd.z*fwd.z);
    if (flen > 0) { fwd.x /= flen; fwd.y /= flen; fwd.z /= flen; }

    vec3 side = { fwd.z*up->y - fwd.y*up->z, fwd.x*up->z - fwd.z*up->x, fwd.y*up->x - fwd.x*up->y };
    f32 slen = sqrtf(side.x*side.x + side.y*side.y + side.z*side.z);
    if (slen > 0) { side.x /= slen; side.y /= slen; side.z /= slen; }

    vec3 u = { side.y*fwd.z - side.z*fwd.y, side.z*fwd.x - side.x*fwd.z, side.x*fwd.y - side.y*fwd.x };

    out->e[0]  = side.x;  out->e[1]  = u.x;  out->e[2]  = -fwd.x;  out->e[3]  = 0;
    out->e[4]  = side.y;  out->e[5]  = u.y;  out->e[6]  = -fwd.y;  out->e[7]  = 0;
    out->e[8]  = side.z;  out->e[9]  = u.z;  out->e[10] = -fwd.z;  out->e[11] = 0;
    out->e[12] = -side.x*eye->x - side.y*eye->y - side.z*eye->z;
    out->e[13] = -u.x*eye->x - u.y*eye->y - u.z*eye->z;
    out->e[14] =  fwd.x*eye->x + fwd.y*eye->y + fwd.z*eye->z;
    out->e[15] = 1;
}

static void perspective_mat(f32 fov_y, f32 aspect, f32 n, f32 f, mat4 *out) {
    f32 fov_rad = fov_y * 3.14159f / 180.0f;
    f32 t = tanf(fov_rad / 2);
    memset(out, 0, sizeof(*out));
    out->e[0]  = 1.0f / (aspect * t);
    out->e[5]  = 1.0f / t;
    out->e[10] = -(f + n) / (f - n);
    out->e[11] = -1.0f;
    out->e[14] = -2.0f * f * n / (f - n);
}

/* --- Matrix utilities --- */

void mat4_identity(mat4 *out) {
    memset(out, 0, sizeof(*out));
    out->e[0] = out->e[5] = out->e[10] = out->e[15] = 1.0f;
}

void mat4_translate(mat4 *out, f32 x, f32 y, f32 z) {
    mat4_identity(out);
    out->e[12] = x;
    out->e[13] = y;
    out->e[14] = z;
}

void mat4_rotate_y(mat4 *out, f32 deg) {
    f32 rad = deg * 3.14159f / 180.0f;
    f32 c = cosf(rad), s = sinf(rad);
    memset(out, 0, sizeof(*out));
    out->e[0] = c;  out->e[2] = s;
    out->e[5] = 1.0f;
    out->e[8] = -s; out->e[10] = c;
    out->e[15] = 1.0f;
}

void mat4_rotate_x(mat4 *out, f32 deg) {
    f32 rad = deg * 3.14159f / 180.0f;
    f32 c = cosf(rad), s = sinf(rad);
    memset(out, 0, sizeof(*out));
    out->e[0] = 1.0f;
    out->e[5] = c;  out->e[6] = -s;
    out->e[9] = s;  out->e[10] = c;
    out->e[15] = 1.0f;
}

void mat4_rotate_z(mat4 *out, f32 deg) {
    f32 rad = deg * 3.14159f / 180.0f;
    f32 c = cosf(rad), s = sinf(rad);
    memset(out, 0, sizeof(*out));
    out->e[0] = c;  out->e[1] = -s;
    out->e[4] = s;  out->e[5] = c;
    out->e[10] = 1.0f;
    out->e[15] = 1.0f;
}

void mat4_scale(mat4 *out, f32 x, f32 y, f32 z) {
    memset(out, 0, sizeof(*out));
    out->e[0] = x;
    out->e[5] = y;
    out->e[10] = z;
    out->e[15] = 1.0f;
}

void mat4_mul(mat4 *out, mat4 *a, mat4 *b) {
    mat4_multiply(out, a, b);
}

/* --- Public API --- */

dsr_renderer *render_create(void *window_handle) {
    (void)window_handle;
    dsr_renderer *r = (dsr_renderer*)calloc(1, sizeof(dsr_renderer));
    if (!r) return NULL;

    r->shader_program = compile_shader(vert_shader_src, frag_shader_src);
    r->terrain_shader = compile_shader(terrain_vert_src, terrain_frag_src);

    glUseProgram(r->shader_program);
    r->loc_mvp   = glGetUniformLocation(r->shader_program, "uMVP");
    r->loc_model = glGetUniformLocation(r->shader_program, "uModel");
    r->loc_color = glGetUniformLocation(r->shader_program, "uColor");
    r->loc_tex   = glGetUniformLocation(r->shader_program, "uUseTex");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.5f, 0.7f, 0.9f, 1.0f);

    r->camera_dirty = true;
    return r;
}

void render_destroy(dsr_renderer *r) {
    if (!r) return;
    glDeleteProgram(r->shader_program);
    glDeleteProgram(r->terrain_shader);
    free(r);
}

void render_begin(dsr_renderer *r, f32 view_width, f32 view_height) {
    r->view_width  = view_width;
    r->view_height = view_height;
    glViewport(0, 0, (i32)view_width, (i32)view_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, r->wireframe ? GL_LINE : GL_FILL);
    glDepthFunc(GL_LEQUAL);
}

void render_end(dsr_renderer *r) {
    (void)r;
}

void render_set_camera(dsr_renderer *r, vec3 pos, vec3 target, f32 fov) {
    vec3 up = {0, 1, 0};
    look_at(&pos, &target, &up, &r->view);
    f32 aspect = r->view_width / r->view_height;
    perspective_mat(fov, aspect, 1.0f, 10000.0f, &r->proj);
    r->camera_dirty = true;
}

render_mesh render_mesh_upload(dsr_renderer *r, dsr_mesh *m) {
    render_mesh rm = {0};
    (void)r;

    if (m->geom_count == 0 || m->geoms[0].lod_count == 0) return rm;
    mesh_lod *lod = &m->geoms[0].lods[0];
    if (lod->vertex_count == 0 || lod->index_count == 0) return rm;

    glGenVertexArrays(1, &rm.vao);
    glGenBuffers(1, &rm.vbo);
    glGenBuffers(1, &rm.ebo);

    glBindVertexArray(rm.vao);

    size_t vert_bytes = (size_t)lod->vertex_count * lod->vertex_stride * sizeof(f32);
    glBindBuffer(GL_ARRAY_BUFFER, rm.vbo);
    glBufferData(GL_ARRAY_BUFFER, vert_bytes, lod->vertices, GL_STATIC_DRAW);

    size_t idx_bytes = (size_t)lod->index_count * sizeof(u16);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rm.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_bytes, lod->indices, GL_STATIC_DRAW);

    /* Use vertex attribute descriptors when available (bundled mesh) */
    if (lod->attribs && lod->attrib_count > 0) {
        for (u32 ai = 0; ai < lod->attrib_count; ai++) {
            u32 loc;
            u32 comps;
            if (lod->attribs[ai].usage == USAGE_POSITION) {
                loc = 0; comps = 3;
            } else if (lod->attribs[ai].usage == USAGE_NORMAL) {
                loc = 1; comps = 3;
            } else if (lod->attribs[ai].usage == USAGE_TEXCOORD) {
                loc = 2; comps = 2;
            } else {
                continue;
            }
            glVertexAttribPointer(loc, comps, GL_FLOAT, GL_FALSE,
                lod->vertex_stride * sizeof(f32), (void*)(uintptr_t)lod->attribs[ai].offset);
            glEnableVertexAttribArray(loc);
        }
    } else {
        /* Default layout for SM (v2/v9): vertex stride and offset heuristic */
        u32 pos_off = 0, norm_off = 0, tex_off = 0;
        u32 fs = lod->vertex_stride;  /* floats per vertex (stride/4 for v2, raw for v9) */
        /* v2 always uses 8 floats: pos(3) + norm(3) + tex(2) */
        if (fs == 8) {
            pos_off = 0; norm_off = 3; tex_off = 6;
        }
        /* v9 common strides */
        else if (fs == 10) {
            /* v9 stride=40: pos(3)+W(1)+blend(2)+norm(3)+tex(2) */
            pos_off = 0; norm_off = 6; tex_off = 8;
        }
        else if (fs == 9) {
            /* v9 stride=36: pos(3)+W(1)+norm(3)+tex(2) */
            pos_off = 0; norm_off = 4; tex_off = 7;
        }
        else {
            pos_off = 0; norm_off = 3; tex_off = 6;
        }
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, fs * sizeof(f32), (void*)(uintptr_t)(pos_off * sizeof(f32)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, fs * sizeof(f32), (void*)(uintptr_t)(norm_off * sizeof(f32)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, fs * sizeof(f32), (void*)(uintptr_t)(tex_off * sizeof(f32)));
        glEnableVertexAttribArray(2);
    }

    rm.index_count = lod->index_count;

    /* Populate per-material index ranges */
    rm.material_count = lod->material_count;
    if (rm.material_count > RENDER_MAX_MATERIALS)
        rm.material_count = RENDER_MAX_MATERIALS;
    for (u32 mi = 0; mi < rm.material_count; mi++) {
        mesh_material *mat = &lod->materials[mi];
        rm.materials[mi].index_offset = mat->index_offset * sizeof(u16); /* byte offset for EBO */
        rm.materials[mi].index_count  = mat->index_count;
        rm.materials[mi].texture_id   = 0;
    }
    return rm;
}

void render_mesh_draw(dsr_renderer *r, render_mesh *rm, mat4 *transform) {
    glUseProgram(r->shader_program);

    mat4 model;
    if (transform) model = *transform;
    else { memset(&model, 0, sizeof(model)); model.e[0] = model.e[5] = model.e[10] = model.e[15] = 1.0f; }

    mat4 mvp;
    mat4_multiply(&mvp, &r->proj, &r->view);
    mat4 view_model;
    mat4_multiply(&view_model, &r->view, &model);
    mat4_multiply(&mvp, &r->proj, &view_model);

    glUniformMatrix4fv(r->loc_mvp, 1, GL_FALSE, mvp.e);
    glUniformMatrix4fv(r->loc_model, 1, GL_FALSE, model.e);
    glUniform4f(r->loc_color, 0.7f, 0.7f, 0.7f, 1.0f);

    glBindVertexArray(rm->vao);

    /* Draw each material subset with its texture */
    if (rm->material_count > 0) {
        for (u32 mi = 0; mi < rm->material_count; mi++) {
            render_material *mat = &rm->materials[mi];
            if (mat->index_count == 0) continue;

            if (mat->texture_id) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mat->texture_id);
                glUniform1i(r->loc_tex, 1);  /* uUseTex = 1 */
            } else {
                glUniform1i(r->loc_tex, 0);  /* uUseTex = 0 */
            }

            glDrawRangeElements(GL_TRIANGLES, 0, rm->index_count - 1,
                                (i32)mat->index_count, GL_UNSIGNED_SHORT,
                                (void*)(uintptr_t)mat->index_offset);
        }
    } else {
        /* Fallback: draw all indices without texture */
        glUniform1i(r->loc_tex, 0);
        glDrawElements(GL_TRIANGLES, (i32)rm->index_count, GL_UNSIGNED_SHORT, 0);
    }
}

void render_terrain_upload(dsr_renderer *r, dsr_terrain *t, render_mesh *out_mesh) {
    (void)r;
    memset(out_mesh, 0, sizeof(*out_mesh));

    u32 w = t->width;
    u32 h = t->height;
    u32 vert_count = w * h;
    u32 index_count = (w - 1) * (h - 1) * 6;

    size_t vb_size = (size_t)vert_count * 8 * sizeof(f32);
    f32 *vb = (f32*)malloc(vb_size);

    for (u32 y = 0; y < h; y++) {
        for (u32 x = 0; x < w; x++) {
            f32 hgt = t->heights[y * w + x];
            f32 wx = (f32)x * t->scale_xz;
            f32 wz = (f32)y * t->scale_xz;
            u32 idx = (y * w + x) * 8;
            vb[idx + 0] = wx;
            vb[idx + 1] = hgt;
            vb[idx + 2] = wz;
            vec3 n = terrain_sample_normal(t, wx, wz);
            vb[idx + 3] = n.x;
            vb[idx + 4] = n.y;
            vb[idx + 5] = n.z;
            vb[idx + 6] = (f32)x / (f32)(w - 1);
            vb[idx + 7] = (f32)y / (f32)(h - 1);
        }
    }

    /* Use u32 indices — u16 overflows at 65535 vertices */
    size_t ib_size = (size_t)index_count * sizeof(u32);
    u32 *ib = (u32*)malloc(ib_size);
    u32 idx = 0;
    for (u32 y = 0; y < h - 1; y++) {
        for (u32 x = 0; x < w - 1; x++) {
            u32 a = y * w + x;
            u32 b = y * w + x + 1;
            u32 c = (y + 1) * w + x;
            u32 d = (y + 1) * w + x + 1;
            ib[idx++] = a; ib[idx++] = b; ib[idx++] = c;
            ib[idx++] = b; ib[idx++] = d; ib[idx++] = c;
        }
    }

    glGenVertexArrays(1, &out_mesh->vao);
    glGenBuffers(1, &out_mesh->vbo);
    glGenBuffers(1, &out_mesh->ebo);
    out_mesh->index_count = index_count;

    glBindVertexArray(out_mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, out_mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, vb_size, vb, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib_size, ib, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)(3 * sizeof(f32)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)(6 * sizeof(f32)));
    glEnableVertexAttribArray(2);

    free(vb);
    free(ib);
}

void render_terrain_draw(dsr_renderer *r, dsr_terrain *t, render_mesh *rm, u32 tex_id) {
    (void)t;
    glUseProgram(r->terrain_shader);

    mat4 mvp;
    mat4_multiply(&mvp, &r->proj, &r->view);

    glUniformMatrix4fv(glGetUniformLocation(r->terrain_shader, "uMVP"), 1, GL_FALSE, mvp.e);
    glUniform1i(glGetUniformLocation(r->terrain_shader, "uUseTex"), tex_id ? 1 : 0);

    if (tex_id) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_id);
    }

    /* Push terrain back in depth buffer to prevent z-fighting with objects */
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glBindVertexArray(rm->vao);
    glDrawElements(GL_TRIANGLES, (i32)rm->index_count, GL_UNSIGNED_INT, 0);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void render_debug_triangle(dsr_renderer *r, f32 x, f32 y, f32 z, f32 r_col, f32 g_col, f32 b_col) {
    static u32 vao = 0, vbo = 0, ebo = 0;
    if (!vao) {
        f32 verts[] = { -10,0,-10, 0,1,0, 0,0, 10,0,-10, 0,1,0, 0,0, 0,10,10, 0,1,0, 0,0 };
        u16 idx[] = { 0, 1, 2 };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,32,0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,32,(void*)12); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,32,(void*)24); glEnableVertexAttribArray(2);
    }

    /* Draw with depth test OFF so it's always visible */
    glUseProgram(r->shader_program);
    glDisable(GL_DEPTH_TEST);
    mat4 model, mvp, tmp;
    mat4_translate(&model, x, y, z);
    mat4_multiply(&tmp, &r->view, &model);
    mat4_multiply(&mvp, &r->proj, &tmp);
    glUniformMatrix4fv(r->loc_mvp, 1, GL_FALSE, mvp.e);
    glUniformMatrix4fv(r->loc_model, 1, GL_FALSE, model.e);
    glUniform4f(r->loc_color, r_col, g_col, b_col, 1.0f);
    glUniform1i(r->loc_tex, 0);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);
    glEnable(GL_DEPTH_TEST);
}

void render_debug_quad(dsr_renderer *r, f32 x, f32 y, f32 z, f32 size) {
    glUseProgram(r->shader_program);
    glDisable(GL_DEPTH_TEST);
    mat4 model, mvp, tmp;
    mat4_translate(&model, x, y, z);
    mat4_multiply(&tmp, &r->view, &model);
    mat4_multiply(&mvp, &r->proj, &tmp);
    glUniformMatrix4fv(r->loc_mvp, 1, GL_FALSE, mvp.e);
    glUniformMatrix4fv(r->loc_model, 1, GL_FALSE, model.e);
    glUniform4f(r->loc_color, 0, 1, 1, 1);
    glUniform1i(r->loc_tex, 0);
    static u32 vao=0,vbo=0,ebo=0;
    if (!vao) {
        f32 v[] = { -1,0,-1, 0,0,1, 0,0, 1,0,-1, 0,0,1, 0,0, 1,0,1, 0,0,1, 0,0, -1,0,1, 0,0,1, 0,0 };
        u16 idx[] = { 0,1,2, 0,2,3 };
        glGenVertexArrays(1,&vao); glBindVertexArray(vao);
        glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
        glGenBuffers(1,&ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idx),idx,GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,32,0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,32,(void*)12); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,32,(void*)24); glEnableVertexAttribArray(2);
    }
    glBindVertexArray(vao);
    /* scale model by size */
    model.e[0] *= size; model.e[1] *= size; model.e[2] *= size;
    model.e[4] *= size; model.e[5] *= size; model.e[6] *= size;
    model.e[8] *= size; model.e[9] *= size; model.e[10] *= size;
    mat4_multiply(&tmp, &r->view, &model);
    mat4_multiply(&mvp, &r->proj, &tmp);
    glUniformMatrix4fv(r->loc_mvp, 1, GL_FALSE, mvp.e);
    glUniformMatrix4fv(r->loc_model, 1, GL_FALSE, model.e);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    glEnable(GL_DEPTH_TEST);
}

render_texture render_texture_load(dsr_renderer *r, const char *path) {
    render_texture tex = {0};
    (void)r;
    (void)path;
    return tex;
}

void render_set_wireframe(dsr_renderer *r, bool wireframe) {
    r->wireframe = wireframe;
}

/* --- DDS texture loader --- */

#define DDS_MAGIC 0x20534444 /* "DDS " little-endian */

/* DDS pixel format flags */
#define DDPF_FOURCC  0x00000004
#define DDPF_RGB     0x00000040
#define DDPF_ALPHA   0x00000002

/* DXT FOURCC codes (little-endian) */
#define FOURCC_DXT1 0x31545844 /* "DXT1" */
#define FOURCC_DXT3 0x33545844 /* "DXT3" */
#define FOURCC_DXT5 0x35545844 /* "DXT5" */

typedef struct {
    u32 size;
    u32 flags;
    u32 height;
    u32 width;
    u32 pitchOrLinearSize;
    u32 depth;
    u32 mipMapCount;
    u32 reserved1[11];
    /* DDS_PIXELFORMAT */
    u32 pf_size;
    u32 pf_flags;
    u32 fourCC;
    u32 rgbBitCount;
    u32 rBitMask;
    u32 gBitMask;
    u32 bBitMask;
    u32 aBitMask;
    u32 caps;
    u32 caps2;
    u32 caps3;
    u32 caps4;
    u32 reserved2;
} dds_header;

static u32 load_dds_from_mem(const u8 *data, size_t size) {
    if (size < sizeof(dds_header) + 4) {
        fprintf(stderr, "DDS: file too small\n");
        return 0;
    }

    const u32 *magic = (const u32*)data;
    if (*magic != DDS_MAGIC) {
        fprintf(stderr, "DDS: bad magic 0x%08x\n", *magic);
        return 0;
    }

    const dds_header *hdr = (const dds_header*)(data + 4);
    if (hdr->size != 124) {
        fprintf(stderr, "DDS: bad header size %u\n", hdr->size);
        return 0;
    }

    u32 w = hdr->width;
    u32 h = hdr->height;
    u32 mip_count = hdr->mipMapCount > 0 ? hdr->mipMapCount : 1;

    /* Determine format */
    GLenum format = 0;
    u32 block_size = 0;
    bool compressed = false;

    if (hdr->pf_flags & DDPF_FOURCC) {
        compressed = true;
        switch (hdr->fourCC) {
            case FOURCC_DXT1: format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; block_size = 8;  break;
            case FOURCC_DXT3: format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; block_size = 16; break;
            case FOURCC_DXT5: format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; block_size = 16; break;
            default:
                fprintf(stderr, "DDS: unsupported FOURCC 0x%08x\n", hdr->fourCC);
                return 0;
        }
    } else if (hdr->pf_flags & DDPF_RGB) {
        compressed = false;
        block_size = hdr->rgbBitCount / 8;
        if (hdr->rgbBitCount == 24) format = GL_RGB;
        else if (hdr->rgbBitCount == 32) format = GL_RGBA;
        else {
            fprintf(stderr, "DDS: unsupported RGB bit count %u\n", hdr->rgbBitCount);
            return 0;
        }
    } else {
        fprintf(stderr, "DDS: unsupported pixel format flags 0x%08x\n", hdr->pf_flags);
        return 0;
    }

    u32 tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    const u8 *pixels = data + 4 + sizeof(dds_header);
    size_t offset = 0;
    u32 mip_w = w, mip_h = h;

    for (u32 level = 0; level < mip_count; level++) {
        if (compressed) {
            u32 bw = (mip_w + 3) / 4;
            u32 bh = (mip_h + 3) / 4;
            u32 data_size = bw * bh * block_size;
            if (offset + data_size > size) break;
            glCompressedTexImage2D(GL_TEXTURE_2D, level, format,
                                   (GLsizei)mip_w, (GLsizei)mip_h, 0,
                                   (GLsizei)data_size, pixels + offset);
            offset += data_size;
        } else {
            u32 row_size = mip_w * block_size;
            u32 data_size = row_size * mip_h;
            if (offset + data_size > size) break;
            glTexImage2D(GL_TEXTURE_2D, level, format == GL_RGBA ? GL_RGBA : GL_RGB,
                         (GLsizei)mip_w, (GLsizei)mip_h, 0,
                         format, GL_UNSIGNED_BYTE, pixels + offset);
            offset += data_size;
        }

        if (mip_w > 1) mip_w >>= 1;
        if (mip_h > 1) mip_h >>= 1;
    }

    fprintf(stderr, "  Loaded DDS: %ux%u mips=%u fmt=0x%x tex=%u\n", w, h, mip_count, format, tex);
    return tex;
}

u32 render_texture_load_dds(const u8 *data, size_t size) {
    return load_dds_from_mem(data, size);
}

void render_texture_bind(u32 tex, u32 unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex);
}
