#pragma once

#include "core/defines.h"
#include "core/platform.h"
#include "core/job_system.h"

#include "rendering/renderer_types.h"
#include "rendering/camera.h"
#include "rendering/render_graph.h"

#include <atomic>

enum RenderingAPI
{
    RenderingAPI_Vulkan
};

#define HE_MAX_BUFFER_COUNT 4096
#define HE_MAX_TEXTURE_COUNT 4096
#define HE_MAX_SAMPLER_COUNT 4096
#define HE_MAX_MATERIAL_COUNT 4096
#define HE_MAX_RENDER_PASS_COUNT 4096
#define HE_MAX_FRAME_BUFFER_COUNT 4096
#define HE_MAX_STATIC_MESH_COUNT 4096
#define HE_MAX_SHADER_COUNT 4096
#define HE_MAX_SHADER_GROUP_COUNT 4096
#define HE_MAX_PIPELINE_STATE_COUNT 4096
#define HE_MAX_BIND_GROUP_LAYOUT_COUNT 4096
#define HE_MAX_BIND_GROUP_COUNT 4096
#define HE_MAX_SCENE_NODE_COUNT 4096

#define HE_RENDER_GRAPH 1

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

struct Renderer
{
    bool (*init)(struct Engine *engine, struct Renderer_State *renderer_state);
    void (*deinit)();

    void (*wait_for_gpu_to_finish_all_work)();

    void (*on_resize)(U32 width, U32 height);

    void (*begin_frame)(const Scene_Data *scene_data);
    void (*set_viewport)(U32 width, U32 height);
    void (*set_vertex_buffers)(const Array_View< Buffer_Handle > &vertex_buffer_handles, const Array_View< U64 > &offsets);
    void (*set_index_buffer)(Buffer_Handle index_buffer_handle, U64 offset);
    void (*set_pipeline_state)(Pipeline_State_Handle pipeline_state_handle);
    void (*draw_static_mesh)(Static_Mesh_Handle static_mesh_handle, U32 first_instance);
    void (*end_frame)();

    bool (*create_buffer)(Buffer_Handle buffer_handle, const Buffer_Descriptor &descriptor);
    void (*destroy_buffer)(Buffer_Handle buffer_handle);

    bool (*create_texture)(Texture_Handle texture_handle, const Texture_Descriptor &descriptor);
    void (*destroy_texture)(Texture_Handle texture_handle);

    bool (*create_sampler)(Sampler_Handle sampler_handle, const Sampler_Descriptor &descriptor);
    void (*destroy_sampler)(Sampler_Handle sampler_handle);

    bool (*create_shader)(Shader_Handle shader_handle, const Shader_Descriptor &descriptor);
    void (*destroy_shader)(Shader_Handle shader_handle);

    bool (*create_shader_group)(Shader_Group_Handle shader_group_handle, const Shader_Group_Descriptor &descriptor);
    void (*destroy_shader_group)(Shader_Group_Handle shader_group_handle);

    bool (*create_pipeline_state)(Pipeline_State_Handle pipeline_state_handle, const Pipeline_State_Descriptor &descriptor);
    void (*destroy_pipeline_state)(Pipeline_State_Handle pipeline_state_handle);

    bool (*create_bind_group_layout)(Bind_Group_Layout_Handle bind_group_layout_handle, const Bind_Group_Layout_Descriptor &descriptor);
    void (*destroy_bind_group_layout)(Bind_Group_Layout_Handle bind_group_layout_handle);

    bool (*create_bind_group)(Bind_Group_Handle bind_group_handle, const Bind_Group_Descriptor &descriptor);
    void (*update_bind_group)(Bind_Group_Handle bind_group_handle, const Array_View< Update_Binding_Descriptor > &update_binding_descriptors);
    void (*set_bind_groups)(U32 first_bind_group, const Array_View< Bind_Group_Handle > &bind_group_handles);
    void (*destroy_bind_group)(Bind_Group_Handle bind_group_handle);

    bool (*create_render_pass)(Render_Pass_Handle render_pass_handle, const Render_Pass_Descriptor &descriptor);
    void (*begin_render_pass)(Render_Pass_Handle render_pass_handle, Frame_Buffer_Handle frame_buffer_handle, const Array_View< Clear_Value > &clear_views);
    void (*end_render_pass)(Render_Pass_Handle render_pass_handle);
    void (*destroy_render_pass)(Render_Pass_Handle render_pass_handle);

