#include "mesh.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>

/* --- Mini endian-safe reader --- */

typedef struct {
    const u8 *data;
    size_t    size;
    size_t    pos;
} mem_reader;

static bool mr_init(mem_reader *r, const u8 *data, size_t size) {
    r->data = data;
    r->size = size;
    r->pos  = 0;
    return true;
}

static u32 mr_u32(mem_reader *r) {
    if (r->pos + 4 > r->size) return 0;
    u32 v = (u32)r->data[r->pos] |
            ((u32)r->data[r->pos+1] << 8) |
            ((u32)r->data[r->pos+2] << 16) |
            ((u32)r->data[r->pos+3] << 24);
    r->pos += 4;
    return v;
}

static u16 mr_u16(mem_reader *r) {
    if (r->pos + 2 > r->size) return 0;
    u16 v = (u16)r->data[r->pos] |
            ((u16)r->data[r->pos+1] << 8);
    r->pos += 2;
    return v;
}

static u8 mr_u8(mem_reader *r) {
    if (r->pos + 1 > r->size) return 0;
    return r->data[r->pos++];
}

static void mr_skip(mem_reader *r, size_t n) {
    r->pos += n;
    if (r->pos > r->size) r->pos = r->size;
}

static void mr_read(mem_reader *r, void *dst, size_t n) {
    if (r->pos + n > r->size) n = r->size - r->pos;
    memcpy(dst, &r->data[r->pos], n);
    r->pos += n;
}

static f32 mr_f32(mem_reader *r) {
    if (r->pos + 4 > r->size) return 0;
    f32 v;
    memcpy(&v, &r->data[r->pos], 4);
    r->pos += 4;
    return v;
}

static char *mr_string(mem_reader *r) {
    u32 len = mr_u32(r);
    if (len == 0 || len > 4096 || r->pos + len > r->size) {
        char *empty = (char*)calloc(1, 1);
        return empty;
    }
    char *s = (char*)calloc(len + 1, 1);
    memcpy(s, &r->data[r->pos], len);
    r->pos += len;
    return s;
}

/* --- .sm (BF1942 StandardMesh v2/v3/v9/v10) --- */
/* Based on BfMeshView (modStdMesh.bas) by art567 */

/* Helper: read length-prefixed string into arena, returns pointer or NULL */
static char *sm_str(arena *a, mem_reader *r) {
    u32 len = mr_u32(r);
    if (len == 0 || len > 4096 || r->pos + len > r->size) return NULL;
    char *s = (char*)arena_alloc(a, len + 1, 1);
    if (!s) return NULL;
    memcpy(s, &r->data[r->pos], len);
    s[len] = '\0';
    r->pos += len;
    return s;
}

