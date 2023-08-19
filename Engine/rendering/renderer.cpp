#pragma warning(push, 0)
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "core/memory.h"
#include "core/engine.h"

static Free_List_Allocator *_transfer_allocator;
static Free_List_Allocator *_stbi_allocator;

#define STBI_MALLOC(sz) allocate(_stbi_allocator, sz, 0);
#define STBI_REALLOC(p, newsz) reallocate(_stbi_allocator, p, newsz, 0)
#define STBI_FREE(p) deallocate(_stbi_allocator, p)

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#pragma warning(pop)

#include "rendering/renderer.h"
#include "core/platform.h"

#if HE_OS_WINDOWS
#define HE_RHI_VULKAN
#endif

#ifdef HE_RHI_VULKAN
#include "rendering/vulkan/vulkan_renderer.h"
#endif

// todo(amer): to be removed...
#include <queue>
#include <filesystem>
namespace fs = std::filesystem;

bool request_renderer(RenderingAPI rendering_api,
                      Renderer *renderer)
{
    bool result = true;

    switch (rendering_api)
    {
#ifdef HE_RHI_VULKAN
        case RenderingAPI_Vulkan:
        {
            renderer->init = &vulkan_renderer_init;
            renderer->deinit = &vulkan_renderer_deinit;
            renderer->wait_for_gpu_to_finish_all_work = &vulkan_renderer_wait_for_gpu_to_finish_all_work;
            renderer->on_resize = &vulkan_renderer_on_resize;
            renderer->create_texture = &vulkan_renderer_create_texture;
            renderer->destroy_texture = &vulkan_renderer_destroy_texture;
            renderer->create_static_mesh = &vulkan_renderer_create_static_mesh;
            renderer->destroy_static_mesh = &vulkan_renderer_destroy_static_mesh;
            renderer->create_material = &vulkan_renderer_create_material;
            renderer->destroy_material = &vulkan_renderer_destroy_material;
            renderer->begin_frame = &vulkan_renderer_begin_frame;
            renderer->submit_static_mesh = &vulkan_renderer_submit_static_mesh;
            renderer->end_frame = &vulkan_renderer_end_frame;
            renderer->imgui_new_frame = &vulkan_renderer_imgui_new_frame;
        } break;
#endif

        default:
        {
            result = false;
        } break;
    }

    return result;
}

bool init_renderer_state(Engine *engine,
                         Renderer_State *renderer_state,
                         struct Memory_Arena *arena)
{
    renderer_state->engine = engine;
    renderer_state->textures = AllocateArray(arena, Texture, MAX_TEXTURE_COUNT);
    renderer_state->materials = AllocateArray(arena, Material, MAX_MATERIAL_COUNT);
    renderer_state->static_meshes = AllocateArray(arena, Static_Mesh, MAX_STATIC_MESH_COUNT);
    renderer_state->scene_nodes = AllocateArray(arena, Scene_Node, MAX_SCENE_NODE_COUNT);

    _transfer_allocator = renderer_state->transfer_allocator;
    _stbi_allocator = &engine->memory.free_list_allocator;
    return true;
}

Scene_Node*
add_child_scene_node(Renderer_State *renderer_state,
                     Scene_Node *parent)
{
    Assert(renderer_state->scene_node_count < MAX_SCENE_NODE_COUNT);
    Assert(parent);

    Scene_Node *node = &renderer_state->scene_nodes[renderer_state->scene_node_count++];
    node->parent = parent;

    if (parent->last_child)
    {
        parent->last_child->next_sibling = node;
        parent->last_child = node;
    }
    else
    {
        parent->first_child = parent->last_child = node;
    }

    return node;
}

