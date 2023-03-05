#version 450

#include "viewer.glsl"

layout (set = 0, binding = 0) uniform viewer_data {
    viewer_desc data;
} uviewer;

const vec3 positions[] = vec3[](
    // front
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 0.0, 1.0),
    vec3(1.0, 1.0, 1.0),
    vec3(0.0, 1.0, 1.0),
    // back
    vec3(0.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 1.0, 0.0),
    vec3(0.0, 1.0, 0.0)
);

const uint indices[] = uint[](
    // front
    0, 1, 2,
    2, 3, 0,
    // right
    1, 5, 6,
    6, 2, 1,
    // back
    7, 6, 5,
    5, 4, 7,
    // left
    4, 0, 3,
    3, 7, 4,
    // bottom
    4, 5, 1,
    1, 0, 4,
    // top
    3, 2, 6,
    6, 7, 3
);

void main() {
    uint idx = indices[gl_VertexIndex];
    vec3 vtx = positions[idx];

    // gl_Position = uviewer.data.
}
