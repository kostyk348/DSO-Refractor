#ifndef DSR_EDITOR_H
#define DSR_EDITOR_H

#include "../dsr/dsr.h"

/*
 * DSO-Refractor Editor
 * ImGui-based level editor with live-editing and hot-reload.
 */

typedef struct {
    bool  show_viewport;
    bool  show_inspector;
    bool  show_asset_browser;
    bool  show_terrain_editor;
    bool  show_console;
    bool  show_debug;

    /* Editor state */
    entity_id selected_entity;
    bool      wireframe_mode;
    f32       camera_speed;

    /* terrain editing */
    f32  terrain_brush_size;
    f32  terrain_brush_strength;
    bool terrain_raise;

    /* Console */
    char console_buf[256];
} dsr_editor;

/* Initialize editor */
void editor_init(dsr_editor *ed, dsr_state *state);

/* Run editor frame */
void editor_frame(dsr_editor *ed, dsr_state *state, f32 dt);

/* Shutdown editor */
void editor_shutdown(dsr_editor *ed);

#endif /* DSR_EDITOR_H */