internal_function Texture*
cgltf_load_texture(cgltf_texture_view *texture_view, const char *model_path, U32 model_path_without_file_name_length,
                   Renderer *renderer, Renderer_State *renderer_state)
{
    const cgltf_image *image = texture_view->texture->image;

    Texture *texture = nullptr;
    char texture_path[MAX_TEXTURE_NAME];

    if (texture_view->texture->image->uri)
    {
        char *uri = texture_view->texture->image->uri;
        sprintf(texture_path, "%.*s/%s",
                u64_to_u32(model_path_without_file_name_length),
                model_path, uri);
    }
    else
    {
        const char *extension_to_append = "";

        char *name = texture_view->texture->image->name;
        U32 name_length = u64_to_u32(strlen(name));
        Assert(name_length <= (MAX_TEXTURE_NAME - 1));

        S32 last_dot_index = -1;
        for (U32 char_index = 0; char_index < name_length; char_index++)
        {
            if (name[name_length - char_index - 1] == '.')
            {
                last_dot_index = char_index;
                break;
            }
        }

        // todo(amer): string utils
        char *extension = name + last_dot_index;
        if (strcmp(extension, ".png") != 0 &&
            strcmp(extension, ".jpg") != 0)
        {
            if (strcmp(image->mime_type, "image/png") == 0)
            {
                extension_to_append = ".png";
            }
            else if (strcmp(image->mime_type, "image/jpg") == 0)
            {
                extension_to_append = ".jpg";
            }
        }
        sprintf(texture_path, "%.*s/%.*s%s",
                u64_to_u32(model_path_without_file_name_length), model_path,
                name_length, name, extension_to_append);
    }

    U32 texture_path_length = u64_to_u32(strlen(texture_path));

    S32 texture_index = find_texture(renderer_state,
                                     texture_path,
                                     texture_path_length);
    if (texture_index == -1)
    {
        Assert(renderer_state->texture_count < MAX_TEXTURE_COUNT);
        texture = allocate_texture(renderer_state);
        strcpy(texture->name, texture_path);
        texture->name_length = texture_path_length;

        S32 texture_width;
        S32 texture_height;
        S32 texture_channels;

        stbi_uc *pixels = nullptr;

        if (!fs::exists(fs::path(texture_path)))
        {
            const auto *view = image->buffer_view;
            U8 *data_ptr = (U8*)view->buffer->data;
            U8 *image_data = data_ptr + view->offset;

            pixels = stbi_load_from_memory(image_data, u64_to_u32(view->size),
                                           &texture_width, &texture_height,
                                           &texture_channels, STBI_rgb_alpha);
        }
        else
        {
            pixels = stbi_load(texture_path,
                               &texture_width, &texture_height,
                               &texture_channels, STBI_rgb_alpha);
        }

        Assert(pixels);

        U64 data_size = texture_width * texture_height * sizeof(U32);
        U32 *data = AllocateArray(renderer_state->transfer_allocator, U32, data_size);
        memcpy(data, pixels, data_size);

        bool mipmapping = true;
        bool created = renderer->create_texture(texture,
                                                texture_width,
                                                texture_height,
                                                data, TextureFormat_RGBA, mipmapping);
        Assert(created);

        deallocate(renderer_state->transfer_allocator, data);
        stbi_image_free(pixels);
    }
    else
    {
        texture = &renderer_state->textures[texture_index];
    }

    return texture;
}

internal_function void* _cgltf_alloc(void* user, cgltf_size size)
{
    return allocate(_transfer_allocator, size, 8);
}

internal_function void _cgltf_free(void* user, void *ptr)
{
    deallocate(_transfer_allocator, ptr);
}

