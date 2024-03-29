#include "assets/texture_importer.h"
#include "core/memory.h"
#include "core/file_system.h"
#include "core/logging.h"

#include "rendering/renderer.h"

#if 0
extern Free_List_Allocator *stbi_image_allocator;

#define STBI_MALLOC(sz) allocate(stbi_image_allocator, sz, 4)
#define STBI_REALLOC(p, newsz) reallocate(stbi_image_allocator, p, newsz, 4)
#define STBI_FREE(p) deallocate(stbi_image_allocator, p)

#endif

#pragma warning(push, 0)

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#pragma warning(pop)

Load_Asset_Result load_texture(String path)
{
    Temprary_Memory_Arena_Janitor scratch_memory = make_scratch_memory_janitor();

    Render_Context render_context = get_render_context();
    Renderer_State *renderer_state = render_context.renderer_state;

    Read_Entire_File_Result file_result = read_entire_file(path, to_allocator(scratch_memory.arena));
    if (!file_result.success)
    {
        HE_LOG(Assets, Error, "load_texture -- failed to read file: %.*s\n", HE_EXPAND_STRING(path));
        return {};
    }
    
    S32 width;
    S32 height;
    S32 channels;
    stbi_uc *pixels = stbi_load_from_memory(file_result.data, u64_to_u32(file_result.size), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        HE_LOG(Assets, Error, "load_texture -- stbi_load_from_memory -- failed to load texture asset: %.*s\n", HE_EXPAND_STRING(path));
        return {};
    }

    U32 *data = HE_ALLOCATE_ARRAY(&renderer_state->transfer_allocator, U32, width * height);
    copy_memory(data, pixels, width * height * sizeof(U32));

    stbi_image_free(pixels);

    void *data_array[] = { data };

    Texture_Descriptor texture_descriptor =
    {
        .width = (U32)width,
        .height = (U32)height,
        .format = Texture_Format::R8G8B8A8_UNORM,
        .data_array = to_array_view(data_array),
        .mipmapping = true,
        .sample_count = 1,
    };

    Texture_Handle texture_handle = renderer_create_texture(texture_descriptor);
    if (!is_valid_handle(&renderer_state->textures, texture_handle))
    {
        HE_LOG(Assets, Error, "load_texture -- renderer_create_texture -- failed to load texture asset: %.*s\n", HE_EXPAND_STRING(path));
        return {};
    }
    return { .success = true, .index = texture_handle.index, .generation = texture_handle.generation };
}

void unload_texture(Load_Asset_Result load_result)
{
    Texture_Handle texture_handle = { .index = load_result.index, .generation = load_result.generation };
    renderer_destroy_texture(texture_handle);
}