#include "con.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdio.h>

/* --- Tokenizer helpers --- */

static int is_con_identifier(char c) {
    return isalnum(c) || c == '_' || c == '.' || c == '/';
}

static void trim_trailing(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\r' || s[len-1] == '\n')) {
        s[--len] = '\0';
    }
}

static char *next_token(char **cursor) {
    while (**cursor && **cursor <= ' ') (*cursor)++;
    if (**cursor == '\0') return NULL;

    static char tok[256];
    int i = 0;

    if (**cursor == '"') {
        (*cursor)++;
        while (**cursor && **cursor != '"' && i < 255) {
            if (**cursor == '\\' && *(*cursor + 1)) {
                (*cursor)++;
                if (**cursor == 'n') tok[i++] = '\n';
                else tok[i++] = **cursor;
            } else {
                tok[i++] = **cursor;
            }
            (*cursor)++;
        }
        if (**cursor == '"') (*cursor)++;
    } else {
        while (**cursor && **cursor > ' ' && i < 255) {
            tok[i++] = **cursor;
            (*cursor)++;
        }
    }
    tok[i] = '\0';
    return tok;
}

/* --- Command classification --- */

static con_command_type classify_command(const char *cmd) {
    if (!cmd) return CMD_UNKNOWN;

    if (strcasecmp(cmd, "rem") == 0) return CMD_REM;
    if (strcasecmp(cmd, "beginrem") == 0) return CMD_BEGINREM;
    if (strcasecmp(cmd, "endrem") == 0) return CMD_ENDREM;
    if (strcasecmp(cmd, "include") == 0) return CMD_INCLUDE;
    if (strcasecmp(cmd, "run") == 0) return CMD_RUN;
    if (strcasecmp(cmd, "return") == 0) return CMD_RETURN;
    if (strcasecmp(cmd, "if") == 0) return CMD_IF;
    if (strcasecmp(cmd, "else") == 0) return CMD_ELSE;
    if (strcasecmp(cmd, "endif") == 0) return CMD_ENDIF;

    if (strcasecmp(cmd, "objecttemplate.create") == 0 ||
        strcasecmp(cmd, "ObjectTemplate.create") == 0) return CMD_OBJECTTEMPLATE_CREATE;
    if (strcasecmp(cmd, "objecttemplate.addtemplate") == 0) return CMD_OBJECTTEMPLATE_ADDTEMPLATE;
    if (strncasecmp(cmd, "objecttemplate.", 15) == 0) return CMD_OBJECTTEMPLATE_SET;

    if (strcasecmp(cmd, "geometrytemplate.create") == 0) return CMD_GEOMETRYTEMPLATE_CREATE;
    if (strncasecmp(cmd, "geometrytemplate.", 17) == 0) return CMD_GEOMETRYTEMPLATE_SET;

    if (strcasecmp(cmd, "materialtemplate.create") == 0) return CMD_MATERIALTEMPLATE_CREATE;
    if (strncasecmp(cmd, "materialtemplate.", 17) == 0) return CMD_MATERIALTEMPLATE_SET;

    if (strcasecmp(cmd, "aitemplate.create") == 0) return CMD_AITEMPLATE_CREATE;
    if (strncasecmp(cmd, "aitemplate.", 11) == 0) return CMD_AITEMPLATE_SET;

    if (strcasecmp(cmd, "soundtemplate.create") == 0) return CMD_SOUNDTEMPLATE_CREATE;
    if (strncasecmp(cmd, "soundtemplate.", 14) == 0) return CMD_SOUNDTEMPLATE_SET;

    if (strcasecmp(cmd, "networkableinfo.create") == 0) return CMD_NETWORKABLEINFO_CREATE;
    if (strncasecmp(cmd, "networkableinfo.", 16) == 0) return CMD_NETWORKABLEINFO_SET;

    if (strcasecmp(cmd, "remcompiler") == 0) return CMD_REMCOMPILER;

    return CMD_UNKNOWN;
}

/* --- Parser --- */

void con_parser_init(con_parser *cp) {
    memset(cp, 0, sizeof(*cp));
    cp->cmds_arena  = arena_create(256 * 1024);
    cp->tmpl_arena  = arena_create(128 * 1024);
    cp->in_comment_block = false;
    cp->include_depth = 0;
    cp->line_number = 0;
    strcpy(cp->current_file, "<string>");
}

void con_parser_destroy(con_parser *cp) {
    arena_destroy(&cp->cmds_arena);
    arena_destroy(&cp->tmpl_arena);
    memset(cp, 0, sizeof(*cp));
}

