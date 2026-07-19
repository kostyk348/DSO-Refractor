#include "core.h"
#include <stdlib.h>
#include <string.h>

/* --- Arena allocator --- */

arena arena_create(size_t size) {
    arena a;
    a.base = (char*)malloc(size);
    a.size = size;
    a.offset = 0;
    a.prev_offset = 0;
    if (!a.base) { a.size = 0; }
    return a;
}

void arena_destroy(arena *a) {
    free(a->base);
    a->base = NULL;
    a->size = 0;
    a->offset = 0;
}

void *arena_alloc(arena *a, size_t sz, size_t align) {
    size_t mask = align - 1;
    size_t aligned = (a->offset + mask) & ~mask;
    if (aligned + sz > a->size) return NULL;
    a->prev_offset = a->offset;
    a->offset = aligned + sz;
    return a->base + aligned;
}

void arena_reset(arena *a) {
    a->offset = 0;
}

/* --- Tick arena --- */

tick_arena tick_arena_create(size_t tick_sz, size_t persist_sz) {
    tick_arena ta;
    ta.current    = (arena*)malloc(sizeof(arena));
    ta.persistent = (arena*)malloc(sizeof(arena));
    *ta.current    = arena_create(tick_sz);
    *ta.persistent = arena_create(persist_sz);
    ta.tick_id = 0;
    return ta;
}

void tick_arena_destroy(tick_arena *ta) {
    arena_destroy(ta->current);
    arena_destroy(ta->persistent);
    free(ta->current);
    free(ta->persistent);
}

void *tick_alloc(tick_arena *ta, size_t sz, size_t align) {
    return arena_alloc(ta->current, sz, align);
}

void tick_end(tick_arena *ta) {
    arena_reset(ta->current);
    ta->tick_id++;
}

/* --- Hash-chain --- */

u64 hash_combine(u64 h, const void *data, size_t len) {
    const u8 *p = (const u8*)data;
    /* FNV-1a 64-bit */
    for (size_t i = 0; i < len; i++) {
        h ^= (u64)p[i];
        h *= 0x100000001B3ULL;
    }
    return h;
}

u64 hash_chain_init(void) {
    return 0xCBF29CE484222325ULL; /* FNV offset basis */
}

u64 hash_chain_tick(u64 prev, u64 tick_id, u32 cmd_count) {
    hash_chain_block blk;
    blk.prev_hash    = prev;
    blk.tick_id      = tick_id;
    blk.command_count = cmd_count;
    memset(blk.reserved, 0, 4);
    return hash_combine(prev, &blk, sizeof(blk));
}

/* --- Tick loop --- */

tick_loop tick_loop_make(f64 tick_rate) {
    tick_loop tl;
    tl.tick_rate = tick_rate;
    tl.dt = 1.0 / tick_rate;
    tl.accumulator = 0.0;
    tl.tick_id = 0;
    tl.time = 0.0;
    return tl;
}

void tick_loop_reset(tick_loop *tl) {
    tl->accumulator = 0.0;
    tl->tick_id = 0;
    tl->time = 0.0;
}

int tick_loop_advance(tick_loop *tl, f64 frame_dt) {
    tl->accumulator += frame_dt;
    if (tl->accumulator < tl->dt) return 0;
    int count = 0;
    while (tl->accumulator >= tl->dt) {
        tl->accumulator -= tl->dt;
        tl->time += tl->dt;
        tl->tick_id++;
        count++;
    }
    return (count == 1) ? 1 : -1;
}

/* --- SoA entity array --- */

entity_array entity_array_create(u32 capacity) {
    entity_array ea;
    ea.count    = 0;
    ea.capacity = capacity;
    ea.ids      = (entity_id*)calloc(capacity, sizeof(entity_id));
    ea.types    = (object_type*)calloc(capacity, sizeof(object_type));
    ea.positions = (vec3*)calloc(capacity, sizeof(vec3));
    ea.rotations = (vec4*)calloc(capacity, sizeof(vec4));
    ea.velocities = (vec3*)calloc(capacity, sizeof(vec3));
    ea.state_flags = (u32*)calloc(capacity, sizeof(u32));
    return ea;
}

void entity_array_destroy(entity_array *ea) {
    free(ea->ids);
    free(ea->types);
    free(ea->positions);
    free(ea->rotations);
    free(ea->velocities);
    free(ea->state_flags);
    memset(ea, 0, sizeof(*ea));
}

entity_id entity_spawn(entity_array *ea, object_type type, vec3 pos) {
    if (ea->count >= ea->capacity) return ENTITY_NULL;
    u32 idx = ea->count;
    ea->ids[idx] = (entity_id)(idx + 1);
    ea->types[idx] = type;
    ea->positions[idx] = pos;
    ea->rotations[idx] = vec4_make(0,0,0,1);
    ea->velocities[idx] = vec3_make(0,0,0);
    ea->state_flags[idx] = 0;
    ea->count++;
    return ea->ids[idx];
}

void entity_despawn(entity_array *ea, entity_id id) {
    for (u32 i = 0; i < ea->count; i++) {
        if (ea->ids[i] == id) {
            u32 last = ea->count - 1;
            ea->ids[i]      = ea->ids[last];
            ea->types[i]    = ea->types[last];
            ea->positions[i] = ea->positions[last];
            ea->rotations[i] = ea->rotations[last];
            ea->velocities[i] = ea->velocities[last];
            ea->state_flags[i] = ea->state_flags[last];
            ea->count--;
            return;
        }
    }
}

/* --- Game state --- */

dsr_state *dsr_create(f64 tick_rate) {
    dsr_state *s = (dsr_state*)calloc(1, sizeof(dsr_state));
    if (!s) return NULL;
    s->tick       = tick_loop_make(tick_rate);
    s->arena      = tick_arena_create(1024 * 1024, 4 * 1024 * 1024);
    s->entities   = entity_array_create(4096);
    s->chain_hash = hash_chain_init();
    s->time_scale = 1.0f;
    s->flags      = 0;
    return s;
}

void dsr_destroy(dsr_state *s) {
    if (!s) return;
    entity_array_destroy(&s->entities);
    tick_arena_destroy(&s->arena);
    free(s);
}

int dsr_tick(dsr_state *s, f64 frame_dt) {
    int r = tick_loop_advance(&s->tick, frame_dt * (f64)s->time_scale);
    if (r != 0) {
        s->chain_hash = hash_chain_tick(s->chain_hash, s->tick.tick_id, s->entities.count);
        tick_end(&s->arena);
    }
    return r;
}
