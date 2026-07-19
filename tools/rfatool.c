/*
 * rfatool — .rfa archive packer/unpacker
 * Usage: rfatool [command] [options]
 *   rfatool l <file.rfa>           — list contents
 *   rfatool x <file.rfa> [dir]     — extract all
 *   rfatool p <dir> <out.rfa>      — pack directory
 */

#include "../dsr/dsr.h"
#include <stdio.h>
#include <string.h>

static void print_usage(void) {
    printf("Usage:\n");
    printf("  rfatool l <file.rfa>           — list archive\n");
    printf("  rfatool x <file.rfa> [dir]     — extract to directory\n");
    printf("  rfatool p <dir> <out.rfa>      — pack directory into RFA (TODO)\n");
}

static int cmd_list(int argc, char **argv) {
    if (argc < 1) { print_usage(); return 1; }
    rfa_archive rfa;
    rfa_error err = rfa_open(&rfa, argv[0]);
    if (err != RFA_OK) {
        fprintf(stderr, "Failed to open %s (error %d)\n", argv[0], err);
        return 1;
    }
    rfa_dump(&rfa);
    rfa_close(&rfa);
    return 0;
}

static int cmd_extract(int argc, char **argv) {
    if (argc < 1) { print_usage(); return 1; }
    rfa_archive rfa;
    rfa_error err = rfa_open(&rfa, argv[0]);
    if (err != RFA_OK) {
        fprintf(stderr, "Failed to open %s\n", argv[0]);
        return 1;
    }
    const char *dest = (argc > 1) ? argv[1] : ".";
    printf("Extracting to %s ...\n", dest);
    bool ok = rfa_extract_all(&rfa, dest);
    printf("%s\n", ok ? "Done" : "Failed");
    rfa_close(&rfa);
    return ok ? 0 : 1;
}

int main(int argc, char **argv) {
    if (argc < 3) { print_usage(); return 1; }

    if (strcmp(argv[1], "l") == 0) return cmd_list(argc - 2, argv + 2);
    if (strcmp(argv[1], "x") == 0) return cmd_extract(argc - 2, argv + 2);

    print_usage();
    return 1;
}