static bool parse_line(con_parser *cp, const char *line) {
    /* Skip BOM if any */
    if ((u8)line[0] == 0xEF && (u8)line[1] == 0xBB && (u8)line[2] == 0xBF)
        line += 3;

    /* Trim leading whitespace */
    while (*line && *line <= ' ') line++;
    if (*line == '\0') return true;

    /* Comment block */
    if (cp->in_comment_block) {
        if (strncasecmp(line, "endrem", 6) == 0) {
            cp->in_comment_block = false;
        }
        return true;
    }

    /* Comment lines */
    if (strncasecmp(line, "rem", 3) == 0 && (line[3] <= ' ' || line[3] == '\0')) {
        return true;
    }

    /* Allocate command */
    con_command *cmd = arena_alloc_s(&cp->cmds_arena, con_command);
    if (!cmd) return false;
    memset(cmd, 0, sizeof(*cmd));
    strncpy(cmd->raw, line, CON_MAX_LINE - 1);
    cmd->raw[CON_MAX_LINE - 1] = '\0';

    /* Tokenize */
    char buf[CON_MAX_LINE];
    strncpy(buf, line, CON_MAX_LINE - 1);
    buf[CON_MAX_LINE - 1] = '\0';
    trim_trailing(buf);

    char *cursor = buf;

    /* Handle beginrem/endrem */
    {
        char *t = next_token(&cursor);
        if (t && strcasecmp(t, "beginrem") == 0) {
            cp->in_comment_block = true;
            return true;
        }
        if (t && strcasecmp(t, "endrem") == 0) {
            return true; /* should not happen, already handled above */
        }
    }

    /* Re-tokenize for real */
    cursor = buf;
    cmd->arg_count = 0;
    char *tok;
    while ((tok = next_token(&cursor)) != NULL && cmd->arg_count < CON_MAX_ARGS) {
        strncpy(cmd->args[cmd->arg_count], tok, 255);
        cmd->args[cmd->arg_count][255] = '\0';
        cmd->arg_count++;
    }

    if (cmd->arg_count > 0) {
        cmd->type = classify_command(cmd->args[0]);

        /* Track ObjectTemplate.create */
        if (cmd->type == CMD_OBJECTTEMPLATE_CREATE && cmd->arg_count >= 3) {
            object_template *tmpl = arena_alloc_s(&cp->tmpl_arena, object_template);
            if (tmpl) {
                memset(tmpl, 0, sizeof(*tmpl));
                strncpy(tmpl->name, cmd->args[2], 127);
                tmpl->name[127] = '\0';
                /* Map type string */
                if (strcasecmp(cmd->args[1], "SimpleObject") == 0) tmpl->obj_type = OBJ_SIMPLE;
                else if (strcasecmp(cmd->args[1], "PlayerControlObject") == 0) tmpl->obj_type = OBJ_PLAYER_CONTROL;
                else if (strcasecmp(cmd->args[1], "Soldier") == 0) tmpl->obj_type = OBJ_SOLDIER;
                else if (strcasecmp(cmd->args[1], "HandFireArm") == 0) tmpl->obj_type = OBJ_WEAPON;
                else if (strcasecmp(cmd->args[1], "Projectile") == 0) tmpl->obj_type = OBJ_PROJECTILE;
                else if (strncasecmp(cmd->args[1], "Bundle", 6) == 0) tmpl->obj_type = OBJ_BUNDLE;
                else tmpl->obj_type = OBJ_SIMPLE;

                tmpl->next = cp->templates;
                cp->templates = tmpl;
                cp->template_count++;
            }
        }

        /* Handle include */
        if ((cmd->type == CMD_INCLUDE || cmd->type == CMD_RUN) && cmd->arg_count >= 2) {
            /* The caller's read_fn handles actual file resolution */
        }
    }

    return true;
}

bool con_parse_string(con_parser *cp, const char *string, const char *source_name) {
    if (source_name) strncpy(cp->current_file, source_name, 255);
    cp->line_number = 0;

    char line[CON_MAX_LINE];
    size_t pos = 0;
    size_t len = strlen(string);

    while (pos < len) {
        size_t start = pos;
        while (pos < len && string[pos] != '\n') pos++;
        size_t line_len = pos - start;
        if (line_len >= CON_MAX_LINE) line_len = CON_MAX_LINE - 1;
        memcpy(line, string + start, line_len);
        line[line_len] = '\0';
        cp->line_number++;

        if (!parse_line(cp, line)) return false;
        if (string[pos] == '\n') pos++;
    }

    return true;
}

bool con_parse(con_parser *cp, const char *path,
               char *(*read_fn)(const char *path, size_t *out_len, void *user),
               void *user)
{
    size_t len = 0;
    char *content = read_fn(path, &len, user);
    if (!content) return false;

    strncpy(cp->current_file, path, 255);
    bool ok = con_parse_string(cp, content, path);

    if (content) free(content);
    return ok;
}

object_template *con_find_template(con_parser *cp, const char *name) {
    object_template *t = cp->templates;
    while (t) {
        if (strcasecmp(t->name, name) == 0) return t;
        t = t->next;
    }
    return NULL;
}

void con_dump(con_parser *cp) {
    printf("CON parser state:\n");
    printf("  templates: %u\n", cp->template_count);
    printf("  commands arena: %zu/%zu\n",
           cp->cmds_arena.offset, cp->cmds_arena.size);

    printf("\nTemplates:\n");
    object_template *t = cp->templates;
    while (t) {
        printf("  [%s] type=%d\n", t->name, t->obj_type);
        t = t->next;
    }
}
