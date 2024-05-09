#type vertex

#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../shaders/common.glsl"

layout (location = 0) in vec3 in_position;

layout (std430, set = SHADER_GLOBALS_BIND_GROUP, binding = SHADER_INSTANCE_STORAGE_BUFFER_BINDING) readonly buffer Instance_Buffer
{
    Instance_Data instances[];
};

void main()
{
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    float noop = NOOP(in_position.x) * NOOP(float(instances[gl_InstanceIndex].entity_index)) * NOOP(globals.gamma);
    gl_Position = vec4(uv * 2.0f + -1.0f, 0.0f, 1.0f) * noop;
}

#type fragment

#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

#include "../shaders/common.glsl"

layout(set = SHADER_PASS_BIND_GROUP, binding = SHADER_BINDLESS_TEXTURES_BINDING) uniform sampler2D u_textures[];

vec4 sample_texture(uint texture_index, vec2 uv)
{
    return texture( u_textures[ nonuniformEXT( texture_index ) ], uv );
}

layout (std430, set = SHADER_PASS_BIND_GROUP, binding = SHADER_LIGHT_STORAGE_BUFFER_BINDING) readonly buffer Light_Buffer
{
    Light lights[];
};

layout (std430, set = SHADER_PASS_BIND_GROUP, binding = SHADER_LIGHT_BINS_STORAGE_BUFFER_BINDING) readonly buffer Light_Bins_Buffer
{
    uint light_bins[];
};

layout (set = SHADER_PASS_BIND_GROUP, binding = SHADER_HEAD_INDEX_STORAGE_IMAGE_BINDING, r32ui) uniform uimage2D head_index_image;

layout (std430, set = SHADER_PASS_BIND_GROUP, binding = SHADER_NODE_STORAGE_BUFFER_BINDING) readonly buffer Node_Buffer
{
    Node nodes[];
};

layout (std430, set = SHADER_PASS_BIND_GROUP, binding = SHADER_NODE_COUNT_STORAGE_BUFFER_BINDING) readonly buffer Node_Count_Buffer
{
    int node_count;
};

layout(location = 0) out vec4 out_color;

#define MAX_FRAGMENT_COUNT 128

void main()
{
    Node fragments[MAX_FRAGMENT_COUNT];
    int count = 0;

    ivec2 coords = ivec2(gl_FragCoord.xy);
    uint node_index = imageLoad(head_index_image, coords).r;

    while (node_index != 0xffffffff && count < MAX_FRAGMENT_COUNT)
    {
        fragments[count] = nodes[node_index];
        node_index = fragments[count].next;
        ++count;
    }

    // Do the insertion sort
    for (uint i = 1; i < count; ++i)
    {
        Node insert = fragments[i];
        uint j = i;
        while (j > 0 && insert.depth > fragments[j - 1].depth)
        {
            fragments[j] = fragments[j - 1];
            --j;
        }
        fragments[j] = insert;
    }

    vec4 color = fragments[0].color;

    for (int i = 1; i < count; ++i)
    {
        color = mix(color, fragments[i].color, fragments[i].color.a);
    }

    float noop = NOOP(lights[0].color.r) * NOOP(float(light_bins[0])) * NOOP(float(node_count));

    vec3 c = color.rgb;
    c = c / (c + vec3(1.0));
    c = linear_to_srgb(c, globals.gamma);
    out_color = vec4(c * noop, 1.0);
}