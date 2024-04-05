#include "assets/model_importer.h"

#include "core/file_system.h"
#include "core/logging.h"
#include "core/memory.h"

#include "rendering/renderer.h"

#pragma warning(push, 0)

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#pragma warning(pop)

static void* cgltf_alloc(void *user, cgltf_size size)
{
    return allocate((Free_List_Allocator *)user, size, 16);
}

static void cgltf_free(void *user, void *ptr)
{
    deallocate((Free_List_Allocator *)user, ptr);
}

static Asset_Handle get_texture_asset_handle(String model_relative_path, const cgltf_image *image)
{
    Temprary_Memory_Arena_Janitor scratch_memory = make_scratch_memory_janitor();

    String texture_name = {};
    String parent_path = get_parent_path(model_relative_path);

    if (image->uri)
    {
        texture_name = HE_STRING(image->uri);
    }
    else if (image->name)
    {
        texture_name = HE_STRING(image->name);
    }
    else
    {
        return { .uuid = 0 };
    }

    String texture_path = format_string(scratch_memory.arena, "%.*s/%.*s", HE_EXPAND_STRING(parent_path), HE_EXPAND_STRING(texture_name));
    Asset_Handle asset_handle = import_asset(texture_path);
    return asset_handle;
};

void on_import_model(Asset_Handle asset_handle)
{
    Temprary_Memory_Arena_Janitor scratch_memory = make_scratch_memory_janitor();
    Free_List_Allocator *allocator = get_general_purpose_allocator();

    const Asset_Registry_Entry &entry = get_asset_registry_entry(asset_handle);
    String path = format_string(scratch_memory.arena, "%.*s/%.*s", HE_EXPAND_STRING(get_asset_path()), HE_EXPAND_STRING(entry.path));

    Read_Entire_File_Result file_result = read_entire_file(path, to_allocator(allocator));

    cgltf_options options = {};
    options.memory.user_data = allocator;
    options.memory.alloc_func = cgltf_alloc;
    options.memory.free_func = cgltf_free;

    cgltf_data *model_data = nullptr;

    if (cgltf_parse(&options, file_result.data, file_result.size, &model_data) != cgltf_result_success)
    {
        HE_LOG(Resource, Fetal, "on import model -- cgltf -- unable to parse asset file: %.*s\n", HE_EXPAND_STRING(path));
        return;
    }

    HE_DEFER { cgltf_free(model_data); };

    Asset_Handle opaque_pbr_shader_asset = import_asset(HE_STRING_LITERAL("opaque_pbr.glsl"));

    for (U32 material_index = 0; material_index < model_data->materials_count; material_index++)
    {
        cgltf_material *material = &model_data->materials[material_index];
        String material_name = {};

        if (material->name)
        {
            material_name = HE_STRING(material->name);
        }
        else
        {
            material_name = format_string(scratch_memory.arena, "material_%d", material_index);
        }

        String material_path = format_string(scratch_memory.arena, "@%llu-%llu/%.*s.hamaterial", asset_handle.uuid, (U64)material_index, HE_EXPAND_STRING(material_name));
        Asset_Handle asset = import_asset(material_path);
        set_parent(asset, opaque_pbr_shader_asset);
    }

    for (U32 static_mesh_index = 0; static_mesh_index < model_data->meshes_count; static_mesh_index++)
    {
        cgltf_mesh *static_mesh = &model_data->meshes[static_mesh_index];

        String static_mesh_name;

        if (static_mesh->name)
        {
            static_mesh_name = HE_STRING(static_mesh->name);
        }
        else
        {
            static_mesh_name = format_string(scratch_memory.arena, "static_mesh_%d", static_mesh_index);
        }

        String static_mesh_path = format_string(scratch_memory.arena, "@%llu-%llu/%.*s.hastaticmesh", asset_handle.uuid, (U64)static_mesh_index, HE_EXPAND_STRING(static_mesh_name));
        Asset_Handle asset = import_asset(static_mesh_path);
    }
}