    bool (*create_frame_buffer)(Frame_Buffer_Handle frame_buffer_handle, const Frame_Buffer_Descriptor &descriptor);
    void (*destroy_frame_buffer)(Frame_Buffer_Handle frame_buffer_handle);

    bool (*create_static_mesh)(Static_Mesh_Handle static_mesh_handle, const Static_Mesh_Descriptor &descriptor);
    void (*destroy_static_mesh)(Static_Mesh_Handle static_mesh_handle);

    bool (*init_imgui)();
    void (*imgui_new_frame)();
    void (*imgui_render)();
};

bool request_renderer(RenderingAPI rendering_api, Renderer *renderer);

struct Renderer_State
{
    struct Engine *engine;
    
    bool imgui_docking;
    
    Memory_Arena arena;
    Temprary_Memory_Arena frame_arena;

    Renderer renderer;

    U32 back_buffer_width;
    U32 back_buffer_height;

    Resource_Pool< Buffer > buffers;
    Resource_Pool< Texture > textures;
    Resource_Pool< Sampler > samplers;
    Resource_Pool< Shader > shaders;
    Resource_Pool< Shader_Group > shader_groups;
    Resource_Pool< Pipeline_State > pipeline_states;
    Resource_Pool< Bind_Group_Layout > bind_group_layouts;
    Resource_Pool< Bind_Group > bind_groups;
    Resource_Pool< Render_Pass > render_passes;
    Resource_Pool< Frame_Buffer > frame_buffers;
    Resource_Pool< Material > materials;
    Resource_Pool< Static_Mesh > static_meshes;

    std::atomic< U32 > scene_node_count;
    Scene_Node *scene_nodes;
    Scene_Node *root_scene_node;

    F32 gamma;
    U32 sample_count;
    U32 anisotropic_filtering;

    Shader_Handle mesh_vertex_shader;
    Shader_Handle mesh_fragment_shader;
    Shader_Group_Handle mesh_shader_group;
    Pipeline_State_Handle mesh_pipeline;

    Bind_Group_Handle per_frame_bind_groups[HE_MAX_FRAMES_IN_FLIGHT];
    Bind_Group_Handle per_render_pass_bind_groups[HE_MAX_FRAMES_IN_FLIGHT];

    Texture_Handle color_attachments[HE_MAX_FRAMES_IN_FLIGHT];
    Texture_Handle depth_attachments[HE_MAX_FRAMES_IN_FLIGHT];
    Texture_Handle resolve_color_attachments[HE_MAX_FRAMES_IN_FLIGHT];

    Frame_Buffer_Handle world_frame_buffers[HE_MAX_FRAMES_IN_FLIGHT];
    Frame_Buffer_Handle ui_frame_buffers[HE_MAX_FRAMES_IN_FLIGHT];

    Render_Pass_Handle world_render_pass;
    Render_Pass_Handle ui_render_pass;

    Texture_Handle white_pixel_texture;
    Texture_Handle normal_pixel_texture;

    Sampler_Handle default_sampler;

    Buffer_Handle globals_uniform_buffers[HE_MAX_FRAMES_IN_FLIGHT];

    Buffer_Handle object_data_storage_buffers[HE_MAX_FRAMES_IN_FLIGHT];
    Object_Data *object_data_base;
    U32 object_data_count;

    Buffer_Handle transfer_buffer;

    U64 max_vertex_count;
    U64 vertex_count;
    Buffer_Handle position_buffer;
    Buffer_Handle normal_buffer;
    Buffer_Handle uv_buffer;
    Buffer_Handle tangent_buffer;

    Buffer_Handle index_buffer;
    U64 index_offset;

    Scene_Data scene_data;

    Free_List_Allocator transfer_allocator;

    U32 frames_in_flight;
    U32 current_frame_in_flight_index;

    Mutex render_commands_mutex;

    Render_Graph render_graph;
};

bool pre_init_renderer_state(struct Engine *engine);

bool init_renderer_state(struct Engine *engine);

void deinit_renderer_state();

void invalidate_render_entities();

Scene_Node *add_child_scene_node(Scene_Node *parent);

bool load_model(Scene_Node *root_scene_node, const String &path, Memory_Arena *arena);

Scene_Node* load_model_threaded(const String &path);

