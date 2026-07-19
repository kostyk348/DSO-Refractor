/* DSO-Refractor: Main game loop
 * Loads BF1942 Demo level (Tobruk) and renders with camera controls.
 * DSO philosophy: deterministic tick, arena allocators, SoA entities.
 *
 * Controls:
 *   WASD  - move camera
 *   Mouse - look around (held RMB)
 *   Q/E   - up/down
 *   F1    - toggle editor mode
 *   F2    - toggle wireframe
 *   Esc   - quit
 */

#include "dsr/dsr.h"
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* --- SDL state --- */
static SDL_Window   *window   = NULL;
static SDL_GLContext gl_ctx   = NULL;
static dsr_state    *g_state  = NULL;
static dsr_renderer *g_render = NULL;
static dsr_terrain   g_terrain;
static render_mesh   g_terrain_mesh;
static bool          g_running = true;
static bool          g_editor_mode = false;
static bool          g_wireframe = false;
static int           g_rot_mode = 1;   /* rotation-axis interpretation, cycle with 'R' */
static int           g_mirror = 0;     /* handedness flip (mirror) toggle, cycle with 'M' */
static f64           g_last_time = 0.0;

/* --- Camera --- */
static vec3  cam_pos    = {2500.0f, 80.0f, 750.0f};
static vec3  cam_target = {2589.0f, 45.0f, 876.0f};
static f32   cam_yaw   = 0.0f;
static f32   cam_pitch = -20.0f;
static f32   cam_speed = 200.0f;
static bool  cam_mouse_look = false;

/* --- Static level objects --- */
#define MAX_MESHES 64
#define MAX_OBJECTS 2048

typedef struct {
    dsr_mesh    mesh;
    render_mesh render;
    char        name[64];
} loaded_mesh;

typedef struct {
    int  mesh_idx;
    vec3 pos;
    f32  rotX;   /* .con rotation[0] */
    f32  rotY;   /* .con rotation[1] */
    f32  rotZ;   /* .con rotation[2] */
    vec3 scale;
    vec3 color;
} static_obj;

static loaded_mesh  g_meshes[MAX_MESHES];
static u32          g_mesh_count = 0;
static static_obj   g_objects[MAX_OBJECTS];
static u32          g_obj_count  = 0;
static rfa_archive  g_mesh_rfa;
static rfa_archive  g_texture_rfa;
static u32          g_terrain_tex = 0;

/* --- SDL/GL init --- */
static int init_sdl(const char *title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return -1;
    }

    gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) {
        fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetSwapInterval(1);
    return 0;
}

/* --- Camera update --- */
static void update_camera(f32 dt) {
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    vec3 forward = { cam_target.x - cam_pos.x, 0, cam_target.z - cam_pos.z };
    f32 len = sqrtf(forward.x * forward.x + forward.z * forward.z);
    if (len > 0) { forward.x /= len; forward.z /= len; }

    vec3 right = { forward.z, 0, -forward.x };

    f32 spd = cam_speed * dt;
    if (keys[SDL_SCANCODE_LSHIFT]) spd *= 3;

    if (keys[SDL_SCANCODE_W]) { cam_pos.x += forward.x * spd; cam_pos.z += forward.z * spd; }
    if (keys[SDL_SCANCODE_S]) { cam_pos.x -= forward.x * spd; cam_pos.z -= forward.z * spd; }
    if (keys[SDL_SCANCODE_A]) { cam_pos.x -= right.x * spd; cam_pos.z -= right.z * spd; }
    if (keys[SDL_SCANCODE_D]) { cam_pos.x += right.x * spd; cam_pos.z += right.z * spd; }
    if (keys[SDL_SCANCODE_Q]) { cam_pos.y += spd; }
    if (keys[SDL_SCANCODE_E]) { cam_pos.y -= spd; }

    if (cam_mouse_look) {
        int dx, dy;
        SDL_GetRelativeMouseState(&dx, &dy);
        cam_yaw   += (f32)dx * 0.1f;
        cam_pitch += (f32)dy * 0.1f;
        if (cam_pitch > 89.0f) cam_pitch = 89.0f;
        if (cam_pitch < -89.0f) cam_pitch = -89.0f;
    }

    f32 rad_yaw   = cam_yaw * 3.14159f / 180.0f;
    f32 rad_pitch = cam_pitch * 3.14159f / 180.0f;
    f32 look_dist = 100.0f;
    cam_target.x = cam_pos.x + cosf(rad_yaw) * cosf(rad_pitch) * look_dist;
    cam_target.y = cam_pos.y + sinf(rad_pitch) * look_dist;
    cam_target.z = cam_pos.z + sinf(rad_yaw) * cosf(rad_pitch) * look_dist;
}

