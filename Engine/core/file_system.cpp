#include "file_system.h"
#include <ctype.h>

void sanitize_path(String &path)
{
    char *data = const_cast< char* >(path.data);

    for (U64 i = 0; i < path.count; i++)
    {
        data[i] = tolower(data[i]);
        if (data[i] == '\\')
        {
            data[i] = '/';
        }
    }
}

bool file_exists(String path)
{
    bool is_file = false;
    bool exists = platform_path_exists(path.data, &is_file);
    return is_file;
}

bool directory_exists(String path)
{
    bool is_file = false;
    bool exists = platform_path_exists(path.data, &is_file);
    return !is_file;
}

String open_file_dialog(String title, String filter, Array_View< String > extensions)
{
    Temprary_Memory_Arena_Janitor scratch_memory = make_scratch_memory_janitor();
    Memory_Arena *scratch_arena = scratch_memory.arena;

    const char **_extensions = nullptr;

    if (extensions.count)
    {
        _extensions = HE_ALLOCATE_ARRAY(scratch_arena, const char*, extensions.count);
        for (U32 i = 0; i < extensions.count; i++)
        {
            _extensions[i] = extensions[i].data;
        }
    }

    char *buffer = (char *)&scratch_arena->base[scratch_arena->offset];
    U64 count = scratch_arena->size - scratch_arena->offset;
    bool success = platform_open_file_dialog(buffer, count, title.data, title.count, filter.data, filter.count, _extensions, extensions.count);
    if (!success)
    {
        return HE_STRING_LITERAL("");
    }
    String result = HE_STRING(buffer);
    sanitize_path(result);

    scratch_arena->offset += result.count + 1;
    String working_path = get_current_working_directory(to_allocator(scratch_memory.arena));
    String path = sub_string(result, working_path.count + 1);
    return copy_string(path, to_allocator(get_general_purpose_allocator()));
}

String save_file_dialog(String title, String filter, Array_View< String > extensions)
{
    Temprary_Memory_Arena_Janitor scratch_memory = make_scratch_memory_janitor();
    Memory_Arena *scratch_arena = scratch_memory.arena;

    const char **_extensions = nullptr;

    if (extensions.count)
    {
        _extensions = HE_ALLOCATE_ARRAY(scratch_arena, const char*, extensions.count);
        for (U32 i = 0; i < extensions.count; i++)
        {
            _extensions[i] = extensions[i].data;
        }
    }

    char *buffer = (char *)&scratch_arena->base[scratch_arena->offset];
    U64 count = scratch_arena->size - scratch_arena->offset;
    bool success = platform_save_file_dialog(buffer, count, title.data, title.count, filter.data, filter.count, _extensions, extensions.count);
    if (!success)
    {
        return HE_STRING_LITERAL("");
    }

    String result = HE_STRING(buffer);
    scratch_arena->offset += result.count + 1;

    sanitize_path(result);
    String working_path = get_current_working_directory(to_allocator(scratch_memory.arena));
    String path = sub_string(result, working_path.count + 1);
    return copy_string(path, to_allocator(get_general_purpose_allocator()));
}

String get_current_working_directory(Allocator allocator)
{
    U64 size = 0;
    platform_get_current_working_directory(nullptr, &size);
    char *data = HE_ALLOCATOR_ALLOCATE_ARRAY(allocator, char, size);
    platform_get_current_working_directory(data, &size);
    return { size - 1, data };
}

String get_parent_path(String path)
{
    S64 slash_index = find_first_char_from_right(path, HE_STRING_LITERAL("\\/"));
    if (slash_index != -1)
    {
        return sub_string(path, 0, slash_index);
    }
    return HE_STRING_LITERAL("");
}

String get_extension(String path)
{
    S64 dot_index = find_first_char_from_right(path, HE_STRING_LITERAL("."));
    if (dot_index != -1)
    {
        return sub_string(path, dot_index + 1);
    }
    return HE_STRING_LITERAL("");
}

String get_name(String path)
{
    S64 start_index = find_first_char_from_right(path, HE_STRING_LITERAL("\\/"));
    if (start_index == -1)
    {
        start_index = 0;
    }
    else
    {
        start_index++;
    }

    S64 end_index = find_first_char_from_right(path, HE_STRING_LITERAL("."));
    if (end_index == -1)
    {
        end_index = path.count - 1;
    }
    else
    {
        end_index--;
    }

    return sub_string(path, start_index, end_index - start_index + 1);
}

String get_name_with_extension(String path)
{
    S64 start_index = find_first_char_from_right(path, HE_STRING_LITERAL("\\/"));
    if (start_index == -1)
    {
        start_index = 0;
    }
    else
    {
        start_index++;
    }
    
    return sub_string(path, start_index); 
}

Read_Entire_File_Result read_entire_file(String path, Allocator allocator)
{
    Open_File_Result open_file_result = platform_open_file(path.data, OpenFileFlag_Read);
    
    if (!open_file_result.success)
    {
        return { .success = false, .data = nullptr, .size = 0 };
    }

    if (!open_file_result.size)
    {
        return { .success = false, .data = nullptr, .size = 0 };
    }

    U8 *data = HE_ALLOCATOR_ALLOCATE_ARRAY(allocator, U8, open_file_result.size);
    bool read = platform_read_data_from_file(&open_file_result, 0, data, open_file_result.size);
    if (!read)
    {
        HE_ALLOCATOR_DEALLOCATE(allocator, data);
        return { .success = false, .data = nullptr, .size = open_file_result.size };
    }
    platform_close_file(&open_file_result);
    return { .success = true, .data = data, .size = open_file_result.size };
}

bool write_entire_file(String path, void *data, U64 size)
{
    Open_File_Result open_file_result = platform_open_file(path.data, Open_File_Flags(OpenFileFlag_Write|OpenFileFlag_Truncate));
    if (!open_file_result.success)
    {
        return false;
    }
    bool success = platform_write_data_to_file(&open_file_result, 0, data, size);
    platform_close_file(&open_file_result);
    return success;
}