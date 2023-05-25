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

#define HE_VULKAN_DEBUGGING 1

#ifdef HE_SHIPPING
#undef HE_VULKAN_DEBUGGING
#define HE_VULKAN_DEBUGGING 0
#endif

#if HE_VULKAN_DEBUGGING

#define CheckVkResult(VulkanFunctionCall)\
{\
    VkResult vk_result = VulkanFunctionCall;\
    Assert(vk_result == VK_SUCCESS);\
}

#else

#define CheckVkResult(VulkanFunctionCall) VulkanFunctionCall

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

    // note(amer): this should be going to texture agnostic
    U32 width;
    U32 height;
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

struct Vulkan_Graphics_Pipeline
{
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout layout;
    VkPipeline handle;
};

struct Vulkan_Buffer
{
    VkBuffer handle;
    VkDeviceMemory memory;
    void *data;
    U64 size;
};

struct Vulkan_Shader
{
    VkShaderModule handle;
};

// note(amer): to be moved to renderer types
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Global_Uniform_Buffer
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
};

struct Static_Mesh
{
    Vulkan_Buffer vertex_buffer;
    U32 vertex_count;

    Vulkan_Buffer index_buffer;
    U16 index_count;

    VkSampler sampler;
    Vulkan_Image image;
};
// =======================================

#define MAX_FRAMES_IN_FLIGHT 3

struct Vulkan_Context
{
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

    Vulkan_Shader vertex_shader;
    Vulkan_Shader fragment_shader;
    Vulkan_Graphics_Pipeline graphics_pipeline;

    VkCommandPool graphics_command_pool;
    VkCommandBuffer graphics_command_buffers[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore rendering_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence frame_in_flight_fences[MAX_FRAMES_IN_FLIGHT];

    Vulkan_Buffer global_uniform_buffers[MAX_FRAMES_IN_FLIGHT];

    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_sets[MAX_FRAMES_IN_FLIGHT];

    Free_List_Allocator *allocator;

    VkCommandPool transfer_command_pool;
    VkCommandBuffer transfer_command_buffer;
    Vulkan_Buffer transfer_buffer;

    U32 frames_in_flight;
    U32 current_frame_in_flight_index;

    // todo(amer): to be removed...
    Static_Mesh static_mesh;

#if HE_VULKAN_DEBUGGING
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
};