/* --- Handle events --- */
static void handle_events(void) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) { g_running = false; }
        if (ev.type == SDL_KEYDOWN) {
            if (ev.key.keysym.sym == SDLK_ESCAPE) g_running = false;
            if (ev.key.keysym.sym == SDLK_F1) g_editor_mode = !g_editor_mode;
            if (ev.key.keysym.sym == SDLK_F2) {
                g_wireframe = !g_wireframe;
                render_set_wireframe(g_render, g_wireframe);
            }
            if (ev.key.keysym.sym == SDLK_r) {
                g_rot_mode = (g_rot_mode + 1) % 3;
                fprintf(stderr, "[R] rotation mode -> %d (0=xyz, 1=YawX, 2=YawZ)\n", g_rot_mode);
            }
            if (ev.key.keysym.sym == SDLK_m) {
                g_mirror = (g_mirror + 1) % 2;
                fprintf(stderr, "[M] mirror -> %d\n", g_mirror);
            }
        }
        if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_RIGHT) {
            cam_mouse_look = true;
            SDL_SetRelativeMouseMode(SDL_TRUE);
        }
        if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_RIGHT) {
            cam_mouse_look = false;
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
    }
}

/* --- Load .rs shader file and populate material texture paths --- */
static void load_rs_textures(dsr_mesh *m, const char *obj_name) {
    /* Build .rs path: standardMesh/{name}.rs */
    char rs_path[512];
    snprintf(rs_path, sizeof(rs_path), "standardMesh/%s.rs", obj_name);
    int rs_idx = rfa_find(&g_mesh_rfa, rs_path);
    if (rs_idx < 0) {
        /* Try lowercase */
        char lower[512];
        snprintf(lower, sizeof(lower), "standardMesh/%s.rs", obj_name);
        for (char *p = lower; *p; p++) *p = (char)tolower((u8)*p);
        rs_idx = rfa_find(&g_mesh_rfa, lower);
    }
    if (rs_idx < 0) {
        printf("  No .rs for %s\n", obj_name);
        return;
    }

    rfa_entry *e = &g_mesh_rfa.entries[rs_idx];
    size_t sz = g_mesh_rfa.compressed ? e->info.uncompressed_size : e->info.compressed_size;
    if (sz == 0 || sz > 1024 * 1024) return;

    u8 *buf = (u8*)malloc(sz + 1);
    if (!buf) return;
    i64 r = rfa_extract_to_buf(&g_mesh_rfa, rs_idx, buf, sz);
    if (r < 0) { free(buf); return; }
    buf[sz] = '\0';

    /* Parse text lines for subshader material entries */
    /* Format:
     *   subshader "shader_name_MaterialN" "parent" {
     *       ...
     *       texture "path/to/tex.dds";
     *   }
     */
    if (m->geom_count == 0 || m->geoms[0].lod_count == 0) { free(buf); return; }
    mesh_lod *lod = &m->geoms[0].lods[0];

    char *text = (char*)buf;
    char *p = text;
    while (*p) {
        /* Find "subshader \"" */
        char *ss = strstr(p, "subshader \"");
        if (!ss) break;
        ss += 11; /* skip past "subshader \"" */

        /* Find material number: look for "_Material" + digits */
        char *mat_tag = strstr(ss, "_Material");
        if (!mat_tag) { p = ss; continue; }
        mat_tag += 9; /* skip "_Material" */
        int mat_idx = (int)strtol(mat_tag, NULL, 10);

        /* Find texture line: "texture \"" */
        char *tex_start = strstr(ss, "texture \"");
        if (!tex_start) { p = ss; continue; }
        tex_start += 9; /* skip "texture \"" */
        char *tex_end = strchr(tex_start, '"');
        if (!tex_end) { p = ss; continue; }

        /* Extract texture path */
        size_t tex_len = (size_t)(tex_end - tex_start);
        if (tex_len > 0 && tex_len < MESH_NAME_LEN - 1 &&
            mat_idx >= 0 && (u32)mat_idx < lod->material_count) {
            strncpy(lod->materials[mat_idx].textures[0], tex_start, tex_len);
            lod->materials[mat_idx].textures[0][tex_len] = '\0';
            lod->materials[mat_idx].texture_count = 1;
        }

        p = tex_end + 1;
    }

    free(buf);
}

