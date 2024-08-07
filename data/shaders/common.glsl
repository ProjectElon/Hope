#ifndef COMMON_GLSL
#define COMMON_GLSL

#define SHADER_GLOBALS_BIND_GROUP 0
#define SHADER_GLOBALS_UNIFORM_BINDING 0
#define SHADER_INSTANCE_STORAGE_BUFFER_BINDING 1

#define SHADER_PASS_BIND_GROUP 1
#define SHADER_LIGHT_STORAGE_BUFFER_BINDING 0
#define SHADER_LIGHT_BINS_STORAGE_BUFFER_BINDING 1
#define SHADER_BINDLESS_TEXTURES_BINDING 2

#define SHADER_OBJECT_BIND_GROUP 2
#define SHADER_MATERIAL_UNIFORM_BUFFER_BINDING 0

#define SHADER_MATERIAL_TYPE_OPAQUE 0
#define SHADER_MATERIAL_TYPE_ALPHA_CUTOFF 1
#define SHADER_MATERIAL_TYPE_TRANSPARENT 2

#define SHADER_LIGHT_TYPE_DIRECTIONAL 0
#define SHADER_LIGHT_TYPE_POINT 1
#define SHADER_LIGHT_TYPE_SPOT 2

#ifndef __cplusplus

#define PI 3.1415926535897932384626433832795
#define NOOP(x) step(0.0, clamp((x), 0.0, 1.0))

vec3 srgb_to_linear(vec3 color, float gamma)
{
    return pow(color, vec3(gamma));
}

vec4 srgb_to_linear(vec4 color, float gamma)
{
    return pow(color, vec4(gamma));
}

vec3 linear_to_srgb(vec3 color, float gamma)
{
    return pow(color, vec3(1.0/gamma));
}

#endif

struct Shader_Globals
{
    mat4 view;
    mat4 projection;

    float eye[3];

    float ambient[3];

    float gamma;
    float z_near;
    float z_far;

    uint light_count;
    uint directional_light_count;
    uint light_bin_count;

    int max_node_count;

    uint irradiance_map;

    uint prefilter_map_lod;
    uint prefilter_map;
    uint brdf_lut;

    uint use_environment_map;
};

struct Shader_Instance_Data
{
    mat4 local_to_world;
    int entity_index;
};

struct Shader_Light
{
    float direction[3];
    float position[3];
    float color[3];
    uint screen_aabb[2];
    uint type;
    float radius;
    float outer_angle;
    float inner_angle;
};

struct Shader_Node
{
    float color[4];
    float depth;
    uint next;
    int entity_index;
};

#endif // COMMON_GLSL