/* Parse v9/v10 format: header + collision + per-material geometry */
static bool load_sm_v9(dsr_mesh *m, mem_reader *r) {
    /* Skip collision meshes.
     * Format (from BfMeshView modStdMesh.bas):
     *   colnum u32
     *   per collision mesh:
     *     size  u32  - total remaining bytes in this block
     *     u1    u32
     *     u2    u32  (always 5)
     *     vertnum u32
     *     vert[vertnum]  16 bytes each (colvert = vec3 + w)
     *     facenum u32
     *     face[facenum]  8 bytes each (colface = 3x i16 + matid + flags)
     *     ... more data, skipped via size field
     */
    u32 colnum = mr_u32(r);
    for (u32 ci = 0; ci < colnum; ci++) {
        u32 size = mr_u32(r);
        mr_skip(r, size);
    }

    /* LOD count (also acts as file type: 1=single, 6=multi) */
    u32 lodnum = mr_u32(r);
    if (lodnum == 0 || lodnum > MESH_MAX_LODS) return false;

    m->geom_count = 1;
    mesh_geom *g = &m->geoms[0];
    g->lod_count = lodnum;

    /* Read each LOD: headers then geometry per material (interleaved) */
    for (u32 li = 0; li < lodnum; li++) {
        u32 matnum = mr_u32(r);
        if (matnum > 64) return false;

        mesh_lod *lod = &g->lods[li];
        lod->material_count = matnum;
        size_t ms = matnum * sizeof(mesh_material);
        lod->materials = (mesh_material*)arena_alloc(&m->mesh_arena, ms, 4);
        if (!lod->materials) return false;
        memset(lod->materials, 0, ms);

        /* Per-material stride/sizes collected during header pass */
        typedef struct { u32 stride, vn, in; } mat_info;
        mat_info info[64];

        /* First: read material headers */
        for (u32 mi = 0; mi < matnum; mi++) {
            char *name = sm_str(&m->mesh_arena, r);
            if (name) strncpy(lod->materials[mi].shader, name, MESH_NAME_LEN - 1);
            mr_u32(r); mr_u32(r); mr_u32(r); /* u2 u3 u4 (0) */
            u32 prim = mr_u32(r);
            mr_u32(r);                       /* u6 */
            u32 vs = mr_u32(r);
            u32 vn = mr_u32(r);
            u32 in = mr_u32(r);
            mr_u32(r);                       /* u7 */

            info[mi].stride = vs;
            info[mi].vn     = vn;
            info[mi].in     = in;
            lod->materials[mi].vertex_count  = vn;
            lod->materials[mi].index_count   = in;
            lod->materials[mi].texture_count = 0;
            (void)prim;
        }

        /* Second: read geometry data for this LOD's materials.
         *
         * v9 .sm stores geometry PER MATERIAL (a vertex block immediately
         * followed by an index block), interleaved across all materials of the
         * LOD. We merge every material into ONE vertex/index buffer so no
         * geometry is dropped. All materials in a LOD share the same stride for
         * our BF1942 data; if they don't, fall back to loading only material 0.
         */
        {
            u32 base_stride = info[0].stride;
            u32 total_vn = 0, total_in = 0;
            bool uniform = true;
            for (u32 mi = 0; mi < matnum; mi++) {
                if (info[mi].stride != base_stride) { uniform = false; break; }
                if (info[mi].vn == 0 || info[mi].vn > 100000 ||
                    info[mi].in == 0 || info[mi].in > 300000) {
                    uniform = false; break;
                }
                total_vn += info[mi].vn;
                total_in += info[mi].in;
            }

            if (uniform && total_vn > 0 && total_in > 0) {
                u32 fs = base_stride / 4;
                lod->vertex_count  = total_vn;
                lod->vertex_stride = fs;
                lod->index_count   = total_in;

                size_t vb_sz = (size_t)total_vn * base_stride;
                lod->vertices = (vec3*)arena_alloc(&m->mesh_arena, vb_sz, 4);
                size_t ib_sz = (size_t)total_in * sizeof(u16);
                lod->indices = (u16*)arena_alloc(&m->mesh_arena, ib_sz, 2);
                if (!lod->vertices || !lod->indices) return false;

                /* Convert Z-up authored vertex data to the engine/world frame.
                 * The terrain and .con positions use world = (x, z, y), i.e. a
                 * Z-up -> Y-up swap (Z is up in the authored mesh). This keeps
                 * meshes consistent with the ground plane and object positions.
                 * A building whose long axis is Z therefore stands upright
                 * along world Y. */
                f32 bmin[3] = { 1e9f, 1e9f, 1e9f };
                f32 bmax[3] = { -1e9f, -1e9f, -1e9f };
                u32 vbase = 0, ibase = 0;
                for (u32 mi = 0; mi < matnum; mi++) {
                    u32 vn = info[mi].vn;
                    u32 in = info[mi].in;
                    f32 *vdst = (f32*)lod->vertices + (size_t)vbase * fs;
                    mr_read(r, vdst, (size_t)vn * base_stride);

                    for (u32 vi = 0; vi < vn; vi++) {
                        f32 *v = vdst + vi * fs;
                        f32 x = v[0], y = v[1], z = v[2];
                        v[0] = x;  v[1] = z;  v[2] = y;
                        f32 nx = v[6], ny = v[7], nz = v[8];
                        v[6] = nx; v[7] = nz; v[8] = ny;
                        for (int a = 0; a < 3; a++) {
                            if (v[a] < bmin[a]) bmin[a] = v[a];
                            if (v[a] > bmax[a]) bmax[a] = v[a];
                        }
                    }

                    for (u32 ii = 0; ii < in; ii++)
                        lod->indices[ibase + ii] = (u16)(mr_u16(r) + vbase);

                    lod->materials[mi].vertex_offset = vbase;
                    lod->materials[mi].vertex_count  = vn;
                    lod->materials[mi].index_offset  = ibase;
                    lod->materials[mi].index_count   = in;

                    vbase += vn;
                    ibase += in;
                }

                lod->bbox_min = (vec3){ bmin[0], bmin[1], bmin[2] };
                lod->bbox_max = (vec3){ bmax[0], bmax[1], bmax[2] };
            } else {
                /* Fallback: load only material 0 (legacy behaviour). */
                for (u32 mi = 0; mi < matnum; mi++) {
                    u32 stride = info[mi].stride;
                    u32 vn = info[mi].vn;
                    u32 in = info[mi].in;
                    if (vn == 0 || vn > 100000 || in > 300000 ||
                        stride == 0 || stride > 128) {
                        mr_skip(r, (size_t)vn * (stride ? stride : 32));
                        mr_skip(r, (size_t)in * 2);
                        continue;
                    }
                    if (mi == 0) {
                        u32 fs = stride / 4;
                        lod->vertex_count  = vn;
                        lod->vertex_stride = fs;
                        lod->index_count   = in;
                        size_t vb_sz = (size_t)vn * stride;
                        lod->vertices = (vec3*)arena_alloc(&m->mesh_arena, vb_sz, 4);
                        if (!lod->vertices) return false;
                        mr_read(r, lod->vertices, vb_sz);
                        f32 bmin[3] = { 1e9f, 1e9f, 1e9f };
                        f32 bmax[3] = { -1e9f, -1e9f, -1e9f };
                        for (u32 vi = 0; vi < vn; vi++) {
                            f32 *v = (f32*)lod->vertices + vi * fs;
                            f32 x = v[0], y = v[1], z = v[2];
                            v[0] = x;  v[1] = z;  v[2] = -y;
                            f32 nx = v[6], ny = v[7], nz = v[8];
                            v[6] = nx; v[7] = nz; v[8] = -ny;
                            for (int a = 0; a < 3; a++) {
                                if (v[a] < bmin[a]) bmin[a] = v[a];
                                if (v[a] > bmax[a]) bmax[a] = v[a];
                            }
                        }
                        lod->bbox_min = (vec3){ bmin[0], bmin[1], bmin[2] };
                        lod->bbox_max = (vec3){ bmax[0], bmax[1], bmax[2] };
                        size_t ib_sz = (size_t)in * sizeof(u16);
                        lod->indices = (u16*)arena_alloc(&m->mesh_arena, ib_sz, 2);
                        if (!lod->indices) return false;
                        for (u32 ii = 0; ii < in; ii++)
                            lod->indices[ii] = mr_u16(r);
                    } else {
                        mr_skip(r, (size_t)vn * stride);
                        mr_skip(r, (size_t)in * sizeof(u16));
                    }
                }
            }
        }
    }

    return true;
}