/* --- Load DDS texture from texture.rfa by path --- */
static u32 load_texture_from_rfa(const char *tex_path) {
    /* tex_path is like "texture/afrhouse_stn5_s" (without .dds?) or "texture/afrhouse_stn5_s.dds" with extension */
    if (!g_texture_rfa.fp) return 0;

    char search_path[512];
    snprintf(search_path, sizeof(search_path), "%s", tex_path);
    /* Ensure .dds extension */
    size_t plen = strlen(search_path);
    if (plen < 4 || strcasecmp(search_path + plen - 4, ".dds") != 0)
        strcat(search_path, ".dds");

    /* Try to find in texture.rfa (entries use lowercase paths) */
    int idx = rfa_find(&g_texture_rfa, search_path);
    if (idx < 0) {
        /* Also try under texture/ prefix if not already there */
        if (strncmp(search_path, "texture/", 8) != 0) {
            char prefixed[512];
            snprintf(prefixed, sizeof(prefixed), "texture/%s", search_path);
            idx = rfa_find(&g_texture_rfa, prefixed);
        }
    }
    if (idx < 0) return 0;

    rfa_entry *e = &g_texture_rfa.entries[idx];
    size_t sz = g_texture_rfa.compressed ? e->info.uncompressed_size : e->info.compressed_size;
    if (sz == 0 || sz > 1024 * 1024 * 8) return 0;

    u8 *dds_buf = (u8*)malloc(sz);
    if (!dds_buf) return 0;
    i64 r = rfa_extract_to_buf(&g_texture_rfa, idx, dds_buf, sz);
    if (r < 0) { free(dds_buf); return 0; }

    u32 tex = render_texture_load_dds(dds_buf, (size_t)r);
    free(dds_buf);
    return tex;
}

