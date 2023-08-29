#pragma once

#include "core/defines.h"
#include "renderer_types.h"
#include "camera.h"

enum RenderingAPI
{
    RenderingAPI_Vulkan
};

#define MAX_TEXTURE_COUNT 4096
#define MAX_MATERIAL_COUNT 4096
#define MAX_STATIC_MESH_COUNT 4096
#define MAX_SHADER_COUNT 4096
#define MAX_PIPELINE_STATE_COUNT 4096
#define MAX_SCENE_NODE_COUNT 4096
#define GAMMA 2.2f

struct Directional_Light
{
    glm::vec3 direction;
    glm::vec4 color;
    F32 intensity;
};

struct Scene_Data
{
    glm::mat4 view;
    glm::mat4 projection;

    Directional_Light directional_light;
};

struct Renderer_State
{
    struct Engine *engine;

    U32 back_buffer_width;
    U32 back_buffer_height;

    U32 texture_count;
    Texture *textures;

    U32 material_count;
    Material *materials;

    U32 static_mesh_count;
    Static_Mesh *static_meshes;

    U32 shader_count;
    Shader *shaders;

    U32 pipeline_state_count;
    Pipeline_State *pipeline_states;

    U32 scene_node_count;
    Scene_Node *scene_nodes;

    Shader *mesh_vertex_shader;
    Shader *mesh_fragment_shader;
    Pipeline_State *mesh_pipeline;

    Texture *white_pixel_texture;
    Texture *normal_pixel_texture;

    Scene_Data scene_data;

    struct Free_List_Allocator *transfer_allocator;
};

bool init_renderer_state(struct Engine *engine,
                         Renderer_State *renderer_state,
                         struct Memory_Arena *arena);

void deinit_renderer_state(struct Renderer *renderer, Renderer_State *renderer_state);

struct Renderer
{
    bool (*init)(struct Renderer_State *renderer_state,
                 struct Engine *engine,
                 struct Memory_Arena *arena);

    void (*wait_for_gpu_to_finish_all_work)(struct Renderer_State *renderer_state);
    void (*deinit)(struct Renderer_State *renderer_state);

    void (*on_resize)(struct Renderer_State *renderer_state, U32 width, U32 height);

    void (*begin_frame)(struct Renderer_State *renderer_state, const Scene_Data *scene_data);
    void (*submit_static_mesh)(struct Renderer_State *renderer_state, const struct Static_Mesh *mesh, const glm::mat4 &transfom);
    void (*end_frame)(struct Renderer_State *renderer_state);

    bool (*create_texture)(Texture *texture, const Texture_Descriptor &descriptor);

    void (*destroy_texture)(Texture *texture);

    bool (*create_shader)(Shader *shader, const Shader_Descriptor &descriptor);
    void (*destroy_shader)(Shader *shader);

    bool (*create_pipeline_state)(Pipeline_State *pipeline_state, const Pipeline_State_Descriptor &descriptor);

    void (*destroy_pipeline_state)(Pipeline_State *pipeline_state);

    bool (*create_static_mesh)(Static_Mesh *static_mesh, const Static_Mesh_Descriptor &descriptor);
    void (*destroy_static_mesh)(Static_Mesh *static_mesh);

    bool (*create_material)(Material *material, const Material_Descriptor &descriptor);
    void (*destroy_material)(Material *material);

    void (*imgui_new_frame)();
};

bool request_renderer(RenderingAPI rendering_api, Renderer *renderer);

Scene_Node *add_child_scene_node(Renderer_State *renderer_state,
                                 Scene_Node *parent);

Scene_Node* load_model(const char *path, Renderer *renderer,
                       Renderer_State *renderer_state);

void render_scene_node(Renderer *renderer, Renderer_State *renderer_state, Scene_Node *scene_node, glm::mat4 transform);

Texture *allocate_texture(Renderer_State *renderer_state);
Material *allocate_material(Renderer_State *renderer_state);
Static_Mesh *allocate_static_mesh(Renderer_State *renderer_state);
Shader *allocate_shader(Renderer_State *renderer_state);
Pipeline_State *allocate_pipeline_state(Renderer_State *renderer_state);

U8 *get_property(Material *material, String name, ShaderDataType shader_datatype);

U32 index_of(Renderer_State *renderer_state, Texture *texture);
U32 index_of(Renderer_State *renderer_state, Material *material);
U32 index_of(Renderer_State *renderer_state, Static_Mesh *static_mesh);
U32 index_of(Renderer_State *renderer_state, Shader *shader);
U32 index_of(Renderer_State *renderer_state, Pipeline_State *pipeline_state);

U32 index_of(Renderer_State *renderer_state, const Texture *texture);
U32 index_of(Renderer_State *renderer_state, const Material *material);
U32 index_of(Renderer_State *renderer_state, const Static_Mesh *static_mesh);
U32 index_of(Renderer_State *renderer_state, const Shader *shader);
U32 index_of(Renderer_State *renderer_state, const Pipeline_State *pipeline_state);

S32 find_texture(Renderer_State *renderer_state, const String &name);
S32 find_material(Renderer_State *renderer_state, U64 hash);

inline glm::vec4 sRGB_to_linear(const glm::vec4 &color)
{
    return glm::pow(color, glm::vec4(GAMMA));
}

inline glm::vec4 linear_to_sRGB(const glm::vec4 &color)
{
    return glm::pow(color, glm::vec4(1.0f / GAMMA));
}

U32 get_size_of_shader_data_type(ShaderDataType shader_data_type);