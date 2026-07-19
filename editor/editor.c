#include "editor.h"
#include <imgui.h>
#include <stdio.h>
#include <string.h>

void editor_init(dsr_editor *ed, dsr_state *state) {
    (void)state;
    memset(ed, 0, sizeof(*ed));
    ed->show_viewport       = true;
    ed->show_inspector      = true;
    ed->show_asset_browser  = true;
    ed->show_terrain_editor = true;
    ed->show_console        = false;
    ed->show_debug          = true;
    ed->camera_speed        = 10.0f;
    ed->terrain_brush_size  = 5.0f;
    ed->terrain_brush_strength = 0.5f;
    ed->terrain_raise       = true;
    ed->selected_entity     = ENTITY_NULL;
    ed->console_buf[0]      = '\0';
}

void editor_frame(dsr_editor *ed, dsr_state *state, f32 dt) {
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open RFA...")) { }
        if (ImGui::MenuItem("New Map")) { }
        if (ImGui::MenuItem("Save Map")) { }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) { }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Viewport",       NULL, &ed->show_viewport);
        ImGui::MenuItem("Inspector",      NULL, &ed->show_inspector);
        ImGui::MenuItem("Asset Browser",  NULL, &ed->show_asset_browser);
        ImGui::MenuItem("Terrain Editor", NULL, &ed->show_terrain_editor);
        ImGui::MenuItem("Console",        NULL, &ed->show_console);
        ImGui::MenuItem("Debug",          NULL, &ed->show_debug);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Tools")) {
        ImGui::MenuItem("Wireframe", NULL, &ed->wireframe_mode);
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    /* --- Viewport --- */
    if (ed->show_viewport) {
        ImGui::Begin("Viewport", &ed->show_viewport);
        ImVec2 size = ImGui::GetContentRegionAvail();
        /* In a real implementation: render scene to framebure texture here */
        ImGui::Image((ImTextureID)(intptr_t)0, size);
        ImGui::End();
    }

    /* --- Entity Inspector --- */
    if (ed->show_inspector) {
        ImGui::Begin("Inspector", &ed->show_inspector);
        if (ed->selected_entity != ENTITY_NULL) {
            ImGui::Text("Entity ID: %u", ed->selected_entity);
            /* Find entity and show properties */
            for (u32 i = 0; i < state->entities.count; i++) {
                if (state->entities.ids[i] == ed->selected_entity) {
                    ImGui::DragFloat3("Position", (f32*)&state->entities.positions[i], 0.1f);
                    ImGui::DragFloat4("Rotation", (f32*)&state->entities.rotations[i], 0.01f);
                    ImGui::Text("Type: %d", state->entities.types[i]);
                    break;
                }
            }
        } else {
            ImGui::Text("No entity selected");
        }
        ImGui::End();
    }

    /* --- Entity List (as part of debug) --- */
    if (ed->show_debug) {
        ImGui::Begin("Debug", &ed->show_debug);
        ImGui::Text("Tick: %llu", (unsigned long long)state->tick.tick_id);
        ImGui::Text("Entities: %u/%u", state->entities.count, state->entities.capacity);
        ImGui::Text("Chain hash: 0x%016llX", (unsigned long long)state->chain_hash);
        ImGui::Separator();

        char buf[64];
        for (u32 i = 0; i < state->entities.count; i++) {
            snprintf(buf, sizeof(buf), "Entity %u", state->entities.ids[i]);
            if (ImGui::Selectable(buf, ed->selected_entity == state->entities.ids[i])) {
                ed->selected_entity = state->entities.ids[i];
            }
        }
        ImGui::End();
    }

    /* --- Asset Browser --- */
    if (ed->show_asset_browser) {
        ImGui::Begin("Asset Browser", &ed->show_asset_browser);
        ImGui::Text("RFA archives (drag to import):");
        ImGui::BulletText("Objects.rfa");
        ImGui::BulletText("StandardMesh.rfa");
        ImGui::BulletText("Menu.rfa");
        ImGui::Separator();
        ImGui::Text("Mods:");
        if (ImGui::Button("Open RFA...")) {
            /* TODO: file dialog */
        }
        ImGui::End();
    }

    /* --- Terrain Editor --- */
    if (ed->show_terrain_editor) {
        ImGui::Begin("Terrain Editor", &ed->show_terrain_editor);
        ImGui::SliderFloat("Brush Size", &ed->terrain_brush_size, 1.0f, 50.0f);
        ImGui::SliderFloat("Strength", &ed->terrain_brush_strength, 0.0f, 2.0f);
        ImGui::RadioButton("Raise", &ed->terrain_raise, 1); ImGui::SameLine();
        ImGui::RadioButton("Lower", &ed->terrain_raise, 0);
        if (ImGui::Button("Flatten")) { }
        if (ImGui::Button("Smooth")) { }
        ImGui::Separator();
        ImGui::Text("Heightmap: -- x --");
        ImGui::SliderFloat("Scale XZ", &ed->camera_speed, 1.0f, 20.0f);
        ImGui::End();
    }

    /* --- Console --- */
    if (ed->show_console) {
        ImGui::Begin("Console", &ed->show_console);
        ImGui::Text("Refractor .con console:");
        if (ImGui::InputText("##console", ed->console_buf, sizeof(ed->console_buf),
                             ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (ed->console_buf[0]) {
                printf("CONSOLE: %s\n", ed->console_buf);
            }
            ed->console_buf[0] = '\0';
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::End();
    }

    (void)dt;
}

void editor_shutdown(dsr_editor *ed) {
    (void)ed;
}