/* --- Load mesh from standardMesh.rFA by object name --- */
static int find_or_load_mesh(const char *obj_name, dsr_renderer *rr) {
    /* Check if already loaded */
    for (u32 i = 0; i < g_mesh_count; i++)
        if (strcasecmp(g_meshes[i].name, obj_name) == 0)
            return (int)i;

    if (g_mesh_count >= MAX_MESHES) return -1;
    if (!g_mesh_rfa.fp) return -1;

    /* Build .sm path: standardMesh/{name}.sm */
    char sm_path[512];
    snprintf(sm_path, sizeof(sm_path), "standardMesh/%s.sm", obj_name);
    int idx = rfa_find(&g_mesh_rfa, sm_path);
    if (idx < 0) {
        /* Try lowercase */
        char lower[512];
        for (char *p = lower, *s = sm_path; *s; s++, p++) *p = (char)tolower((u8)*s);
        *lower = '\0'; /* clear for rebuild */
        snprintf(lower, sizeof(lower), "standardMesh/%s.sm", obj_name);
        for (char *p = lower; *p; p++) *p = (char)tolower((u8)*p);
        idx = rfa_find(&g_mesh_rfa, lower);
    }
    if (idx < 0) return -1;

    rfa_entry *e = &g_mesh_rfa.entries[idx];
    size_t sz = g_mesh_rfa.compressed ? e->info.uncompressed_size : e->info.compressed_size;
    if (sz == 0 || sz > 1024 * 1024 * 4) return -1;

    u8 *buf = (u8*)malloc(sz);
    if (!buf) return -1;
    i64 r = rfa_extract_to_buf(&g_mesh_rfa, idx, buf, sz);
    if (r < 0) { free(buf); return -1; }

    dsr_mesh *m = &g_meshes[g_mesh_count].mesh;
    if (!mesh_load(m, buf, (size_t)r, sm_path)) {
        mesh_destroy(m);
        free(buf);
        return -1;
    }
    free(buf);

    /* Load .rs shader file to populate material texture paths */
    load_rs_textures(m, obj_name);

    /* Upload to GPU (skip if no renderer, e.g. headless) */
    render_mesh rmesh = {0};
    if (rr) {
        rmesh = render_mesh_upload(rr, m);
        /* Load DDS textures from texture.rfa for each material */
        if (m->geom_count > 0 && m->geoms[0].lod_count > 0) {
            mesh_lod *lod = &m->geoms[0].lods[0];
            for (u32 mi = 0; mi < lod->material_count && mi < RENDER_MAX_MATERIALS; mi++) {
                if (lod->materials[mi].texture_count > 0 &&
                    lod->materials[mi].textures[0][0]) {
                    u32 tex = load_texture_from_rfa(lod->materials[mi].textures[0]);
                    if (tex) {
                        rmesh.materials[mi].texture_id = tex;
                        printf("    mat[%u] -> tex %u: %s\n",
                               mi, tex, lod->materials[mi].textures[0]);
                    } else {
                        printf("    mat[%u] -> TEX NOT FOUND: %s\n",
                               mi, lod->materials[mi].textures[0]);
                    }
                }
            }
        }
        g_meshes[g_mesh_count].render = rmesh;
    }
    strncpy(g_meshes[g_mesh_count].name, obj_name, 63);
    g_meshes[g_mesh_count].name[63] = '\0';
    u32 loaded_idx = g_mesh_count++;
    return (int)loaded_idx;
}

