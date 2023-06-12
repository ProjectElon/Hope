#pragma once

#include "core/defines.h"

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/quaternion.hpp> // quaternion
#include <glm/gtx/quaternion.hpp> // quaternion
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi
#include <glm/gtc/type_ptr.hpp>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

enum TextureFormat
{
    TextureFormat_RGBA
};

#define MAX_TEXTURE_NAME 256

struct Texture
{
    char name[MAX_TEXTURE_NAME];
    U32 name_length;

    U32 width;
    U32 height;
};

#define MAX_MATERIAL_NAME 256

struct Material
{
    char name[MAX_MATERIAL_NAME];
    U32 name_length;

    U64 hash; // todo(amer): temprary
    Texture *albedo;
};

#define MAX_MESH_NAME 256

struct Static_Mesh
{
    U16 vertex_count;
    U32 index_count;

    Material *material;
};

struct Scene_Node
{
    Scene_Node *parent;
    Scene_Node *first_child;
    Scene_Node *last_child;
    Scene_Node *next_sibling;

    U32 start_mesh_index;
    U32 static_mesh_count;

    glm::mat4 transform;
};