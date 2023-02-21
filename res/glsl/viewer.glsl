#ifndef VIEWER_GLSL
#define VIEWER_GLSL

struct viewer_desc {
    mat4 projection;
    mat4 view;
    mat4 inverse_projection;
    mat4 inverse_view;
    mat4 view_projection;

    vec3 wposition;
    vec3 wview_dir;
    vec3 wup;

    float fov;
    float aspect_ratio;
    float near;
    float far;
};

#endif