// note(amer): https://github.com/deccer/CMake-Glfw-OpenGL-Template/blob/main/src/Project/ProjectApplication.cpp
// thanks to this giga chad for the example
Scene_Node *load_model(const char *path, Renderer *renderer,
                       Renderer_State *renderer_state)
{
    Read_Entire_File_Result result =
        platform_begin_read_entire_file(path);

    if (!result.success)
    {
        return nullptr;
    }

    U8 *buffer = AllocateArray(renderer_state->transfer_allocator, U8, result.size);
    platform_end_read_entire_file(&result, buffer);

    U64 path_length = strlen(path);
    U64 path_without_file_name_length = 0;

    for (U64 char_index = path_length - 1; char_index >= 0; char_index--)
    {
        if (path[char_index] == '\\' || path[char_index] == '/')
        {
            path_without_file_name_length = char_index;
            break;
        }
    }

    cgltf_options options = {};
    options.memory.alloc_func = _cgltf_alloc;
    options.memory.free_func = _cgltf_free;

    cgltf_data *model_data = nullptr;

    if (cgltf_parse(&options, buffer, result.size, &model_data) != cgltf_result_success)
    {
        return nullptr;
    }

    if (cgltf_load_buffers(&options, model_data, path) != cgltf_result_success)
    {
        return nullptr;
    }

    for (U32 material_index = 0; material_index < model_data->materials_count; material_index++)
    {
        cgltf_material *material = &model_data->materials[material_index];
        U64 material_hash = (U64)material;

        Assert(renderer_state->material_count < MAX_MATERIAL_COUNT);
        Material* renderer_material = allocate_material(renderer_state);

        if (material->name)
        {
            renderer_material->name_length = u64_to_u32(strlen(material->name));
            Assert(renderer_material->name_length <= (MAX_MATERIAL_NAME - 1));
            strncpy(renderer_material->name, material->name, renderer_material->name_length);
        }
        renderer_material->hash = material_hash;

        Texture *albedo = nullptr;
        Texture *normal = nullptr;
        Texture *metallic_roughness = nullptr;

        if (material->has_pbr_metallic_roughness)
        {
            if (material->pbr_metallic_roughness.base_color_texture.texture)
            {
                albedo = cgltf_load_texture(&material->pbr_metallic_roughness.base_color_texture,
                                            path,
                                            u64_to_u32(path_without_file_name_length),
                                            renderer,
                                            renderer_state);
            }

            if (material->pbr_metallic_roughness.metallic_roughness_texture.texture)
            {
                metallic_roughness = cgltf_load_texture(&material->pbr_metallic_roughness.base_color_texture,
                                                        path,
                                                        u64_to_u32(path_without_file_name_length),
                                                        renderer,
                                                        renderer_state);
            }
            else
            {
                metallic_roughness = albedo;
            }
        }

        if (material->normal_texture.texture)
        {
            normal = cgltf_load_texture(&material->normal_texture,
                                        path,
                                        u64_to_u32(path_without_file_name_length),
                                        renderer,
                                        renderer_state);
        }
        else
        {
            // todo(amer): default normal texture doing this for now.
            normal = albedo;
        }

        Assert(albedo);
        Assert(normal);
        Assert(metallic_roughness);
        Assert(albedo != normal);
        Assert(normal != metallic_roughness);

        renderer->create_material(renderer_material,
                                  index_of(renderer_state, albedo),
                                  index_of(renderer_state, normal),
                                  index_of(renderer_state, metallic_roughness),
                                  1.0f,
                                  index_of(renderer_state, metallic_roughness),
                                  1.0f);
    }

    U32 position_count = 0;
    glm::vec3 *positions = nullptr;

    U32 normal_count = 0;
    glm::vec3 *normals = nullptr;

    U32 tangent_count = 0;
    glm::vec4 *tangents = nullptr;

    U32 uv_count = 0;
    glm::vec2 *uvs = nullptr;

    U32 index_count = 0;
    U16 *indices = nullptr;

    Scene_Node *root_scene_node = &renderer_state->scene_nodes[renderer_state->scene_node_count++];
    root_scene_node->parent = nullptr;
    root_scene_node->transform = glm::mat4(1.0f);

    struct Scene_Node_Bundle
    {
        cgltf_node *cgltf_node;
        Scene_Node *node;
    };

    for (U32 node_index = 0; node_index < model_data->nodes_count; node_index++)
    {
        std::queue< Scene_Node_Bundle > nodes;
        nodes.push({ &model_data->nodes[node_index], add_child_scene_node(renderer_state, root_scene_node) });
        while (!nodes.empty())
        {
            Scene_Node_Bundle &node_bundle = nodes.front();
            cgltf_node *node = node_bundle.cgltf_node;
            Scene_Node *scene_node = node_bundle.node;
            nodes.pop();

            cgltf_node_transform_world(node, glm::value_ptr(scene_node->transform));

            if (node->mesh)
            {
                scene_node->start_mesh_index = renderer_state->static_mesh_count;
                scene_node->static_mesh_count += u64_to_u32(node->mesh->primitives_count);

                for (U32 primitive_index = 0; primitive_index < node->mesh->primitives_count; primitive_index++)
                {
                    cgltf_primitive *primitive = &node->mesh->primitives[primitive_index];
                    Assert(primitive->material);
                    cgltf_material *material = primitive->material;

                    U64 material_hash = (U64)material;
                    S32 material_index = find_material(renderer_state, material_hash);
                    Assert(material_index != -1);

                    Static_Mesh *static_mesh = allocate_static_mesh(renderer_state);
                    static_mesh->material = &renderer_state->materials[material_index];

                    Assert(primitive->type == cgltf_primitive_type_triangles);

                    for (U32 attribute_index = 0; attribute_index < primitive->attributes_count; attribute_index++)
                    {
                        cgltf_attribute *attribute = &primitive->attributes[attribute_index];
                        Assert(attribute->type != cgltf_attribute_type_invalid);

                        const auto *accessor = attribute->data;
                        const auto *view = accessor->buffer_view;
                        U8 *data_ptr = (U8*)view->buffer->data;

                        switch (attribute->type)
                        {
                            case cgltf_attribute_type_position:
                            {
                                Assert(attribute->data->type == cgltf_type_vec3);
                                Assert(attribute->data->component_type == cgltf_component_type_r_32f);

                                position_count = u64_to_u32(attribute->data->count);
                                U64 stride = attribute->data->stride;
                                Assert(stride == sizeof(glm::vec3));
                                positions = (glm::vec3*)(data_ptr + view->offset + accessor->offset);
                            } break;

                            case cgltf_attribute_type_normal:
                            {
                                Assert(attribute->data->type == cgltf_type_vec3);
                                Assert(attribute->data->component_type == cgltf_component_type_r_32f);

                                normal_count = u64_to_u32(attribute->data->count);
                                U64 stride = attribute->data->stride;
                                Assert(stride == sizeof(glm::vec3));
                                normals = (glm::vec3*)(data_ptr + view->offset + accessor->offset);
                            } break;

                            case cgltf_attribute_type_tangent:
                            {
                                Assert(attribute->data->type == cgltf_type_vec4);
                                Assert(attribute->data->component_type == cgltf_component_type_r_32f);
                                tangent_count = u64_to_u32(attribute->data->count);
                                U64 stride = attribute->data->stride;
                                Assert(stride == sizeof(glm::vec4));
                                tangents = (glm::vec4*)(data_ptr + view->offset + accessor->offset);
                            } break;

                            case cgltf_attribute_type_texcoord:
                            {
                                Assert(attribute->data->type == cgltf_type_vec2);
                                Assert(attribute->data->component_type == cgltf_component_type_r_32f);

                                uv_count = u64_to_u32(attribute->data->count);
                                U64 stride = attribute->data->stride;
                                Assert(stride == sizeof(glm::vec2));
                                uvs = (glm::vec2*)(data_ptr + view->offset + accessor->offset);
                            } break;
                        }
                    }
                    
                    Assert(tangents);
                    Assert(position_count == normal_count);
                    Assert(position_count == uv_count);
                    Assert(position_count == tangent_count);

                    // note(amer): we only support u16 indices for now.
                    Assert(primitive->indices->type == cgltf_type_scalar);
                    Assert(primitive->indices->component_type == cgltf_component_type_r_16u);
                    Assert(primitive->indices->stride == sizeof(U16));

                    index_count = u64_to_u32(primitive->indices->count);
                    const auto *accessor = primitive->indices;
                    const auto *view = accessor->buffer_view;
                    U8 *data_ptr = (U8*)view->buffer->data;
                    indices = (U16*)(data_ptr + view->offset + accessor->offset);

                    U32 vertex_count = position_count;
                    Vertex *vertices = AllocateArray(renderer_state->transfer_allocator, Vertex, vertex_count);

                    for (U32 vertex_index = 0; vertex_index < vertex_count; vertex_index++)
                    {
                        const glm::vec4 &tanget = tangents[vertex_index];
                        Vertex *vertex = &vertices[vertex_index];
                        vertex->position = positions[vertex_index];
                        vertex->normal = normals[vertex_index];
                        vertex->tangent = tanget;
                        vertex->bi_tangent = glm::cross(vertex->normal, vertex->tangent) * tanget.w;
                        vertex->uv = uvs[vertex_index];
                    }

                    bool created = renderer->create_static_mesh(static_mesh, vertices, vertex_count,
                                                                indices, index_count);
                    Assert(created);

                    deallocate(renderer_state->transfer_allocator, vertices);
                }
            }

            for (U32 child_node_index = 0;
                 child_node_index < node->children_count;
                 child_node_index++)
            {
                nodes.push({ node->children[child_node_index], add_child_scene_node(renderer_state, scene_node) });
            }
        }
    }

    cgltf_free(model_data);
    deallocate(renderer_state->transfer_allocator, buffer);
    return root_scene_node;
}