void render_scene_node(Scene_Node *scene_node, const glm::mat4 &transform);

U8 *get_property(Material *material, const String &name, Shader_Data_Type data_type);

glm::vec4 srgb_to_linear(const glm::vec4 &color);
glm::vec4 linear_to_srgb(const glm::vec4 &color);

void renderer_on_resize(U32 width, U32 height);

glm::vec2 renderer_get_viewport();

Scene_Data *renderer_get_scene_data();
void renderer_wait_for_gpu_to_finish_all_work();

//
// Buffers
//

Buffer_Handle renderer_create_buffer(const Buffer_Descriptor &descriptor);
Buffer *renderer_get_buffer(Buffer_Handle buffer_handle);
void renderer_destroy_buffer(Buffer_Handle &buffer_handle);

//
// Textures
//

Texture_Handle renderer_create_texture(const Texture_Descriptor &descriptor);
Texture* renderer_get_texture(Texture_Handle texture_handle);
void renderer_destroy_texture(Texture_Handle &texture_handle);

//
// Samplers
//

Sampler_Handle renderer_create_sampler(const Sampler_Descriptor &descriptor);
Sampler* renderer_get_sampler(Sampler_Handle sampler_handle);
void renderer_destroy_sampler(Sampler_Handle &sampler_handle);

//
// Shaders
//

Shader_Handle renderer_create_shader(const Shader_Descriptor &descriptor);
Shader* renderer_get_shader(Shader_Handle shader_handle);
void renderer_destroy_shader(Shader_Handle &shader_handle);

//
// Shader Groups
//

Shader_Group_Handle renderer_create_shader_group(const Shader_Group_Descriptor &descriptor);
Shader_Group* renderer_get_shader_group(Shader_Group_Handle shader_group_handle);
void renderer_destroy_shader_group(Shader_Group_Handle &shader_group_handle);

//
// Bind Group Layouts
//

Bind_Group_Layout_Handle renderer_create_bind_group_layout(const Bind_Group_Layout_Descriptor &descriptor);
Bind_Group_Layout* renderer_get_bind_group_layout(Bind_Group_Layout_Handle bind_group_layout_handle);
void renderer_destroy_bind_group_layout(Bind_Group_Layout_Handle &bind_group_layout_handle);

//
// Bind Groups
//

Bind_Group_Handle renderer_create_bind_group(const Bind_Group_Descriptor &descriptor);
Bind_Group* renderer_get_bind_Group(Bind_Group_Handle bind_group_handle);
void renderer_destroy_bind_group(Bind_Group_Handle &bind_group_handle);

//
// Pipeline States
//

Pipeline_State_Handle renderer_create_pipeline_state(const Pipeline_State_Descriptor &descriptor);
Pipeline_State* renderer_get_pipeline_state(Pipeline_State_Handle pipeline_state_handle);
void renderer_destroy_pipeline_state(Pipeline_State_Handle &pipeline_state_handle);

//
// Render Passes
//

Render_Pass_Handle renderer_create_render_pass(const Render_Pass_Descriptor &descriptor);
Render_Pass* renderer_get_render_pass(Render_Pass_Handle render_pass_handle);
void renderer_destroy_render_pass(Render_Pass_Handle &render_pass_handle);

//
// Frame Buffers
//

Frame_Buffer_Handle renderer_create_frame_buffer(const Frame_Buffer_Descriptor &descriptor);
Frame_Buffer* renderer_get_frame_buffer(Frame_Buffer_Handle frame_buffer_handle);
void renderer_destroy_frame_buffer(Frame_Buffer_Handle &frame_buffer_handle);

//
// Static Meshes
//
Static_Mesh_Handle renderer_create_static_mesh(const Static_Mesh_Descriptor &descriptor);
Static_Mesh* renderer_get_static_mesh(Static_Mesh_Handle static_mesh_handle);
void renderer_destroy_static_mesh(Static_Mesh_Handle &static_mesh_handle);

//
// Materials
//
Material_Handle renderer_create_material(const Material_Descriptor &descriptor);
Material* renderer_get_material(Material_Handle material_handle);
void renderer_destroy_material(Material_Handle &material_handle);

void init_imgui(Engine *engine);
void imgui_new_frame();

// todo(amer): Temprary
Renderer *get_renderer();
Renderer_State *get_renderer_state();

U32 get_size_of_shader_data_type(Shader_Data_Type data_type);