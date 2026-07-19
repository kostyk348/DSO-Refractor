#ifndef DSR_MESH_H
#define DSR_MESH_H

#include "core.h"

/* Refractor Engine mesh loader
 * Supports: .sm (StandardMesh, BF1942), .bundledmesh (BF2+), .staticmesh (BF2+)
 *
 * Based on:
 *   BfMeshView (bytehazard.com)
 *   BF2Mesh C# parser (UTurista)
 *   BattlefieldRespawn File_Headers.md
 */

#define MESH_MAX_LODS 8
#define MESH_MAX_GEOMS 8
#define MESH_MAX_TEX 8
#define MESH_MAX_BONES 128
#define MESH_NAME_LEN 256

typedef enum {
    MESH_SM,             /* .sm  — BF1942 StandardMesh */
    MESH_BUNDLEDMESH,    /* .bundledmesh — BF2+ */
    MESH_STATICMESH,     /* .staticmesh — BF2+ */
    MESH_SKINNEDMESH,    /* .skinnedmesh — BF2+ */
    MESH_UNKNOWN,
} mesh_format;

/* Vertex attribute types */
typedef enum {
    ATTR_FLOAT1 = 0,
    ATTR_FLOAT2 = 1,
    ATTR_FLOAT3 = 2,
    ATTR_FLOAT4 = 3,
    ATTR_UBYTE4 = 4,
    ATTR_SHORT2 = 5,
    ATTR_SHORT4 = 6,
    ATTR_UBYTE4N = 7,
    ATTR_SHORT2N = 8,
    ATTR_SHORT4N = 9,
    ATTR_USHORT2N = 10,
    ATTR_USHORT4N = 11,
    ATTR_UDEC3 = 12,
    ATTR_DEC3N = 13,
    ATTR_FLOAT16_2 = 14,
    ATTR_FLOAT16_4 = 15,
} attr_type;

/* Vertex attribute usage (D3DDECLUSAGE extended) */
typedef enum {
    USAGE_POSITION    = 0,
    USAGE_BLENDWEIGHT = 1,
    USAGE_BLENDINDICES = 2,
    USAGE_NORMAL      = 3,
    USAGE_TEXCOORD    = 5,
    USAGE_TANGENT     = 6,
    USAGE_BINORMAL    = 7,
    USAGE_COLOR       = 10,
} attr_usage;

typedef struct {
    u16 flags;
    u16 offset;
    u16 vartype;
    u16 usage;
} vertex_attrib;

typedef struct {
    i32  lod_index;
    u32  vertex_offset;
    u32  vertex_count;
    u32  index_offset;
    u32  index_count;
    vec3 bounds_min;
    vec3 bounds_max;
    /* Material/drawcall info */
    char shader[MESH_NAME_LEN];
    char technique[MESH_NAME_LEN];
    char textures[MESH_MAX_TEX][MESH_NAME_LEN];
    u32  texture_count;
} mesh_material;

typedef struct {
    u32 vertex_count;
    u32 index_count;
    vec3 *vertices;        /* interleaved, stride-based */
    u16  *indices;
    u32  vertex_stride;
    u32  attrib_count;
    vertex_attrib *attribs;
    u32  material_count;
    mesh_material *materials;
    vec3 bbox_min;         /* post-load model-space bounds (Y-up) */
    vec3 bbox_max;
} mesh_lod;

typedef struct {
    u32        lod_count;
    mesh_lod   lods[MESH_MAX_LODS];
} mesh_geom;

typedef struct {
    mesh_format format;
    u32         version;
    u32         geom_count;
    mesh_geom   geoms[MESH_MAX_GEOMS];
    /* For BF2+ format */
    mat4       *node_transforms;
    u32         node_count;
    /* For SkinnedMesh */
    u32         bone_count;
    u32         bone_ids[MESH_MAX_BONES];
    mat4        bone_transforms[MESH_MAX_BONES];
    /* Internal */
    arena       mesh_arena;
} dsr_mesh;

/* Load a mesh file from buffer */
bool mesh_load(dsr_mesh *m, const u8 *data, size_t data_size, const char *filename_hint);

/* Load a mesh file from path */
bool mesh_load_file(dsr_mesh *m, const char *path);

/* Destroy mesh */
void mesh_destroy(dsr_mesh *m);

/* Debug dump */
void mesh_dump(dsr_mesh *m);

#endif /* DSR_MESH_H */
