#include "rfa.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/* LZO decompression stub — replace with miniLZO when linked */
static int lzo_decompress(const u8 *src, size_t src_len, u8 *dst, size_t *dst_len) {
    /* TODO: link miniLZO (lzo1x_decompress_safe) */
    /* For now, only uncompressed RFAs work */
    (void)src; (void)src_len; (void)dst; (void)dst_len;
    return -1;
}

static u32 read_u32_le(FILE *f) {
    u32 v = 0;
    if (fread(&v, 4, 1, f) != 1) return 0;
    return v;
}

static char *read_string(FILE *f, u32 *out_len) {
    u32 len = read_u32_le(f);
    if (out_len) *out_len = len;
    if (len == 0 || len > 4096) return NULL;
    char *s = (char*)calloc(len + 1, 1);
    if (fread(s, 1, len, f) != len) { free(s); return NULL; }
    s[len] = '\0';
    return s;
}

rfa_error rfa_open(rfa_archive *rfa, const char *path) {
    memset(rfa, 0, sizeof(*rfa));
    rfa->path = strdup(path);
    if (!rfa->path) return RFA_ERR_ALLOC;

    rfa->fp = fopen(path, "rb");
    if (!rfa->fp) { free(rfa->path); return RFA_ERR_OPEN; }

    fseek(rfa->fp, 0, SEEK_END);
    rfa->file_size = (u32)ftell(rfa->fp);
    rewind(rfa->fp);

    /* Check for v1.1 header string */
    char sig[29] = {0};
    if (rfa->file_size >= 28) {
        fread(sig, 1, 28, rfa->fp);
        sig[28] = '\0';
        if (strcmp(sig, "Refractor2 FlatArchive 1.1  ") == 0) {
            rfa->is_v1dot1 = true;
        } else {
            rewind(rfa->fp);
        }
    }

    /* Read offset to directory */
    u32 dir_offset = read_u32_le(rfa->fp);
    u32 compressed_flag = read_u32_le(rfa->fp);
    rfa->compressed = (compressed_flag == 1);

    if (!rfa->is_v1dot1) {
        /* Skip 143 random bytes + 1 unknown + 4 xpack_header */
        fseek(rfa->fp, 143 + 1, SEEK_CUR);
        u32 enc = read_u32_le(rfa->fp);
        /* We can't reconstruct the sum without the random bytes, but we don't need it */
        rfa->xpack_header_id = enc;
    }

    /* Seek to directory */
    fseek(rfa->fp, dir_offset, SEEK_SET);

    u32 entry_count = read_u32_le(rfa->fp);
    if (entry_count == 0 || entry_count > RFA_MAX_ENTRIES) {
        fclose(rfa->fp);
        rfa->fp = NULL;
        return RFA_ERR_CORRUPT;
    }

    /* Allocate entry arena */
    rfa->entry_arena = arena_create(sizeof(rfa_entry) * entry_count + entry_count * RFA_MAX_NAME);
    rfa->entries = arena_alloc_n(&rfa->entry_arena, rfa_entry, entry_count);
    rfa->entry_count = entry_count;

    for (u32 i = 0; i < entry_count; i++) {
        rfa_entry *e = &rfa->entries[i];
        char *name = read_string(rfa->fp, NULL);
        if (!name) { rfa->entry_count = i; break; }
        strncpy(e->path, name, RFA_MAX_NAME - 1);
        free(name);

        e->info.compressed_size   = read_u32_le(rfa->fp);
        e->info.uncompressed_size = read_u32_le(rfa->fp);
        e->info.data_offset       = read_u32_le(rfa->fp);
        e->unknown[0] = read_u32_le(rfa->fp);
        e->unknown[1] = read_u32_le(rfa->fp);
        e->unknown[2] = read_u32_le(rfa->fp);
    }

    return RFA_OK;
}

void rfa_close(rfa_archive *rfa) {
    if (rfa->fp) fclose(rfa->fp);
    free(rfa->path);
    arena_destroy(&rfa->entry_arena);
    memset(rfa, 0, sizeof(*rfa));
}

int rfa_find(rfa_archive *rfa, const char *path) {
    /* Normalize: lowercase, '/' */
    char norm[RFA_MAX_NAME];
    size_t len = strlen(path);
    for (size_t i = 0; i < len && i < RFA_MAX_NAME - 1; i++) {
        char c = path[i];
        norm[i] = (c >= 'A' && c <= 'Z') ? (c + 32) : (c == '\\' ? '/' : c);
    }
    norm[len < RFA_MAX_NAME - 1 ? len : RFA_MAX_NAME - 1] = '\0';

    for (u32 i = 0; i < rfa->entry_count; i++) {
        char en[RFA_MAX_NAME];
        size_t elen = strlen(rfa->entries[i].path);
        for (size_t j = 0; j < elen && j < RFA_MAX_NAME - 1; j++) {
            char c = rfa->entries[i].path[j];
            en[j] = (c >= 'A' && c <= 'Z') ? (c + 32) : (c == '\\' ? '/' : c);
        }
        en[elen < RFA_MAX_NAME - 1 ? elen : RFA_MAX_NAME - 1] = '\0';

        if (strcmp(norm, en) == 0) return (int)i;
    }
    return -1;
}

