#ifndef DSR_CON_H
#define DSR_CON_H

#include "core.h"

/* .con script parser for Refractor Engine
 * Parses ObjectTemplate.create/addTemplate, GeometryTemplate,
 * property assignments, includes, conditionals.
 *
 * References:
 *   https://bfmods.com/mdt/scripting/Intro.html
 *   https://bftech.darkstormstudios.co.uk/index.php?title=ConGrammar
 */

#define CON_MAX_ARGS 32
#define CON_MAX_LINE 1024
#define CON_MAX_INCLUDE_DEPTH 32

/* .con value types */
typedef enum {
    CON_NONE,
    CON_INT,
    CON_FLOAT,
    CON_STRING,
    CON_BOOL,
} con_value_type;

typedef struct {
    con_value_type type;
    union {
        i32  i;
        f32  f;
        bool b;
    };
    char s[256];
} con_value;

/* .con command types */
typedef enum {
    CMD_UNKNOWN,
    CMD_REM,           /* comment */
    CMD_BEGINREM,
    CMD_ENDREM,
    CMD_INCLUDE,
    CMD_RUN,
    CMD_RETURN,
    CMD_IF,
    CMD_ELSE,
    CMD_ENDIF,
    CMD_OBJECTTEMPLATE_CREATE,
    CMD_OBJECTTEMPLATE_ADDTEMPLATE,
    CMD_OBJECTTEMPLATE_SET,
    CMD_GEOMETRYTEMPLATE_CREATE,
    CMD_GEOMETRYTEMPLATE_SET,
    CMD_MATERIALTEMPLATE_CREATE,
    CMD_MATERIALTEMPLATE_SET,
    CMD_AITEMPLATE_CREATE,
    CMD_AITEMPLATE_SET,
    CMD_SOUNDTEMPLATE_CREATE,
    CMD_SOUNDTEMPLATE_SET,
    CMD_NETWORKABLEINFO_CREATE,
    CMD_NETWORKABLEINFO_SET,
    CMD_REMCOMPILER,
} con_command_type;

typedef struct {
    con_command_type type;
    char             raw[CON_MAX_LINE];
    int              arg_count;
    char             args[CON_MAX_ARGS][256];
} con_command;

/* Object template runtime representation */
typedef struct object_template object_template;
struct object_template {
    char             name[128];
    char             parent[128];
    object_type      obj_type;
    object_template *next;      /* linked list in arena */
    con_command      *commands;
    u32               cmd_count;
    u32               cmd_cap;
};

/* .con parser state */
typedef struct {
    arena           cmds_arena;    /* holds all parsed commands */
    arena           tmpl_arena;    /* holds object templates */
    object_template *templates;    /* linked list head */
    u32             template_count;
    /* internal */
    u32             line_number;
    bool            in_comment_block;
    u32             include_depth;
    char            current_file[256];
} con_parser;

/* Initialize parser */
void con_parser_init(con_parser *cp);

/* Destroy parser */
void con_parser_destroy(con_parser *cp);

/* Parse a single .con file */
bool con_parse(con_parser *cp, const char *path,
               char *(*read_fn)(const char *path, size_t *out_len, void *user),
               void *user);

/* Parse a string as .con */
bool con_parse_string(con_parser *cp, const char *string, const char *source_name);

/* Look up object template by name */
object_template *con_find_template(con_parser *cp, const char *name);

/* Dump parsed commands */
void con_dump(con_parser *cp);

#endif /* DSR_CON_H */