Load_Asset_Result load_model(String path, const Embeded_Asset_Params *params)
{
    Free_List_Allocator *allocator = get_general_purpose_allocator();
    Temprary_Memory_Arena_Janitor scratch_memory = make_scratch_memory_janitor();

    String asset_path = get_asset_path();
    String relative_path = sub_string(path, asset_path.count + 1);

    Render_Context render_context = get_render_context();
    Renderer_State *renderer_state = render_context.renderer_state;

    Read_Entire_File_Result file_result = read_entire_file(path, to_allocator(allocator));

    cgltf_options options = {};
    options.memory.user_data = allocator;
    options.memory.alloc_func = cgltf_alloc;
    options.memory.free_func = cgltf_free;

    cgltf_data *model_data = nullptr;

    if (cgltf_parse(&options, file_result.data, file_result.size, &model_data) != cgltf_result_success)
    {
        HE_LOG(Resource, Fetal, "load_model -- cgltf -- unable to parse asset file: %.*s\n", HE_EXPAND_STRING(path));
        return {};
    }

    HE_DEFER { cgltf_free(model_data); };

    if (cgltf_load_buffers(&options, model_data, path.data) != cgltf_result_success)
    {
        HE_LOG(Resource, Fetal, "load_model -- cgltf -- unable to load buffers from asset file: %.*s\n", HE_EXPAND_STRING(path));
        return {};
    }

    bool embeded_material = false;
    bool embeded_static_mesh = false;

    if (params)
    {
        const Asset_Info *info = get_asset_info(params->type_info_index);
        embeded_material = info->name == HE_STRING_LITERAL("material");
        embeded_static_mesh = info->name == HE_STRING_LITERAL("static_mesh");
    }

    Asset_Handle opaque_pbr_shader_asset = import_asset(HE_STRING_LITERAL("opaque_pbr.glsl"));
    if (!is_asset_loaded(opaque_pbr_shader_asset))
    {
        HE_LOG(Resource, Fetal, "load_model -- cgltf -- unable to load model asset file: %.*s --> parent asset failed to load\n", HE_EXPAND_STRING(path));
        return {};
    }

    Shader_Handle opaque_pbr_shader = get_asset_handle_as<Shader>(opaque_pbr_shader_asset);
    Material_Handle *materials = HE_ALLOCATE_ARRAY(allocator, Material_Handle, model_data->materials_count);
    for (U32 material_index = 0; material_index < model_data->materials_count; material_index++)
    {
        cgltf_material *material = &model_data->materials[material_index];
        
        String material_name = {};

        if (material->name)
        {
            material_name = HE_STRING(material->name);
        }
        else
        {
            material_name = format_string(scratch_memory.arena, "material_%d", material_index);
        }

        Asset_Handle albedo_texture = {};
        Asset_Handle roughness_metallic_texture = {};
        Asset_Handle occlusion_texture = {};
        Asset_Handle normal_texture = {};

        if (material->has_pbr_metallic_roughness)
        {
            if (material->pbr_metallic_roughness.base_color_texture.texture)
            {
                const cgltf_image *image = material->pbr_metallic_roughness.base_color_texture.texture->image;
                albedo_texture = get_texture_asset_handle(relative_path, image);
            }

            if (material->pbr_metallic_roughness.metallic_roughness_texture.texture)
            {
                const cgltf_image *image = material->pbr_metallic_roughness.metallic_roughness_texture.texture->image;
                roughness_metallic_texture = get_texture_asset_handle(relative_path, image);
            }
        }

        if (material->normal_texture.texture)
        {
            const cgltf_image *image = material->normal_texture.texture->image;
            normal_texture = get_texture_asset_handle(relative_path, image);
        }

        if (material->occlusion_texture.texture)
        {
            const cgltf_image *image = material->occlusion_texture.texture->image;
            occlusion_texture = get_texture_asset_handle(relative_path, image);
        }

        String render_pass_name = HE_STRING_LITERAL("opaque");
        Render_Pass_Handle render_pass = get_render_pass(&renderer_state->render_graph, render_pass_name);

        Pipeline_State_Settings settings =
        {
            .cull_mode = material->double_sided ? Cull_Mode::NONE : Cull_Mode::BACK,
            .front_face = Front_Face::COUNTER_CLOCKWISE,
            .fill_mode = Fill_Mode::SOLID,
            .depth_testing = true,
            .sample_shading = true,
        };

        Pipeline_State_Descriptor pipeline_state_descriptor =
        {
            .settings = settings,
            .shader = opaque_pbr_shader,
            .render_pass = render_pass,
        };

        Pipeline_State_Handle pipeline_state_handle = renderer_create_pipeline_state(pipeline_state_descriptor);

        Material_Descriptor material_descriptor =
        {
            .name = material_name,
            .pipeline_state_handle = pipeline_state_handle
        };

        Material_Handle material_handle = renderer_create_material(material_descriptor);

        set_property(material_handle, HE_STRING_LITERAL("albedo_texture"), { .u64 = albedo_texture.uuid });
        set_property(material_handle, HE_STRING_LITERAL("albedo_color"), { .v3 = *(glm::vec3 *)material->pbr_metallic_roughness.base_color_factor });
        set_property(material_handle, HE_STRING_LITERAL("normal_texture"), { .u64 = normal_texture.uuid });
        set_property(material_handle, HE_STRING_LITERAL("roughness_metallic_texture"), { .u64 = roughness_metallic_texture.uuid });
        set_property(material_handle, HE_STRING_LITERAL("roughness_factor"), { .f32 = material->pbr_metallic_roughness.roughness_factor });
        set_property(material_handle, HE_STRING_LITERAL("metallic_factor"), { .f32 = material->pbr_metallic_roughness.metallic_factor });
        set_property(material_handle, HE_STRING_LITERAL("occlusion_texture"), { .u64 = occlusion_texture.uuid });

        materials[material_index] = material_handle;

        if (embeded_material && params->data_id == material_index)
        {
            return { .success = true, .index = material_handle.index, .generation = material_handle.generation };
        }
    }

    Static_Mesh_Handle *static_meshes = HE_ALLOCATE_ARRAY(allocator, Static_Mesh_Handle, model_data->meshes_count);

    for (U32 static_mesh_index = 0; static_mesh_index < model_data->meshes_count; static_mesh_index++)
    {
        cgltf_mesh *static_mesh = &model_data->meshes[static_mesh_index];

        String static_mesh_name;

        if (static_mesh->name)
        {
            static_mesh_name = HE_STRING(static_mesh->name);
        }
        else
        {
            static_mesh_name = format_string(scratch_memory.arena, "static_mesh_%d", static_mesh_index);
        }

        U64 total_vertex_count = 0;
        U64 total_index_count = 0;

        Dynamic_Array< Sub_Mesh > sub_meshes;
        init(&sub_meshes, u64_to_u32(static_mesh->primitives_count), u64_to_u32(static_mesh->primitives_count));

        for (U32 sub_mesh_index = 0; sub_mesh_index < (U32)static_mesh->primitives_count; sub_mesh_index++)
        {
            cgltf_primitive *primitive = &static_mesh->primitives[sub_mesh_index];
            HE_ASSERT(primitive->type == cgltf_primitive_type_triangles);

            HE_ASSERT(primitive->indices->type == cgltf_type_scalar);
            HE_ASSERT(primitive->indices->component_type == cgltf_component_type_r_16u);
            HE_ASSERT(primitive->indices->stride == sizeof(U16));

            sub_meshes[sub_mesh_index].vertex_offset = u64_to_u32(total_vertex_count);
            sub_meshes[sub_mesh_index].index_offset = u64_to_u32(total_index_count);

            total_index_count += primitive->indices->count;
            sub_meshes[sub_mesh_index].index_count = u64_to_u32(primitive->indices->count);
            S64 material_index = primitive->material - model_data->materials;
            HE_ASSERT(material_index >= 0 && material_index <= (S64)model_data->materials_count);
            sub_meshes[sub_mesh_index].material = materials[material_index];

            for (U32 attribute_index = 0; attribute_index < primitive->attributes_count; attribute_index++)
            {
                cgltf_attribute *attribute = &primitive->attributes[attribute_index];
                switch (attribute->type)
                {
                    case cgltf_attribute_type_position:
                    {
                        HE_ASSERT(attribute->data->type == cgltf_type_vec3);
                        HE_ASSERT(attribute->data->component_type == cgltf_component_type_r_32f);
                        U64 stride = attribute->data->stride;
                        HE_ASSERT(stride == sizeof(glm::vec3));
                        total_vertex_count += attribute->data->count;
                        sub_meshes[sub_mesh_index].vertex_count = u64_to_u32(attribute->data->count);
                    } break;

                    case cgltf_attribute_type_normal:
                    {
                        HE_ASSERT(attribute->data->type == cgltf_type_vec3);
                        HE_ASSERT(attribute->data->component_type == cgltf_component_type_r_32f);

                        U64 stride = attribute->data->stride;
                        HE_ASSERT(stride == sizeof(glm::vec3));
                    } break;

                    case cgltf_attribute_type_texcoord:
                    {
                        HE_ASSERT(attribute->data->type == cgltf_type_vec2);
                        HE_ASSERT(attribute->data->component_type == cgltf_component_type_r_32f);

                        U64 stride = attribute->data->stride;
                        HE_ASSERT(stride == sizeof(glm::vec2));
                    } break;

                    case cgltf_attribute_type_tangent:
                    {
                        HE_ASSERT(attribute->data->type == cgltf_type_vec4);
                        HE_ASSERT(attribute->data->component_type == cgltf_component_type_r_32f);

                        U64 stride = attribute->data->stride;
                        HE_ASSERT(stride == sizeof(glm::vec4));
                    } break;
                }
            }
        }

        U64 index_size = sizeof(U16) * total_index_count;
        U64 vertex_size = (sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec4)) * total_vertex_count;
        U64 total_size = index_size + vertex_size;
        U8 *static_mesh_data = HE_ALLOCATE_ARRAY(&renderer_state->transfer_allocator, U8, total_size);

        U16 *indices = (U16 *)static_mesh_data;

        U8 *vertex_data = static_mesh_data + index_size;
        glm::vec3 *positions = (glm::vec3 *)vertex_data;
        glm::vec3 *normals = (glm::vec3 *)(vertex_data + sizeof(glm::vec3) * total_vertex_count);
        glm::vec2 *uvs = (glm::vec2 *)(vertex_data + (sizeof(glm::vec3) + sizeof(glm::vec3)) * total_vertex_count);
        glm::vec4 *tangents = (glm::vec4 *)(vertex_data + (sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3)) * total_vertex_count);

        for (U32 sub_mesh_index = 0; sub_mesh_index < (U32)static_mesh->primitives_count; sub_mesh_index++)
        {
            cgltf_primitive *primitive = &static_mesh->primitives[sub_mesh_index];
            
            const auto *accessor = primitive->indices;
            const auto *view = accessor->buffer_view;
            U8 *data = (U8 *)view->buffer->data + view->offset + accessor->offset;
            copy_memory(indices + sub_meshes[sub_mesh_index].index_offset, data, primitive->indices->count * sizeof(U16));
            
            for (U32 attribute_index = 0; attribute_index < primitive->attributes_count; attribute_index++)
            {
                cgltf_attribute *attribute = &primitive->attributes[attribute_index];
                switch (attribute->type)
                {
                    case cgltf_attribute_type_position:
                    {
                        const auto *accessor = attribute->data;
                        const auto *view = accessor->buffer_view;
                        U8 *data_ptr = (U8 *)view->buffer->data;
                        U64 element_size = attribute->data->stride;
                        U64 element_count = attribute->data->count;
                        U8 *data = data_ptr + view->offset + accessor->offset;
                        copy_memory(positions + sub_meshes[sub_mesh_index].vertex_offset, data, element_size * element_count);
                    } break;

                    case cgltf_attribute_type_normal:
                    {
                        const auto *accessor = attribute->data;
                        const auto *view = accessor->buffer_view;
                        U8 *data_ptr = (U8 *)view->buffer->data;
                        U64 element_size = attribute->data->stride;
                        U64 element_count = attribute->data->count;
                        U8 *data = data_ptr + view->offset + accessor->offset;
                        copy_memory(normals + sub_meshes[sub_mesh_index].vertex_offset, data, element_size * element_count);
                    } break;


                    case cgltf_attribute_type_texcoord:
                    {
                        const auto *accessor = attribute->data;
                        const auto *view = accessor->buffer_view;
                        U8 *data_ptr = (U8 *)view->buffer->data;
                        U64 element_size = attribute->data->stride;
                        U64 element_count = attribute->data->count;
                        U8 *data = data_ptr + view->offset + accessor->offset;
                        copy_memory(uvs + sub_meshes[sub_mesh_index].vertex_offset, data, element_size * element_count);
                    } break;

                    case cgltf_attribute_type_tangent:
                    {
                        const auto *accessor = attribute->data;
                        const auto *view = accessor->buffer_view;
                        U8 *data_ptr = (U8 *)view->buffer->data;
                        U64 element_size = attribute->data->stride;
                        U64 element_count = attribute->data->count;
                        U8 *data = data_ptr + view->offset + accessor->offset;
                        copy_memory(tangents + sub_meshes[sub_mesh_index].vertex_offset, data, element_size * element_count);
                    } break;
                }
            }
        }

        void *data_array[] = { static_mesh_data };

        Static_Mesh_Descriptor static_mesh_descriptor =
        {
            .name = copy_string(static_mesh_name, to_allocator(allocator)),
            .data_array = to_array_view(data_array),
            
            .indices = indices,
            .index_count = u64_to_u32(total_index_count),
    
            .vertex_count = u64_to_u32(total_vertex_count),
            .positions = positions,
            .normals = normals,
            .uvs = uvs,
            .tangents = tangents,

            .sub_meshes = sub_meshes
        };

        Static_Mesh_Handle static_mesh_handle = renderer_create_static_mesh(static_mesh_descriptor);
        static_meshes[static_mesh_index] = static_mesh_handle;

        if (embeded_static_mesh && params->data_id == static_mesh_index)
        {
            return { .success = true, .index = static_mesh_handle.index, .generation = static_mesh_handle.generation };
        }
    }

    cgltf_scene *scene = &model_data->scenes[0];
    
    Model *model = HE_ALLOCATE(allocator, Model);
    model->name = copy_string(get_name(path), to_allocator(allocator));
    
    model->static_mesh_count = u64_to_u32(model_data->meshes_count);
    model->static_meshes = static_meshes;

    model->material_count = u64_to_u32(model_data->materials_count);
    model->materials = materials;
    
    Model_Node *nodes = HE_ALLOCATE_ARRAY(allocator, Model_Node, scene->nodes_count);

    model->node_count = u64_to_u32(scene->nodes_count);
    model->nodes = nodes;

    for (U32 node_index = 0; node_index < scene->nodes_count; node_index++)
    {
        cgltf_node *node = scene->nodes[node_index];

        String node_name = {};
        
        if (node->name)
        {
            node_name = HE_STRING(node->name);
        }
        else
        {
            node_name = format_string(scratch_memory.arena, "node_%u", node_index);
        }

        S32 parent_index = node->parent ? (S32)(node->parent - model_data->nodes) : -1;

        glm::quat rotation = { node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2] };

        Transform transform =
        {
            .position = *(glm::vec3*)&node->translation,
            .rotation = rotation,
            .euler_angles = glm::degrees(glm::eulerAngles(rotation)),
            .scale = *(glm::vec3*)&node->scale
        };

        S64 mesh_index = (S64)(node->mesh - model_data->meshes);
        HE_ASSERT(mesh_index >= 0 && (S64)model_data->meshes_count);

        Model_Node *model_node = &nodes[node_index];
        model_node->name = copy_string(node_name, to_allocator(allocator));
        model_node->parent_index = parent_index;
        model_node->transform = transform;
        model_node->static_mesh = static_meshes[mesh_index];
    }

    return { .success = true, .data = model, .size = sizeof(Model) };
}

void unload_model(Load_Asset_Result load_result)
{
    HE_ASSERT(sizeof(Model) == load_result.size);
    
    Free_List_Allocator *allocator = get_general_purpose_allocator();
    Model *model = (Model *)load_result.data;
    deallocate(allocator, (void *)model->name.data);

    for (U32 i = 0; i < model->material_count; i++)
    {
        renderer_destroy_material(model->materials[i]);
    }

    for (U32 i = 0; i < model->static_mesh_count; i++)
    {
        renderer_destroy_static_mesh(model->static_meshes[i]);
    }

    for (U32 i = 0; i < model->node_count; i++)
    {
        Model_Node *node = &model->nodes[i];
        deallocate(allocator, (void *)node->name.data);
    }

    deallocate(allocator, (void *)model->nodes);
    deallocate(allocator, model);
}