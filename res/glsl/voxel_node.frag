#version 450

layout (location = 0) flat in uint in_sdf_list_node;
layout (location = 0) out vec4 out_color;

void main() 
{
  // TODO: Actual tracing
  out_color = vec4(1.0);
}