/* Parse v2/v3 format: flat header then vertex/index/material data */
static bool load_sm_v2(dsr_mesh *m, mem_reader *r) {
    u32 vert_count = mr_u32(r);
    u32 index_count = mr_u32(r);
    u32 mat_count = mr_u32(r);
    u32 flags = mr_u32(r);
    (void)flags;

    if (vert_count == 0 || vert_count > 100000) return false;
    if (index_count == 0 || index_count > 300000) return false;

    m->geom_count = 1;
    mesh_geom *g = &m->geoms[0];
    g->lod_count = 1;
    mesh_lod *lod = &g->lods[0];

    lod->vertex_stride = 8;
    lod->vertex_count = vert_count;

    size_t vs = (size_t)vert_count * 8 * sizeof(f32);
    lod->vertices = (vec3*)arena_alloc(&m->mesh_arena, vs, 4);
    if (!lod->vertices) return false;

    for (u32 i = 0; i < vert_count; i++) {
        f32 *v = (f32*)lod->vertices + i * 8;
        v[0]=mr_f32(r); v[1]=mr_f32(r); v[2]=mr_f32(r);
        v[3]=mr_f32(r); v[4]=mr_f32(r); v[5]=mr_f32(r);
        v[6]=mr_f32(r); v[7]=mr_f32(r);
    }
    /* DEBUG: print first vertex of v2 mesh */
    { static bool vp = false; if (!vp) { vp = true;
        printf("  v2 mesh: %u verts, first vertex=(%.2f, %.2f, %.2f) normal=(%.2f, %.2f, %.2f) uv=(%.2f, %.2f)\n",
               vert_count,
               ((f32*)lod->vertices)[0], ((f32*)lod->vertices)[1], ((f32*)lod->vertices)[2],
               ((f32*)lod->vertices)[3], ((f32*)lod->vertices)[4], ((f32*)lod->vertices)[5],
               ((f32*)lod->vertices)[6], ((f32*)lod->vertices)[7]);
    }}

    lod->index_count = index_count;
    size_t idx_sz = (size_t)index_count * sizeof(u16);
    lod->indices = (u16*)arena_alloc(&m->mesh_arena, idx_sz, 2);
    if (!lod->indices) return false;
    for (u32 i = 0; i < index_count; i++)
        lod->indices[i] = mr_u16(r);

    lod->material_count = mat_count;
    size_t ms = mat_count * sizeof(mesh_material);
    lod->materials = (mesh_material*)arena_alloc(&m->mesh_arena, ms, 4);
    if (!lod->materials) return false;
    memset(lod->materials, 0, ms);

    for (u32 i = 0; i < mat_count; i++) {
        mesh_material *mat = &lod->materials[i];
        mat->vertex_offset = mr_u32(r);
        mat->vertex_count  = mr_u32(r);
        mat->index_offset  = mr_u32(r);
        mat->index_count   = mr_u32(r);
        mat->texture_count = mr_u32(r);
        for (u32 t = 0; t < mat->texture_count && t < MESH_MAX_TEX; t++) {
            char *tex = mr_string(r);
            if (tex) { strncpy(mat->textures[t], tex, MESH_NAME_LEN - 1); free(tex); }
        }
        char *shader = mr_string(r);
        if (shader) { strncpy(mat->shader, shader, MESH_NAME_LEN - 1); free(shader); }
    }
    return true;
}

