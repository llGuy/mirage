#pragma once

#include <imgui.h>
#include <ImGuizmo.h>

#include "render_graph.hpp"

// Anything that wants debug overlay information should use this procedure
using debug_overlay_proc = void (*)();

void init_debug_overlay();
void render_debug_overlay(VkCommandBuffer cmdbuf);

void register_debug_overlay_client(const char *name, debug_overlay_proc proc, bool open_by_default = false);

bool overlay_has_focus();
