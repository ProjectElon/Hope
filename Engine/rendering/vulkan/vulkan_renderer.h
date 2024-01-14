#pragma once

#include "vulkan_types.h"

bool vulkan_renderer_init(struct Engine *engine, struct Renderer_State *renderer_state);
void vulkan_renderer_deinit();

void vulkan_renderer_wait_for_gpu_to_finish_all_work();

void vulkan_renderer_on_resize(U32 width, U32 height);

bool vulkan_renderer_create_texture(Texture_Handle texture_handle, const Texture_Descriptor &descriptor);
void vulkan_renderer_destroy_texture(Texture_Handle texture_handle);

bool vulkan_renderer_create_sampler(Sampler_Handle sampler_handle, const Sampler_Descriptor &descriptor);
void vulkan_renderer_destroy_sampler(Sampler_Handle sampler_handle);

bool vulkan_renderer_create_buffer(Buffer_Handle buffer_handle, const Buffer_Descriptor &descriptor);
void vulkan_renderer_destroy_buffer(Buffer_Handle buffer_handle);

bool vulkan_renderer_create_shader(Shader_Handle shader_handle, const Shader_Descriptor &descriptor);
void vulkan_renderer_destroy_shader(Shader_Handle shader_handle);

bool vulkan_renderer_create_shader_group(Shader_Group_Handle shader_group_handle, const Shader_Group_Descriptor &descriptor);
void vulkan_renderer_destroy_shader_group(Shader_Group_Handle shader_group_handle);

bool vulkan_renderer_create_pipeline_state(Pipeline_State_Handle pipeline_state_handle, const Pipeline_State_Descriptor &descriptor);
void vulkan_renderer_destroy_pipeline_state(Pipeline_State_Handle pipeline_state_handle);

bool vulkan_renderer_create_bind_group_layout(Bind_Group_Layout_Handle bind_group_layout_handle, const Bind_Group_Layout_Descriptor &descriptor);
void vulkan_renderer_destroy_bind_group_layout(Bind_Group_Layout_Handle bind_group_layout_handle);

bool vulkan_renderer_create_bind_group(Bind_Group_Handle bind_group_handle, const Bind_Group_Descriptor &descriptor);
void vulkan_renderer_update_bind_group(Bind_Group_Handle bind_group_handle, const Array_View< Update_Binding_Descriptor > &update_binding_descriptors);
void vulkan_renderer_set_bind_groups(U32 first_bind_group, const Array_View< Bind_Group_Handle > &bind_group_handles);
void vulkan_renderer_destroy_bind_group(Bind_Group_Handle bind_group_handle);

bool vulkan_renderer_create_render_pass(Render_Pass_Handle render_pass_handle, const Render_Pass_Descriptor &descriptor);
void vulkan_renderer_begin_render_pass(Render_Pass_Handle render_pass_handle, Frame_Buffer_Handle frame_buffer_handle, const Array_View< Clear_Value > &clear_values);
void vulkan_renderer_end_render_pass(Render_Pass_Handle render_pass_handle);
void vulkan_renderer_destroy_render_pass(Render_Pass_Handle render_pass_handle);

bool vulkan_renderer_create_frame_buffer(Frame_Buffer_Handle frame_buffer_handle, const Frame_Buffer_Descriptor &descriptor);
void vulkan_renderer_destroy_frame_buffer(Frame_Buffer_Handle frame_buffer_handle);

bool vulkan_renderer_create_static_mesh(Static_Mesh_Handle static_mesh_handle, const Static_Mesh_Descriptor &descriptor);
void vulkan_renderer_destroy_static_mesh(Static_Mesh_Handle static_mesh_handle);

bool vulkan_renderer_create_semaphore(Semaphore_Handle semaphore_handle, const Renderer_Semaphore_Descriptor &descriptor);
U64 vulkan_renderer_get_semaphore_value(Semaphore_Handle semaphore_handle);
void vulkan_renderer_destroy_semaphore(Semaphore_Handle semaphore_handle);

void vulkan_renderer_set_vsync(bool enabled);

void vulkan_renderer_begin_frame(const struct Scene_Data *scene_data);

void vulkan_renderer_set_viewport(U32 width, U32 height);

void vulkan_renderer_set_vertex_buffers(const Array_View< Buffer_Handle > &vertex_buffer_handles, const Array_View< U64 > &offsets);
void vulkan_renderer_set_index_buffer(Buffer_Handle index_buffer_handle, U64 offset);

void vulkan_renderer_set_pipeline_state(Pipeline_State_Handle pipeline_state_handle);

void vulkan_renderer_draw_static_mesh(Static_Mesh_Handle static_mesh_handle, U32 first_instance);
void vulkan_renderer_draw_sub_mesh(Static_Mesh_Handle static_mesh_handle, U32 first_instance, U32 sub_mesh_index);

void vulkan_renderer_end_frame();

bool vulkan_renderer_init_imgui();
void vulkan_renderer_imgui_new_frame();
void vulkan_renderer_imgui_render();

Memory_Requirements vulkan_renderer_get_texture_memory_requirements(const Texture_Descriptor &descriptor);