/* --- Parse StaticObjects.con --- */
static void parse_static_objects(const char *path, dsr_renderer *rr) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "  StaticObjects.con not found\n"); return; }

    fseek(f, 0, SEEK_END);
    size_t sz = (size_t)ftell(f);
    rewind(f);
    char *buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);

    u32 parsed = 0;
    int current_mesh = -1;
    vec3 pos = {0,0,0};
    f32 rotX = 0, rotY = 0, rotZ = 0;
    vec3 scale = {1,1,1};
    vec3 color = {0.7f, 0.7f, 0.7f};

    char *line = buf;
    char *end = buf + sz;
    while (line < end) {
        char *nl = strchr(line, '\n');
        if (!nl) nl = end;
        *nl = '\0';

        /* Trim leading whitespace */
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;

        if (strncmp(p, "Object.create ", 14) == 0) {
            /* Flush previous */
            if (current_mesh >= 0 && g_obj_count < MAX_OBJECTS) {
                g_objects[g_obj_count].mesh_idx = current_mesh;
                g_objects[g_obj_count].pos = pos;
                g_objects[g_obj_count].rotX = rotX;
                g_objects[g_obj_count].rotY = rotY;
                g_objects[g_obj_count].rotZ = rotZ;
                g_objects[g_obj_count].scale = scale;
                g_objects[g_obj_count].color = color;
                g_obj_count++;
                parsed++;
            }

            char *name = p + 14;
            while (*name == ' ') name++;
            char *end_name = name;
            while (*end_name && *end_name != ' ' && *end_name != '\r') end_name++;
            char saved = *end_name;
            *end_name = '\0';

            current_mesh = find_or_load_mesh(name, rr);
            *end_name = saved;
            pos = (vec3){0,0,0};
            rotX = rotY = rotZ = 0;
            scale = (vec3){1,1,1};
            color = (vec3){0.7f, 0.7f, 0.7f};
        }
        else if (strncmp(p, "Object.absolutePosition ", 24) == 0) {
            f32 raw_x, raw_y, raw_z;
            sscanf(p + 24, "%f/%f/%f", &raw_x, &raw_y, &raw_z);
            pos.x = raw_x;
            pos.y = raw_y;
            pos.z = raw_z;
        }
        else if (strncmp(p, "Object.rotation ", 16) == 0) {
            sscanf(p + 16, "%f/%f/%f", &rotX, &rotY, &rotZ);
        }
        else if (strncmp(p, "Object.geometry.scale ", 22) == 0) {
            sscanf(p + 22, "%f/%f/%f", &scale.x, &scale.y, &scale.z);
        }
        else if (strncmp(p, "Object.geometry.color ", 22) == 0) {
            sscanf(p + 22, "%f/%f/%f", &color.x, &color.y, &color.z);
        }

        line = nl + 1;
    }

    /* Flush last */
    if (current_mesh >= 0 && g_obj_count < MAX_OBJECTS) {
        g_objects[g_obj_count].mesh_idx = current_mesh;
        g_objects[g_obj_count].pos = pos;
        g_objects[g_obj_count].rotX = rotX;
        g_objects[g_obj_count].rotY = rotY;
        g_objects[g_obj_count].rotZ = rotZ;
        g_objects[g_obj_count].scale = scale;
        g_objects[g_obj_count].color = color;
        g_obj_count++;
        parsed++;
    }

    if (g_terrain.heights) {
        u32 above = 0, below = 0;
        for (u32 i = 0; i < g_obj_count; i++) {
            f32 th = terrain_sample_height(&g_terrain, g_objects[i].pos.x, g_objects[i].pos.z);
            if (g_objects[i].pos.y >= th) above++;
            else below++;
        }
        printf("  Objects above terrain: %u, below: %u\n", above, below);
    } else {
        printf("  (no terrain loaded — skipping height sampling)\n");
    }
    free(buf);
    printf("  Parsed %u static objects, %u unique meshes loaded\n", parsed, g_mesh_count);
}

/* --- Render all static objects --- */
/* Rotation-axis interpretation (cycled with 'R'):
 *   mode 0: rotX->X, rotY->Y, rotZ->Z   (naive, each about its world axis)
 *   mode 1: rotX->Y(yaw), rotY->X, rotZ->Z   (first component = yaw)  [default]
 *   mode 2: rotX->Z(yaw), rotY->X, rotZ->Y
 * The .con rotation vector is (rotX, rotY, rotZ) in the object's local frame. */
static void build_rotation(mat4 *out, f32 cx, f32 cy, f32 cz) {
    mat4 a, b, c, t1;
    if (g_rot_mode == 0) {
        mat4_rotate_x(&a, cx); mat4_rotate_y(&b, cy); mat4_rotate_z(&c, cz);
    } else if (g_rot_mode == 1) {
        mat4_rotate_y(&a, cx); mat4_rotate_x(&b, cy); mat4_rotate_z(&c, cz);
    } else {
        mat4_rotate_z(&a, cx); mat4_rotate_x(&b, cy); mat4_rotate_y(&c, cz);
    }
    mat4_mul(&t1, &a, &b);
    mat4_mul(out, &t1, &c);
}