void render_scene_node(Renderer *renderer, Renderer_State *renderer_state, Scene_Node *scene_node, glm::mat4 parent_transform)
{
    glm::mat4 transform = parent_transform * scene_node->transform;

    for (U32 static_mesh_index = 0;
         static_mesh_index < scene_node->static_mesh_count;
         static_mesh_index++)
    {
        Static_Mesh *static_mesh = renderer_state->static_meshes + scene_node->start_mesh_index + static_mesh_index;
        renderer->submit_static_mesh(renderer_state, static_mesh, transform);
    }

    for (Scene_Node *node = scene_node->first_child; node; node = node->next_sibling)
    {
        render_scene_node(renderer, renderer_state, node, transform);
    }
}


Texture *allocate_texture(Renderer_State *renderer_state)
{
    Assert(renderer_state->texture_count < MAX_TEXTURE_COUNT);
    U32 texture_index = renderer_state->texture_count++;
    return &renderer_state->textures[texture_index];
}

Material *allocate_material(Renderer_State *renderer_state)
{
    Assert(renderer_state->material_count < MAX_MATERIAL_COUNT);
    U32 material_index = renderer_state->material_count++;
    return &renderer_state->materials[material_index];
}

Static_Mesh *allocate_static_mesh(Renderer_State *renderer_state)
{
    Assert(renderer_state->static_mesh_count < MAX_STATIC_MESH_COUNT);
    U32 static_mesh_index = renderer_state->static_mesh_count++;
    return &renderer_state->static_meshes[static_mesh_index];
}