/* --- Determine version and dispatch --- */

static bool load_sm(dsr_mesh *m, mem_reader *r) {
    u32 version = mr_u32(r);
    m->version = version;

    if (version == 2 || version == 3) {
        return load_sm_v2(m, r);
    }

    if (version >= 9 && version <= 10) {
        u32 u2 = mr_u32(r);
        (void)u2;
        mr_skip(r, 24); /* bbox */
        if (version > 9) mr_u8(r); /* qflag */
        return load_sm_v9(m, r);
    }

    return false;
}

/* ---------- .bundledmesh / .staticmesh / .skinnedmesh (BF2+) ---------- */

static bool is_pow2(u32 x) { return x && !(x & (x - 1)); }

static bool load_bundledmesh(dsr_mesh *m, mem_reader *r) {
    u32 u1      = mr_u32(r);
    u32 version = mr_u32(r);
    u32 u3      = mr_u32(r);
    u32 u4      = mr_u32(r);
    u32 u5      = mr_u32(r);
    (void)u1; (void)u3; (void)u4; (void)u5;

    m->version = version;
    u8 align = mr_u8(r);
    (void)align;

    m->geom_count = mr_u32(r);
    if (m->geom_count > MESH_MAX_GEOMS) m->geom_count = MESH_MAX_GEOMS;

    for (u32 gi = 0; gi < m->geom_count; gi++) {
        mesh_geom *g = &m->geoms[gi];
        g->lod_count = mr_u32(r);
        if (g->lod_count > MESH_MAX_LODS) g->lod_count = MESH_MAX_LODS;
    }

    u32 attrib_count = mr_u32(r);
    if (attrib_count > 32) return false;

    vertex_attrib attribs[32];
    for (u32 i = 0; i < attrib_count; i++) {
        attribs[i].flags   = mr_u16(r);
        attribs[i].offset  = mr_u16(r);
        attribs[i].vartype = mr_u16(r);
        attribs[i].usage   = mr_u16(r);
    }

    u32 vert_format = mr_u32(r);
    u32 vert_stride = mr_u32(r);
    u32 vert_count  = mr_u32(r);
    (void)vert_format;

    if (vert_count == 0 || vert_count > 100000) return false;
    if (vert_stride == 0 || vert_stride > 128) return false;

    size_t vert_bytes = (size_t)vert_count * vert_stride;
    if (r->pos + vert_bytes > r->size) return false;

    if (m->geom_count > 0 && m->geoms[0].lod_count > 0) {
        mesh_geom *g = &m->geoms[0];
        mesh_lod *lod = &g->lods[0];
        lod->vertex_count = vert_count;
        lod->vertex_stride = vert_stride;
        lod->vertices = (vec3*)arena_alloc(&m->mesh_arena, vert_bytes, 4);
        if (!lod->vertices) return false;
        mr_read(r, lod->vertices, vert_bytes);

        lod->attrib_count = attrib_count;
        lod->attribs = (vertex_attrib*)arena_alloc(&m->mesh_arena, attrib_count * sizeof(vertex_attrib), 4);
        if (lod->attribs) memcpy(lod->attribs, attribs, attrib_count * sizeof(vertex_attrib));
        /* DEBUG: print first mesh attribs */
        static bool printed = false;
        if (!printed) { printed = true;
            printf("  Bundled mesh: stride=%u, %u attribs\n", vert_stride, attrib_count);
            for (u32 ai = 0; ai < attrib_count; ai++)
                printf("    attrib[%u]: usage=%u offset=%u flags=%u vartype=%u\n",
                       ai, attribs[ai].usage, attribs[ai].offset, attribs[ai].flags, attribs[ai].vartype);
        }
    } else {
        mr_skip(r, vert_bytes);
    }

    u32 index_count = mr_u32(r);
    if (index_count > 300000) return false;

    if (m->geom_count > 0 && m->geoms[0].lod_count > 0) {
        mesh_geom *g = &m->geoms[0];
        mesh_lod *lod = &g->lods[0];
        lod->index_count = index_count;
        size_t ib = (size_t)index_count * sizeof(u16);
        lod->indices = (u16*)arena_alloc(&m->mesh_arena, ib, 2);
        if (!lod->indices) return false;
        for (u32 i = 0; i < index_count; i++)
            lod->indices[i] = mr_u16(r);
    }

    return true;
}

