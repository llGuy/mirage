#version 450

layout (location = 0) out vec4 out_color;

layout (push_constant) uniform position_data {
    vec2 pos[4];
    vec4 color;
} udata;

uint indices[] = uint[](
    0, 1, 2,
    2, 0, 3
);

void main() {
    gl_Position = vec4(udata.pos[indices[gl_VertexIndex]], 0.0, 1.0);
    gl_Position.y *= -1.0f;
    out_color = udata.color;
}
