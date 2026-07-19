#ifndef DSR_RFA_H
#define DSR_RFA_H

#include "core.h"
#include <stdio.h>

/* Refractor Flat Archive (.rfa) reader
 * Format: BF1942 Refractor Engine 1 archives.
 * See: https://wiki.xentax.spektr.name/index.php/Battlefield_1942_-_Demo
 */

#define RFA_MAX_NAME 512
#define RFA_MAX_ENTRIES 65536
#define RFA_XPACK_MAGIC_DEFAULT 0x48128321

typedef enum {
    RFA_OK = 0,
    RFA_ERR_OPEN,
    RFA_ERR_READ,
    RFA_ERR_CORRUPT,
    RFA_ERR_LZO,
    RFA_ERR_ALLOC,
} rfa_error;

typedef struct {
    u32  compressed_size;
    u32  uncompressed_size;
    u32  data_offset;
} rfa_block_info;

typedef struct {
    char   path[RFA_MAX_NAME];
    rfa_block_info info;    /* for uncompressed: info.uncompressed_size = compressed_size */
    u32    unknown[3];
} rfa_entry;

typedef struct {
    char    *path;
    u32      entry_count;
    rfa_entry *entries;
    bool     compressed;
    u32      xpack_header_id;
    u32      file_size;
    /* internal */
    FILE    *fp;
    arena    entry_arena;
    bool     is_v1dot1;
} rfa_archive;

/* Open .rfa file, read directory */
rfa_error rfa_open(rfa_archive *rfa, const char *path);

/* Close .rfa */
void rfa_close(rfa_archive *rfa);

/* Find entry by path (case-insensitive, '/' normalized) */
int rfa_find(rfa_archive *rfa, const char *path);

/* Extract entry to buffer. Returns bytes written, or <0 on error */
i64 rfa_extract_to_buf(rfa_archive *rfa, int idx, u8 *buf, size_t buf_size);

/* Extract entry to file */
bool rfa_extract_to_file(rfa_archive *rfa, int idx, const char *dest);

/* Extract all entries to directory */
bool rfa_extract_all(rfa_archive *rfa, const char *dest_dir);

/* Debug dump */
void rfa_dump(rfa_archive *rfa);

#endif /* DSR_RFA_H */