/* ---------- Public API ---------- */

bool mesh_load(dsr_mesh *m, const u8 *data, size_t data_size, const char *filename_hint) {
    memset(m, 0, sizeof(*m));
    m->mesh_arena = arena_create(data_size * 2 + 1024 * 1024);

    mem_reader r;
    mr_init(&r, data, data_size);

    if (filename_hint) {
        const char *ext = strrchr(filename_hint, '.');
        if (ext) {
            if (strcasecmp(ext, ".sm") == 0 || strcasecmp(ext, ".tm") == 0) {
                m->format = MESH_SM;
                return load_sm(m, &r);
            }
            if (strcasecmp(ext, ".bundledmesh") == 0) {
                m->format = MESH_BUNDLEDMESH;
                return load_bundledmesh(m, &r);
            }
            if (strcasecmp(ext, ".staticmesh") == 0) {
                m->format = MESH_STATICMESH;
                return load_bundledmesh(m, &r);
            }
            if (strcasecmp(ext, ".skinnedmesh") == 0) {
                m->format = MESH_SKINNEDMESH;
                return load_bundledmesh(m, &r);
            }
        }
    }

    if (load_bundledmesh(m, &r)) return true;

    mr_init(&r, data, data_size);
    m->format = MESH_SM;
    return load_sm(m, &r);
}

bool mesh_load_file(dsr_mesh *m, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    size_t sz = (size_t)ftell(f);
    rewind(f);
    u8 *buf = (u8*)malloc(sz);
    if (!buf) { fclose(f); return false; }
    fread(buf, 1, sz, f);
    fclose(f);
    bool ok = mesh_load(m, buf, sz, path);
    free(buf);
    return ok;
}

void mesh_destroy(dsr_mesh *m) {
    arena_destroy(&m->mesh_arena);
    memset(m, 0, sizeof(*m));
}

void mesh_dump(dsr_mesh *m) {
    printf("Mesh format=%d version=%u\n", m->format, m->version);
    printf("  geoms: %u\n", m->geom_count);
    for (u32 gi = 0; gi < m->geom_count; gi++) {
        mesh_geom *g = &m->geoms[gi];
        printf("  geom[%u] lods: %u\n", gi, g->lod_count);
        for (u32 li = 0; li < g->lod_count; li++) {
            mesh_lod *lod = &g->lods[li];
            printf("    lod[%u]: verts=%u, indices=%u, stride=%u, mats=%u\n",
                   li, lod->vertex_count, lod->index_count,
                   lod->vertex_stride, lod->material_count);
            for (u32 mi = 0; mi < lod->material_count; mi++) {
                mesh_material *mat = &lod->materials[mi];
                printf("      mat[%u]: shader=%s, indices=[%u..%u), tex=%u\n",
                       mi, mat->shader, mat->index_offset,
                       mat->index_offset + mat->index_count,
                       mat->texture_count);
            }
        }
    }
}