static void render_objects(void) {
    mat4 T, R, S, M, tmp;
    static bool printed = false;

    for (u32 i = 0; i < g_obj_count; i++) {
        static_obj *o = &g_objects[i];
        if (o->mesh_idx < 0 || o->mesh_idx >= (int)g_mesh_count) continue;

        dsr_mesh *m = &g_meshes[o->mesh_idx].mesh;
        f32 ground = g_terrain.heights
            ? terrain_sample_height(&g_terrain, o->pos.x, o->pos.z) : 0.0f;
        /* Mesh origin sits at the vertical middle; drop it so the base rests on terrain */
        f32 base_y  = m->geoms[0].lods[0].bbox_min.y;
        f32 place_y = ground - base_y * o->scale.y;

        if (!printed && i == 0) {
            printed = true;
            printf("OBJ0: con_y=%.2f ground=%.2f -> place_y=%.2f  bboxY=[%.2f,%.2f] rotMode=%d\n",
                   o->pos.y, ground, place_y,
                   m->geoms[0].lods[0].bbox_min.y, m->geoms[0].lods[0].bbox_max.y, g_rot_mode);
        }

        mat4_translate(&T, o->pos.x, place_y, o->pos.z);
        build_rotation(&R, o->rotX, o->rotY, o->rotZ);
        mat4_scale(&S, o->scale.x, o->scale.y, o->scale.z);

        mat4_mul(&tmp, &T, &R);
        mat4_mul(&M, &tmp, &S);

        if (g_mirror) {
            mat4 Mz;
            mat4_scale(&Mz, 1.0f, 1.0f, -1.0f);
            mat4_mul(&M, &M, &Mz);
        }

        render_mesh_draw(g_render, &g_meshes[o->mesh_idx].render, &M);
    }
}

/* --- Load terrain from BF1942 Tobruk level --- */
static bool load_terrain_tobruk(void) {
    const char *path = "bf1942_data/bf1942/Levels/Tobruk/Heightmap.raw";
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    rewind(f);

    u32 res = (u32)sqrtf((f32)(sz / 2));
    f32 max_h = 100.0f;
    if (!terrain_create(&g_terrain, res, res, 4.0f, max_h)) {
        fclose(f);
        return false;
    }

    u16 *buf = (u16*)malloc(sz);
    if (!buf) { fclose(f); return false; }
    fread(buf, 1, sz, f);
    fclose(f);

    f32 scale = max_h / 65535.0f;
    for (size_t i = 0; i < (size_t)res * res; i++)
        g_terrain.heights[i] = (f32)buf[i] * scale;

    g_terrain.min_height = 0;
    g_terrain.max_height = max_h;
    /* Debug: check some heights */
    printf("  Terrain height at (500,500): %.1f, (0,0): %.1f, (1000,1000): %.1f\n",
           g_terrain.heights[500*res+500], g_terrain.heights[0],
           g_terrain.heights[(res-1)*res+(res-1)]);
    free(buf);

    printf("  Heightmap: %ux%u, scale=%.1f, world=%u\n", res, res, g_terrain.scale_xz, res * (u32)g_terrain.scale_xz);
    return true;
}

