#version 450

#include "viewer.glsl"

layout (location = 0) out vec4 out_color;

layout (push_constant) uniform position_data {
    vec4 pos[2];
    vec4 color;
} udata;

layout (set = 0, binding = 0) uniform viewer_data {
    viewer_desc data;
} uviewer;

void main() {
    gl_Position = uviewer.data.view_projection * udata.pos[gl_VertexIndex];
    gl_Position.y *= -1.0f;
    out_color = udata.color;
}