U32 index_of(Renderer_State *renderer_state, const Texture *texture)
{
    U64 index = texture - renderer_state->textures;
    Assert(index < MAX_TEXTURE_COUNT);
    return u64_to_u32(index);
}

U32 index_of(Renderer_State *renderer_state, const Material *material)
{
    U64 index = material - renderer_state->materials;
    Assert(index < MAX_MATERIAL_COUNT);
    return u64_to_u32(index);
}

U32 index_of(Renderer_State *renderer_state, const Static_Mesh *static_mesh)
{
    U64 index = static_mesh - renderer_state->static_meshes;
    Assert(index < MAX_STATIC_MESH_COUNT);
    return u64_to_u32(index);
}

U32 index_of(Renderer_State *renderer_state, Texture *texture)
{
    U64 index = texture - renderer_state->textures;
    Assert(index < MAX_TEXTURE_COUNT);
    return u64_to_u32(index);
}

U32 index_of(Renderer_State *renderer_state, Material *material)
{
    U64 index = material - renderer_state->materials;
    Assert(index < MAX_MATERIAL_COUNT);
    return u64_to_u32(index);
}

U32 index_of(Renderer_State *renderer_state, Static_Mesh *static_mesh)
{
    U64 index = static_mesh - renderer_state->static_meshes;
    Assert(index < MAX_STATIC_MESH_COUNT);
    return u64_to_u32(index);
}

S32 find_texture(Renderer_State *renderer_state, char *name, U32 length)
{
    for (U32 texture_index = 0; texture_index < renderer_state->texture_count; texture_index++)
    {
        Texture *texture = &renderer_state->textures[texture_index];
        if (texture->name_length == length && strncmp(texture->name, name, length) == 0)
        {
            return texture_index;
        }
    }
    return -1;
}

S32 find_material(Renderer_State *renderer_state, U64 hash)
{
    for (U32 material_index = 0; material_index < renderer_state->material_count; material_index++)
    {
        Material *material = &renderer_state->materials[material_index];
        if (material->hash == hash)
        {
            return material_index;
        }
    }
    return -1;
}