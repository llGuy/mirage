#ifndef BLOB_GLSL
#define BLOB_GLSL


#define max_blob_count 32


#define sdf_sphere 0x0
#define sdf_cube   0x1


#define sdf_union               0x0
#define sdf_sub                 0x1
#define sdf_intersect           0x2
#define sdf_smooth_union        0x3
#define sdf_smooth_sub          0x4
#define sdf_smooth_intersect    0x5


struct blob {
    // Add rotations later
    vec4 position;
    vec4 scale;
    uint type;
    uint op;
    uint pad0;
    uint pad1;
};


#endif
