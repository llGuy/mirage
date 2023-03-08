#pragma once

#include <imgui.h>
#include <ImGuizmo.h>

#include "render_graph.hpp"

// Anything that wants debug overlay information should use this procedure
using debug_overlay_proc = void (*)();

void init_debug_overlay();
void render_debug_overlay(VkCommandBuffer cmdbuf, graph_resource_tracker &tracker);

void register_debug_overlay_client(const char *name, debug_overlay_proc proc, bool open_by_default = false);

bool overlay_has_focus();

struct dbg_rectangle 
{
  v2 positions[4];
  // With alpha blending
  v4 color;
};

void add_debug_rectangle(const dbg_rectangle &rect);

// These are going to be defined in world space
struct dbg_line 
{
  v4 positions[2];
  v4 color;
};

void add_debug_line(const dbg_line &line);
