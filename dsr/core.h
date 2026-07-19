#ifndef DSR_CORE_H
#define DSR_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* DSO-Refractor Core
 * Deterministic tick loop, arena allocators, hash-chain audit.
 * DSO philosophy: tick-awareness, separation of concerns, measure-first.
 */

/* --- Fixed-width types --- */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef float    f32;
typedef double   f64;

/* --- Vec3 / Vec4 --- */
typedef struct { f32 x, y, z; }     vec3;
typedef struct { f32 x, y, z, w; }  vec4;
typedef struct { f32 e[16]; }       mat4;

static inline vec3 vec3_make(f32 x, f32 y, f32 z) { vec3 v = {x,y,z}; return v; }
static inline vec4 vec4_make(f32 x, f32 y, f32 z, f32 w) { vec4 v = {x,y,z,w}; return v; }

/* --- Arena allocator (DSO verified pattern) --- */
typedef struct {
    char *base;
    size_t size;
    size_t offset;
    size_t prev_offset;
} arena;

arena arena_create(size_t size);
void  arena_destroy(arena *a);
void *arena_alloc(arena *a, size_t sz, size_t align);
void  arena_reset(arena *a);
#define arena_alloc_s(a, T)   (T*)arena_alloc(a, sizeof(T), _Alignof(T))
#define arena_alloc_n(a, T,n) (T*)arena_alloc(a, sizeof(T)*(n), _Alignof(T))

/* --- Tick-aligned object arena --- */
typedef struct {
    arena *current;    /* allocations this tick */
    arena *persistent; /* allocations that survive ticks */
    u64    tick_id;
} tick_arena;

tick_arena tick_arena_create(size_t tick_sz, size_t persist_sz);
void      tick_arena_destroy(tick_arena *ta);
void     *tick_alloc(tick_arena *ta, size_t sz, size_t align);
void      tick_end(tick_arena *ta);  /* reset tick arena, advance tick */
#define  tick_alloc_s(ta, T)    (T*)tick_alloc(ta, sizeof(T), _Alignof(T))
#define  tick_alloc_n(ta, T, n) (T*)tick_alloc(ta, sizeof(T)*(n), _Alignof(T))

/* --- Hash-chain (SINT-UA verified pattern) --- */
typedef struct {
    u64  prev_hash;
    u64  tick_id;
    u32  command_count;
    u8   reserved[4];
} hash_chain_block;

u64 hash_combine(u64 h, const void *data, size_t len);
u64 hash_chain_init(void);
u64 hash_chain_tick(u64 prev, u64 tick_id, u32 cmd_count);

/* --- Deterministic tick loop --- */
typedef struct {
    f64 tick_rate;        /* Hz (30 for BF1942 compat) */
    f64 dt;               /* seconds per tick */
    f64 accumulator;
    u64 tick_id;
    f64 time;
} tick_loop;

tick_loop tick_loop_make(f64 tick_rate);
void      tick_loop_reset(tick_loop *tl);
int       tick_loop_advance(tick_loop *tl, f64 frame_dt);
/* Returns: 0 = no tick, 1 = tick, -1 = catchup (ran multiple ticks) */

/* --- Object types (from Refractor .con templates) --- */
typedef enum {
    OBJ_NONE = 0,
    OBJ_SIMPLE,
    OBJ_BUNDLE,
    OBJ_ROTATIONAL_BUNDLE,
    OBJ_PLAYER_CONTROL,
    OBJ_SOLDIER,
    OBJ_VEHICLE,
    OBJ_WEAPON,
    OBJ_PROJECTILE,
    OBJ_EFFECT,
    OBJ_BUILDING,
    OBJ_FLAG,
    OBJ_AI,
} object_type;

typedef u32 entity_id;
#define ENTITY_NULL ((entity_id)0)

/* --- SoA entity layout (DSO: SoA over AoS) --- */
typedef struct {
    entity_id *ids;
    object_type *types;
    vec3 *positions;
    vec4 *rotations;
    vec3 *velocities;
    u32  *state_flags;
    u32   count;
    u32   capacity;
} entity_array;

entity_array entity_array_create(u32 capacity);
void         entity_array_destroy(entity_array *ea);
entity_id    entity_spawn(entity_array *ea, object_type type, vec3 pos);
void         entity_despawn(entity_array *ea, entity_id id);

/* --- Game state --- */
typedef struct {
    tick_loop    tick;
    tick_arena   arena;
    entity_array entities;
    u64          chain_hash;
    f32          time_scale;
    u32          flags;
} dsr_state;

dsr_state *dsr_create(f64 tick_rate);
void       dsr_destroy(dsr_state *s);
int        dsr_tick(dsr_state *s, f64 frame_dt);

#endif /* DSR_CORE_H */