i64 rfa_extract_to_buf(rfa_archive *rfa, int idx, u8 *buf, size_t buf_size) {
    if (idx < 0 || idx >= (int)rfa->entry_count) return -1;
    rfa_entry *e = &rfa->entries[idx];

    if (!rfa->compressed) {
        if (buf_size < e->info.uncompressed_size) return -1;
        fseek(rfa->fp, e->info.data_offset, SEEK_SET);
        size_t read = fread(buf, 1, e->info.uncompressed_size, rfa->fp);
        return (i64)read;
    }

    /* Compressed: read segment headers, LZO decompress */
    fseek(rfa->fp, e->info.data_offset, SEEK_SET);
    u32 segment_count = read_u32_le(rfa->fp);

    u8 *write_ptr = buf;
    size_t total = 0;
    for (u32 s = 0; s < segment_count; s++) {
        u32 seg_offset = e->info.data_offset + 4 + s * 12;
        fseek(rfa->fp, seg_offset, SEEK_SET);
        u32 comp_size = read_u32_le(rfa->fp);
        u32 uncomp_size = read_u32_le(rfa->fp);
        u32 data_off    = read_u32_le(rfa->fp);

        if (comp_size == 0 || uncomp_size == 0) continue;
        if (total + uncomp_size > buf_size) return -1;

        fseek(rfa->fp, e->info.data_offset + 4 + segment_count * 12 + data_off, SEEK_SET);
        u8 *comp_buf = (u8*)malloc(comp_size);
        if (!comp_buf) return -1;
        fread(comp_buf, 1, comp_size, rfa->fp);

        size_t out_len = uncomp_size;
        int ret = lzo_decompress(comp_buf, comp_size, write_ptr, &out_len);
        free(comp_buf);
        if (ret != 0) return -1;

        write_ptr += out_len;
        total += out_len;
    }
    return (i64)total;
}

bool rfa_extract_to_file(rfa_archive *rfa, int idx, const char *dest) {
    if (idx < 0 || idx >= (int)rfa->entry_count) return false;
    rfa_entry *e = &rfa->entries[idx];
    size_t sz = rfa->compressed ? e->info.uncompressed_size : e->info.compressed_size;
    if (sz == 0) return false;

    /* Create parent directories */
    char dir[RFA_MAX_NAME];
    strncpy(dir, dest, RFA_MAX_NAME - 1);
    dir[RFA_MAX_NAME - 1] = '\0';
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        struct stat st = {0};
        if (stat(dir, &st) == -1) {
            char cmd[RFA_MAX_NAME + 32];
            snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", dir);
            system(cmd);
        }
    }

    /* If uncompressed, direct copy */
    if (!rfa->compressed) {
        FILE *out = fopen(dest, "wb");
        if (!out) return false;
        fseek(rfa->fp, e->info.data_offset, SEEK_SET);
        u8 buf[65536];
        size_t remaining = e->info.compressed_size;
        while (remaining > 0) {
            size_t chunk = remaining > 65536 ? 65536 : remaining;
            size_t r = fread(buf, 1, chunk, rfa->fp);
            if (r != chunk) { fclose(out); return false; }
            fwrite(buf, 1, r, out);
            remaining -= r;
        }
        fclose(out);
        return true;
    }

    /* Compressed: extract to buffer then write */
    u8 *buf = (u8*)malloc(sz);
    if (!buf) return false;
    i64 written = rfa_extract_to_buf(rfa, idx, buf, sz);
    if (written < 0) { free(buf); return false; }
    FILE *out = fopen(dest, "wb");
    if (!out) { free(buf); return false; }
    fwrite(buf, 1, (size_t)written, out);
    fclose(out);
    free(buf);
    return true;
}

bool rfa_extract_all(rfa_archive *rfa, const char *dest_dir) {
    for (u32 i = 0; i < rfa->entry_count; i++) {
        char full_path[RFA_MAX_NAME];
        if (dest_dir && dest_dir[0]) {
            snprintf(full_path, RFA_MAX_NAME - 1, "%s/%s", dest_dir, rfa->entries[i].path);
        } else {
            strncpy(full_path, rfa->entries[i].path, RFA_MAX_NAME - 1);
        }
        full_path[RFA_MAX_NAME - 1] = '\0';
        if (!rfa_extract_to_file(rfa, (int)i, full_path)) {
            return false;
        }
    }
    return true;
}

void rfa_dump(rfa_archive *rfa) {
    printf("RFA: %s\n", rfa->path);
    printf("  size: %u\n", rfa->file_size);
    printf("  compressed: %s\n", rfa->compressed ? "yes" : "no");
    printf("  entries: %u\n", rfa->entry_count);
    printf("  v1.1: %s\n", rfa->is_v1dot1 ? "yes" : "no");
    for (u32 i = 0; i < rfa->entry_count; i++) {
        rfa_entry *e = &rfa->entries[i];
        printf("  [%4u] %s (comp=%u, uncomp=%u, off=%u)\n",
               i, e->path, e->info.compressed_size,
               e->info.uncompressed_size, e->info.data_offset);
    }
}
