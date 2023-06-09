#pragma once

#include "vulkan_types.h"

bool create_image(Vulkan_Image *image, Vulkan_Context *context,
                  U32 width, U32 height, VkFormat format,
                  VkImageTiling tiling, VkImageUsageFlags usage,
                  VkImageAspectFlags aspect_flags,
                  VkMemoryPropertyFlags memory_property_flags,
                  bool mipmapping = false,
                  VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

void
copy_buffer_to_image(Vulkan_Context *context, Vulkan_Image *image, U32 width, U32 height, void *data, U64 size);

void
destroy_image(Vulkan_Image *image, Vulkan_Context *context);