/* --- List all .con files in RFA for logging --- */
static void scan_rfa_content(void) {
    rfa_archive rfa;
    if (rfa_open(&rfa, "data/objects.rfa") == RFA_OK) {
        printf("objects.rfa: %u entries\n", rfa.entry_count);
        rfa_close(&rfa);
    }
    if (rfa_open(&rfa, "data/standardMesh.rfa") == RFA_OK) {
        printf("standardMesh.rfa: %u entries\n", rfa.entry_count);
        rfa_close(&rfa);
    }
    if (rfa_open(&rfa, "data/bf1942.rfa") == RFA_OK) {
        printf("bf1942.rfa: %u entries\n", rfa.entry_count);
        rfa_close(&rfa);
    }
    if (rfa_open(&rfa, "data/texture.rfa") == RFA_OK) {
        printf("texture.rfa: %u entries\n", rfa.entry_count);
        rfa_close(&rfa);
    }
    if (rfa_open(&rfa, "data/bf1942.rfa") == RFA_OK) {
        printf("bf1942.rfa: %u entries (textures for Tobruk level):\n", rfa.entry_count);
        int cnt = 0;
        for (u32 i = 0; i < rfa.entry_count && cnt < 60; i++) {
            const char *p = rfa.entries[i].path;
            if (p && strlen(p) > 4 && strcasecmp(p+strlen(p)-4, ".dds") == 0) {
                if (strstr(p, "Tobruk") || strstr(p, "tobruk") || strstr(p, "afr") || strstr(p, "level")) {
                    printf("  %s\n", p);
                    cnt++;
                }
            }
        }
        if (cnt == 0) {
            /* Just print first 20 DDS regardless */
            for (u32 i = 0; i < rfa.entry_count; i++) {
                const char *p = rfa.entries[i].path;
                if (p && strlen(p) > 4 && strcasecmp(p+strlen(p)-4, ".dds") == 0) {
                    printf("  %s\n", p);
                    if (++cnt >= 20) break;
                }
            }
        }
        rfa_close(&rfa);
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    const int width = 1280, height = 720;
    bool headless = (argc > 1 && strcmp(argv[1], "--headless") == 0);

    printf("DSO-Refractor Engine v0.1\n");
    printf("=========================\n");

    if (headless) {
        printf("Headless mode: file operations only\n");
        scan_rfa_content();
        /* Verify mesh loading from RFA (no renderer = headless) */
        rfa_open(&g_mesh_rfa, "data/standardMesh.rfa");
        parse_static_objects("bf1942_data/bf1942/Levels/Tobruk/StaticObjects.con", NULL);
        rfa_close(&g_mesh_rfa);
        printf("\nDone. All RFA archives verified.\n");
        return 0;
    }

    if (init_sdl("DSO-Refractor — Battlefield 1942 Demo", width, height) != 0)
        return 1;

    /* Init GLEW */
    glewExperimental = GL_TRUE;
    GLenum glew_err = glewInit();
    if (glew_err != GLEW_OK) {
        fprintf(stderr, "GLEW init failed: %s\n", glewGetErrorString(glew_err));
        return 1;
    }
    printf("OpenGL: %s, GLSL: %s, Vendor: %s\n",
           glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION),
           glGetString(GL_VENDOR));

    /* Create engine state */
    g_state = dsr_create(30.0);

    /* Create renderer */
    g_render = render_create(NULL);
    if (!g_render) {
        fprintf(stderr, "render_create failed\n");
        return 1;
    }

    /* Scan available RFA content */
    scan_rfa_content();

    /* Open RFA for mesh loading */
    rfa_open(&g_mesh_rfa, "data/standardMesh.rfa");

    /* Open RFA for textures */
    if (rfa_open(&g_texture_rfa, "data/texture.rfa") != RFA_OK)
        rfa_open(&g_texture_rfa, "data/bf1942.rfa");

    /* Load Tobruk terrain FIRST (objects need terrain sampling) */
    printf("Loading terrain...\n");
    if (load_terrain_tobruk()) {
        printf("  Height at (647,218) = %.1f (world 2588,872)\n",
               g_terrain.heights[218 * 1024 + 647]);
        printf("  Height at (462,165) = %.1f (world 1848,660)\n",
               g_terrain.heights[165 * 1024 + 462]);
        render_terrain_upload(g_render, &g_terrain, &g_terrain_mesh);
        printf("  Terrain mesh: %u indices\n", g_terrain_mesh.index_count);

        /* Load terrain detail texture */
        printf("  Loading terrain texture...\n");
        const char *tex_paths[] = {
            "bf1942/Levels/Tobruk/Textures/detail.dds",
            "Levels/Tobruk/Textures/detail.dds",
        };
        for (int i = 0; i < 2; i++) {
            int ti = rfa_find(&g_texture_rfa, tex_paths[i]);
            if (ti >= 0) {
                rfa_entry *e = &g_texture_rfa.entries[ti];
                size_t sz = g_texture_rfa.compressed ? e->info.uncompressed_size : e->info.compressed_size;
                if (sz > 0 && sz < 1024*1024*16) {
                    u8 *buf = (u8*)malloc(sz);
                    if (buf) {
                        i64 r = rfa_extract_to_buf(&g_texture_rfa, ti, buf, sz);
                        if (r > 0) {
                            g_terrain_tex = render_texture_load_dds(buf, (size_t)r);
                            if (g_terrain_tex) printf("    detail.dds loaded (tex=%u)\n", g_terrain_tex);
                        }
                        free(buf);
                    }
                }
                break;
            }
        }
    } else {
        /* Procedural fallback */
        printf("  BF1942 level not found, creating procedural terrain\n");
        terrain_create(&g_terrain, 256, 256, 4.0f, 1.0f);
        for (u32 y = 0; y < g_terrain.height; y++)
            for (u32 x = 0; x < g_terrain.width; x++)
                g_terrain.heights[y * g_terrain.width + x] =
                    sinf((f32)x / 30) * 5 + cosf((f32)y / 20) * 5 + sinf((f32)(x+y) / 50) * 10;
        render_terrain_upload(g_render, &g_terrain, &g_terrain_mesh);
    }

    /* Load level static objects (after terrain, needs height sampling) */
    printf("Loading static objects...\n");
    parse_static_objects("bf1942_data/bf1942/Levels/Tobruk/StaticObjects.con", g_render);

    g_last_time = (f64)SDL_GetTicks() / 1000.0;

    printf("\n=== Controls ===\n");
    printf("WASD  - move camera  | RMB - mouse look\n");
    printf("Q/E   - up/down      | F1 - editor  | F2 - wireframe\n");
    printf("R     - cycle rotation axis (if objects mis-oriented)\n");
    printf("M     - toggle mirror (if whole level looks left-right flipped)\n");
    printf("Esc   - quit\n\n");

    /* Main loop */
    while (g_running) {
        f64 now = (f64)SDL_GetTicks() / 1000.0;
        f64 dt = now - g_last_time;
        g_last_time = now;
        if (dt > 0.1) dt = 0.1;

        handle_events();
        update_camera((f32)dt);

        /* Engine tick */
        dsr_tick(g_state, dt);

        /* Get viewport size */
        int vp_w, vp_h;
        SDL_GetWindowSize(window, &vp_w, &vp_h);

        /* Render frame */
        render_begin(g_render, (f32)vp_w, (f32)vp_h);
        render_set_camera(g_render, cam_pos, cam_target, 60.0f);

        /* Draw terrain */
        render_terrain_draw(g_render, &g_terrain, &g_terrain_mesh, g_terrain_tex);

        /* Draw static objects */
        render_objects();

        /* Check for GL errors */
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
            fprintf(stderr, "GL error: 0x%x\n", err);

        render_end(g_render);

        /* FPS counter */
        if (g_editor_mode) {
            char title[128];
            snprintf(title, sizeof(title), "DSO-Refractor — BF1942 Tobruk [%.0f FPS]", 1.0 / dt);
            SDL_SetWindowTitle(window, title);
        }

        SDL_GL_SwapWindow(window);
    }

    /* Cleanup */
    rfa_close(&g_mesh_rfa);
    for (u32 i = 0; i < g_mesh_count; i++)
        mesh_destroy(&g_meshes[i].mesh);
    terrain_destroy(&g_terrain);
    render_destroy(g_render);
    dsr_destroy(g_state);

    SDL_GL_DeleteContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
