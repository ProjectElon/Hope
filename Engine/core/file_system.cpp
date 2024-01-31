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

bool file_exists(const String &path)
{
    bool is_file = false;
    bool exists = platform_path_exists(path.data, &is_file);
    return is_file;
}

bool directory_exists(const String &path)
{
    bool is_file = false;
    bool exists = platform_path_exists(path.data, &is_file);
    return !is_file;
}

String get_parent_path(const String &path)
{
    S64 slash_index = find_first_char_from_right(path, "\\/");
    if (slash_index != -1)
    {
        return sub_string(path, 0, slash_index);
    }
    return HE_STRING_LITERAL("");
}

String get_extension(const String &path)
{
    S64 dot_index = find_first_char_from_right(path, ".");
    if (dot_index != -1)
    {
        return sub_string(path, dot_index + 1);
    }
    return HE_STRING_LITERAL("");
}

String get_name(const String &path)
{
    S64 start_index = find_first_char_from_right(path, "\\/");
    if (start_index == -1)
    {
        start_index = 0;
    }
    else
    {
        start_index++;
    }

    S64 end_index = find_first_char_from_right(path, ".");
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

bool write_entire_file(const char *filepath, void *data, U64 size)
{
    Open_File_Result open_file_result = platform_open_file(filepath, Open_File_Flags(OpenFileFlag_Write|OpenFileFlag_Truncate));
    if (!open_file_result.success)
    {
        return false;
    }
    bool success = platform_write_data_to_file(&open_file_result, 0, data, size);
    HE_ASSERT(success);
    platform_close_file(&open_file_result);
    return success;
}