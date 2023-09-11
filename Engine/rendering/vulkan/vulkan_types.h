#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/quaternion.hpp> // quaternion
#include <glm/gtx/quaternion.hpp> // quaternion
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi

#include "core/defines.h"
#include "core/memory.h"
#include "core/platform.h"
#include "rendering/renderer_types.h"

#if HE_GRAPHICS_DEBUGGING

#define HE_CHECK_VKRESULT(vulkan_function_call)\
{\
    VkResult result = vulkan_function_call;\
    HE_ASSERT(result == VK_SUCCESS);\
}

#else

#define HE_CHECK_VKRESULT(vulkan_function_call) vulkan_function_call

#endif

struct Vulkan_Image
{
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    VkFormat format;
    U32 mip_levels;
    void *data;
    U64 size;

    VkSampler sampler;
};

struct Vulkan_Buffer
{
    VkBuffer handle;
    VkDeviceMemory memory;
    void *data;
    U64 size;
};

struct Vulkan_Descriptor_Set
{
    U32 binding_count;
    VkDescriptorSetLayoutBinding *bindings;
};

struct Vulkan_Shader
{
    VkShaderModule handle;
    VkShaderStageFlagBits stage;
    Vulkan_Descriptor_Set descriptor_sets[HE_MAX_DESCRIPTOR_SET_COUNT];
};

struct Vulkan_Pipeline_State
{
    U32 descriptor_set_layout_count;
    VkDescriptorSetLayout descriptor_set_layouts[HE_MAX_DESCRIPTOR_SET_COUNT];

    VkPipelineLayout layout;
    VkPipeline handle;
};

struct Vulkan_Swapchain_Support
{
    U32 surface_format_count;
    VkSurfaceFormatKHR *surface_formats;

    U32 present_mode_count;
    VkPresentModeKHR *present_modes;

    VkFormat image_format;
    VkFormat depth_stencil_format;
};

struct Vulkan_Swapchain
{
    VkSwapchainKHR handle;
    U32 width;
    U32 height;
    VkPresentModeKHR present_mode;
    VkFormat image_format;
    VkColorSpaceKHR image_color_space;

    U32 image_count;
    VkImage *images;
    VkImageView *image_views;
    VkFramebuffer *frame_buffers;

    VkFormat depth_stencil_format;
    Vulkan_Image color_attachment;
    Vulkan_Image depth_stencil_attachment;
};

struct Vulkan_Material
{
    Vulkan_Buffer buffers[HE_MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet descriptor_sets[HE_MAX_FRAMES_IN_FLIGHT];
};

struct Vulkan_Static_Mesh
{
    S32 first_vertex;
    U32 first_index;
};

struct Engine;

struct Vulkan_Context
{
    Engine *engine;

    VkInstance instance;

    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties physical_device_properties;
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;

    U32 graphics_queue_family_index;
    U32 present_queue_family_index;
    U32 transfer_queue_family_index;
    VkDevice logical_device;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    Vulkan_Swapchain_Support swapchain_support;
    Vulkan_Swapchain swapchain;

    VkSampleCountFlagBits msaa_samples;
    VkRenderPass render_pass;

    VkPipelineCache pipeline_cache;

    VkSemaphore image_available_semaphores[HE_MAX_FRAMES_IN_FLIGHT];
    VkSemaphore rendering_finished_semaphores[HE_MAX_FRAMES_IN_FLIGHT];
    VkFence frame_in_flight_fences[HE_MAX_FRAMES_IN_FLIGHT];

    Vulkan_Buffer globals_uniform_buffers[HE_MAX_FRAMES_IN_FLIGHT];

    Vulkan_Buffer object_storage_buffers[HE_MAX_FRAMES_IN_FLIGHT];
    Object_Data *object_data_base;
    U32 object_data_count;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_sets[HE_MAX_DESCRIPTOR_SET_COUNT][HE_MAX_FRAMES_IN_FLIGHT];

    VkCommandPool graphics_command_pool;
    VkCommandBuffer graphics_command_buffers[HE_MAX_FRAMES_IN_FLIGHT];

    VkCommandPool transfer_command_pool;
    Vulkan_Buffer transfer_buffer;

    U64 max_vertex_count;
    U64 vertex_count;
    Vulkan_Buffer position_buffer;
    Vulkan_Buffer normal_buffer;
    Vulkan_Buffer uv_buffer;
    Vulkan_Buffer tangent_buffer;

    Vulkan_Buffer index_buffer;
    U64 index_offset;

    U32 frames_in_flight;
    U32 current_frame_in_flight_index;
    U32 current_swapchain_image_index;

    Vulkan_Image *textures;
    Vulkan_Material *materials;
    Vulkan_Static_Mesh *static_meshes;
    Vulkan_Shader *shaders;
    Vulkan_Pipeline_State *pipeline_states;

    Free_List_Allocator *allocator;
    Free_List_Allocator transfer_allocator;

    VkDescriptorPool imgui_descriptor_pool;

#if HE_GRAPHICS_DEBUGGING
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
};