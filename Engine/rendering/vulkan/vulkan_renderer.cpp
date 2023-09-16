#include <string.h>

#include "vulkan_renderer.h"
#include "core/platform.h"
#include "core/debugging.h"
#include "core/memory.h"
#include "core/file_system.h"
#include "core/engine.h"
#include "rendering/renderer.h"

#include "vulkan_image.h"
#include "vulkan_buffer.h"
#include "vulkan_swapchain.h"
#include "vulkan_shader.h"
#include "core/cvars.h"

#include "ImGui/backends/imgui_impl_vulkan.cpp"

static Vulkan_Context vulkan_context;

static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                      VkDebugUtilsMessageTypeFlagsEXT message_type,
                      const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                      void *user_data)
{
    (void)message_severity;
    (void)message_type;
    (void)user_data;
    HE_LOG(Rendering, Trace, "%s\n", callback_data->pMessage);
    HE_ASSERT(message_severity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
    return VK_FALSE;
}

S32 find_memory_type_index(Vulkan_Context *context, VkMemoryRequirements memory_requirements,VkMemoryPropertyFlags memory_property_flags)
{
    S32 result = -1;

    for (U32 memory_type_index = 0;
        memory_type_index < context->physical_device_memory_properties.memoryTypeCount;
        memory_type_index++)
    {
        if (((1 << memory_type_index) & memory_requirements.memoryTypeBits))
        {
            // todo(amer): we should track how much memory we allocated from heaps so allocations don't fail
            const VkMemoryType *memory_type = &context->physical_device_memory_properties.memoryTypes[memory_type_index];
            if ((memory_type->propertyFlags & memory_property_flags) == memory_property_flags)
            {
                result = (S32)memory_type_index;
                break;
            }
        }
    }

    return result;
}

static bool is_physical_device_supports_all_features(VkPhysicalDevice physical_device, VkPhysicalDeviceFeatures2 features2, VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features)
{
    VkPhysicalDeviceDescriptorIndexingFeatures supported_descriptor_indexing_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES };

    VkPhysicalDeviceFeatures2 supported_features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    supported_features2.pNext = &supported_descriptor_indexing_features;
    vkGetPhysicalDeviceFeatures2(physical_device, &supported_features2);

    if (features2.features.robustBufferAccess && !supported_features2.features.robustBufferAccess ||
        features2.features.fullDrawIndexUint32 && !supported_features2.features.fullDrawIndexUint32 ||
        features2.features.imageCubeArray && !supported_features2.features.imageCubeArray ||
        features2.features.independentBlend && !supported_features2.features.independentBlend ||
        features2.features.geometryShader && !supported_features2.features.geometryShader ||
        features2.features.tessellationShader && !supported_features2.features.tessellationShader ||
        features2.features.sampleRateShading && !supported_features2.features.sampleRateShading ||
        features2.features.dualSrcBlend && !supported_features2.features.dualSrcBlend ||
        features2.features.logicOp && !supported_features2.features.logicOp ||
        features2.features.multiDrawIndirect && !supported_features2.features.multiDrawIndirect ||
        features2.features.drawIndirectFirstInstance && !supported_features2.features.drawIndirectFirstInstance ||
        features2.features.depthClamp && !supported_features2.features.depthClamp ||
        features2.features.depthBiasClamp && !supported_features2.features.depthBiasClamp ||
        features2.features.fillModeNonSolid && !supported_features2.features.fillModeNonSolid ||
        features2.features.depthBounds && !supported_features2.features.depthBounds ||
        features2.features.wideLines && !supported_features2.features.wideLines ||
        features2.features.largePoints && !supported_features2.features.largePoints ||
        features2.features.alphaToOne && !supported_features2.features.alphaToOne ||
        features2.features.multiViewport && !supported_features2.features.multiViewport ||
        features2.features.samplerAnisotropy && !supported_features2.features.samplerAnisotropy ||
        features2.features.textureCompressionETC2 && !supported_features2.features.textureCompressionETC2 ||
        features2.features.textureCompressionASTC_LDR && !supported_features2.features.textureCompressionASTC_LDR ||
        features2.features.textureCompressionBC && !supported_features2.features.textureCompressionBC ||
        features2.features.occlusionQueryPrecise && !supported_features2.features.occlusionQueryPrecise ||
        features2.features.pipelineStatisticsQuery && !supported_features2.features.pipelineStatisticsQuery ||
        features2.features.vertexPipelineStoresAndAtomics && !supported_features2.features.vertexPipelineStoresAndAtomics ||
        features2.features.fragmentStoresAndAtomics && !supported_features2.features.fragmentStoresAndAtomics ||
        features2.features.shaderTessellationAndGeometryPointSize && !supported_features2.features.shaderTessellationAndGeometryPointSize ||
        features2.features.shaderImageGatherExtended && !supported_features2.features.shaderImageGatherExtended ||
        features2.features.shaderStorageImageExtendedFormats && !supported_features2.features.shaderStorageImageExtendedFormats ||
        features2.features.shaderStorageImageMultisample && !supported_features2.features.shaderStorageImageMultisample ||
        features2.features.shaderStorageImageReadWithoutFormat && !supported_features2.features.shaderStorageImageReadWithoutFormat ||
        features2.features.shaderStorageImageWriteWithoutFormat && !supported_features2.features.shaderStorageImageWriteWithoutFormat ||
        features2.features.shaderUniformBufferArrayDynamicIndexing && !supported_features2.features.shaderUniformBufferArrayDynamicIndexing ||
        features2.features.shaderSampledImageArrayDynamicIndexing && !supported_features2.features.shaderSampledImageArrayDynamicIndexing ||
        features2.features.shaderStorageBufferArrayDynamicIndexing && !supported_features2.features.shaderStorageBufferArrayDynamicIndexing ||
        features2.features.shaderStorageImageArrayDynamicIndexing && !supported_features2.features.shaderStorageImageArrayDynamicIndexing ||
        features2.features.shaderClipDistance && !supported_features2.features.shaderClipDistance ||
        features2.features.shaderCullDistance && !supported_features2.features.shaderCullDistance ||
        features2.features.shaderFloat64 && !supported_features2.features.shaderFloat64 ||
        features2.features.shaderInt64 && !supported_features2.features.shaderInt64 ||
        features2.features.shaderInt16 && !supported_features2.features.shaderInt16 ||
        features2.features.shaderResourceResidency && !supported_features2.features.shaderResourceResidency ||
        features2.features.shaderResourceMinLod && !supported_features2.features.shaderResourceMinLod ||
        features2.features.sparseBinding && !supported_features2.features.sparseBinding ||
        features2.features.sparseResidencyBuffer && !supported_features2.features.sparseResidencyBuffer ||
        features2.features.sparseResidencyImage2D && !supported_features2.features.sparseResidencyImage2D ||
        features2.features.sparseResidencyImage3D && !supported_features2.features.sparseResidencyImage3D ||
        features2.features.sparseResidency2Samples && !supported_features2.features.sparseResidency2Samples ||
        features2.features.sparseResidency4Samples && !supported_features2.features.sparseResidency4Samples ||
        features2.features.sparseResidency8Samples && !supported_features2.features.sparseResidency8Samples ||
        features2.features.sparseResidency16Samples && !supported_features2.features.sparseResidency16Samples ||
        features2.features.sparseResidencyAliased && !supported_features2.features.sparseResidencyAliased ||
        features2.features.variableMultisampleRate && !supported_features2.features.variableMultisampleRate ||
        features2.features.inheritedQueries && !supported_features2.features.inheritedQueries)
    {
        return false;
    }

    if (descriptor_indexing_features.shaderInputAttachmentArrayDynamicIndexing && !supported_descriptor_indexing_features.shaderInputAttachmentArrayDynamicIndexing ||
        descriptor_indexing_features.shaderUniformTexelBufferArrayDynamicIndexing && !supported_descriptor_indexing_features.shaderUniformTexelBufferArrayDynamicIndexing ||
        descriptor_indexing_features.shaderStorageTexelBufferArrayDynamicIndexing && !supported_descriptor_indexing_features.shaderStorageTexelBufferArrayDynamicIndexing ||
        descriptor_indexing_features.shaderUniformBufferArrayNonUniformIndexing && !supported_descriptor_indexing_features.shaderUniformBufferArrayNonUniformIndexing ||
        descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing && !supported_descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing ||
        descriptor_indexing_features.shaderStorageBufferArrayNonUniformIndexing && !supported_descriptor_indexing_features.shaderStorageBufferArrayNonUniformIndexing ||
        descriptor_indexing_features.shaderStorageImageArrayNonUniformIndexing && !supported_descriptor_indexing_features.shaderStorageImageArrayNonUniformIndexing ||
        descriptor_indexing_features.shaderInputAttachmentArrayNonUniformIndexing && !supported_descriptor_indexing_features.shaderInputAttachmentArrayNonUniformIndexing ||
        descriptor_indexing_features.shaderUniformTexelBufferArrayNonUniformIndexing && !supported_descriptor_indexing_features.shaderUniformTexelBufferArrayNonUniformIndexing ||
        descriptor_indexing_features.shaderStorageTexelBufferArrayNonUniformIndexing && !supported_descriptor_indexing_features.shaderStorageTexelBufferArrayNonUniformIndexing ||
        descriptor_indexing_features.descriptorBindingUniformBufferUpdateAfterBind && !supported_descriptor_indexing_features.descriptorBindingUniformBufferUpdateAfterBind ||
        descriptor_indexing_features.descriptorBindingSampledImageUpdateAfterBind && !supported_descriptor_indexing_features.descriptorBindingSampledImageUpdateAfterBind ||
        descriptor_indexing_features.descriptorBindingStorageImageUpdateAfterBind && !supported_descriptor_indexing_features.descriptorBindingStorageImageUpdateAfterBind ||
        descriptor_indexing_features.descriptorBindingStorageBufferUpdateAfterBind && !supported_descriptor_indexing_features.descriptorBindingStorageBufferUpdateAfterBind ||
        descriptor_indexing_features.descriptorBindingUniformTexelBufferUpdateAfterBind && !supported_descriptor_indexing_features.descriptorBindingUniformTexelBufferUpdateAfterBind ||
        descriptor_indexing_features.descriptorBindingStorageTexelBufferUpdateAfterBind && !supported_descriptor_indexing_features.descriptorBindingStorageTexelBufferUpdateAfterBind ||
        descriptor_indexing_features.descriptorBindingUpdateUnusedWhilePending && !supported_descriptor_indexing_features.descriptorBindingUpdateUnusedWhilePending ||
        descriptor_indexing_features.descriptorBindingPartiallyBound && !supported_descriptor_indexing_features.descriptorBindingPartiallyBound ||
        descriptor_indexing_features.descriptorBindingVariableDescriptorCount && !supported_descriptor_indexing_features.descriptorBindingVariableDescriptorCount ||
        descriptor_indexing_features.runtimeDescriptorArray && !supported_descriptor_indexing_features.runtimeDescriptorArray)
    {
        return false;
    }

    return true;
}

static VkPhysicalDevice
pick_physical_device(VkInstance instance, VkSurfaceKHR surface,
                     VkPhysicalDeviceFeatures2 features,
                     VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features,
                     Memory_Arena *arena)
{
    Temprary_Memory_Arena temprary_arena = {};
    begin_temprary_memory_arena(&temprary_arena, arena);

    HE_DEFER
    {
        end_temprary_memory_arena(&temprary_arena);
    };

    U32 physical_device_count = 0;
    HE_CHECK_VKRESULT(vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));

    if (!physical_device_count)
    {
        // todo(amer): logging
        return VK_NULL_HANDLE;
    }

    VkPhysicalDevice *physical_devices = HE_ALLOCATE_ARRAY(&temprary_arena,
                                                           VkPhysicalDevice,
                                                           physical_device_count);

    HE_CHECK_VKRESULT(vkEnumeratePhysicalDevices(instance,
                                                 &physical_device_count,
                                                 physical_devices));

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    U32 best_physical_device_score_so_far = 0;

    for (U32 physical_device_index = 0;
         physical_device_index < physical_device_count;
         physical_device_index++)
    {
        VkPhysicalDevice *current_physical_device = &physical_devices[physical_device_index];

        if (!is_physical_device_supports_all_features(*current_physical_device, features, descriptor_indexing_features))
        {
            continue;
        }

        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(*current_physical_device, &properties);

        U32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(*current_physical_device,
                                                 &queue_family_count,
                                                 nullptr);

        bool can_physical_device_do_graphics = false;
        bool can_physical_device_present = false;

        VkQueueFamilyProperties *queue_families = HE_ALLOCATE_ARRAY(&temprary_arena,
                                                                    VkQueueFamilyProperties,
                                                                    queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(*current_physical_device,
                                                 &queue_family_count,
                                                 queue_families);

        for (U32 queue_family_index = 0;
             queue_family_index < queue_family_count;
             queue_family_index++)
        {
            VkQueueFamilyProperties *queue_family = &queue_families[queue_family_index];

            if ((queue_family->queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                can_physical_device_do_graphics = true;
            }

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(*current_physical_device,
                                                 queue_family_index,
                                                 surface,
                                                 &present_support);

            if (present_support == VK_TRUE)
            {
                can_physical_device_present = true;
            }
        }

        if (can_physical_device_do_graphics && can_physical_device_present)
        {
            U32 score = 0;
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                score++;
            }
            if (score >= best_physical_device_score_so_far)
            {
                best_physical_device_score_so_far = score;
                physical_device = *current_physical_device;
            }
        }
    }

    return physical_device;
}

static bool
init_imgui(Vulkan_Context *context)
{
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1024 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1024 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_create_info.maxSets = 1024;
    descriptor_pool_create_info.poolSizeCount = HE_ARRAYCOUNT(pool_sizes);
    descriptor_pool_create_info.pPoolSizes = pool_sizes;

    HE_CHECK_VKRESULT(vkCreateDescriptorPool(context->logical_device,
                                              &descriptor_pool_create_info,
                                              nullptr,
                                              &context->imgui_descriptor_pool));

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = context->instance;
    init_info.PhysicalDevice = context->physical_device;
    init_info.Device = context->logical_device;
    init_info.Queue = context->graphics_queue;
    init_info.QueueFamily = context->graphics_queue_family_index;
    init_info.DescriptorPool = context->imgui_descriptor_pool;
    init_info.MinImageCount = context->swapchain.image_count;
    init_info.ImageCount = context->swapchain.image_count;
    init_info.MSAASamples = context->msaa_samples;
    init_info.PipelineCache = context->pipeline_cache;
    ImGui_ImplVulkan_Init(&init_info, context->render_pass);

    VkCommandBuffer command_buffer = context->graphics_command_buffers[0];
    vkResetCommandBuffer(command_buffer, 0);

    VkCommandBufferBeginInfo command_buffer_begin_info =
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    command_buffer_begin_info.flags = 0;
    command_buffer_begin_info.pInheritanceInfo = 0;

    vkBeginCommandBuffer(command_buffer,
                         &command_buffer_begin_info);
    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(context->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(context->graphics_queue);
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    return true;
}

static bool init_vulkan(Vulkan_Context *context, Engine *engine)
{
    context->engine = engine;
    context->allocator = &engine->memory.free_list_allocator;

    Memory_Arena *arena = &engine->memory.permanent_arena;
    context->arena = create_sub_arena(arena, HE_MEGA(32));
    context->buffers = HE_ALLOCATE_ARRAY(arena, Vulkan_Buffer, HE_MAX_BUFFER_COUNT);
    context->textures = HE_ALLOCATE_ARRAY(arena, Vulkan_Image, HE_MAX_TEXTURE_COUNT);
    context->samplers = HE_ALLOCATE_ARRAY(arena, Vulkan_Sampler, HE_MAX_SAMPLER_COUNT);
    context->materials = HE_ALLOCATE_ARRAY(arena, Vulkan_Material, HE_MAX_MATERIAL_COUNT);
    context->static_meshes = HE_ALLOCATE_ARRAY(arena, Vulkan_Static_Mesh, HE_MAX_STATIC_MESH_COUNT);
    context->shaders = HE_ALLOCATE_ARRAY(arena, Vulkan_Shader, HE_MAX_SHADER_COUNT);
    context->pipeline_states = HE_ALLOCATE_ARRAY(arena, Vulkan_Pipeline_State, HE_MAX_PIPELINE_STATE_COUNT);

    const char *required_instance_extensions[] =
    {
#if HE_OS_WINDOWS
        "VK_KHR_win32_surface",
#endif

#if HE_GRAPHICS_DEBUGGING
        "VK_EXT_debug_utils",
#endif

        "VK_KHR_surface",
    };

    U32 required_api_version = VK_API_VERSION_1_1;
    U32 driver_api_version = 0;

    // vkEnumerateInstanceVersion requires at least vulkan 1.1
    auto enumerate_instance_version = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion");

    if (enumerate_instance_version)
    {
        enumerate_instance_version(&driver_api_version);
    }
    else
    {
        driver_api_version = VK_API_VERSION_1_0;
    }

    HE_ASSERT(required_api_version <= driver_api_version);

    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = engine->app_name.data;
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName = engine->name.data;
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = required_api_version;

    VkInstanceCreateInfo instance_create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledExtensionCount = HE_ARRAYCOUNT(required_instance_extensions);
    instance_create_info.ppEnabledExtensionNames = required_instance_extensions;

#if HE_GRAPHICS_DEBUGGING

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info =
        { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

    debug_messenger_create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    debug_messenger_create_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;

    debug_messenger_create_info.pfnUserCallback = vulkan_debug_callback;
    debug_messenger_create_info.pUserData = nullptr;

    const char *layers[] =
    {
        "VK_LAYER_KHRONOS_validation",
    };

    instance_create_info.enabledLayerCount = HE_ARRAYCOUNT(layers);
    instance_create_info.ppEnabledLayerNames = layers;
    instance_create_info.pNext = &debug_messenger_create_info;

#endif

    HE_CHECK_VKRESULT(vkCreateInstance(&instance_create_info, nullptr, &context->instance));

#if HE_GRAPHICS_DEBUGGING

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerExt =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");
    HE_ASSERT(vkCreateDebugUtilsMessengerExt);

    HE_CHECK_VKRESULT(vkCreateDebugUtilsMessengerExt(context->instance,
                                                     &debug_messenger_create_info,
                                                     nullptr,
                                                     &context->debug_messenger));

#endif

    context->surface = (VkSurfaceKHR)platform_create_vulkan_surface(engine,
                                                                    context->instance);
    HE_ASSERT(context->surface);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES };
    descriptor_indexing_features.shaderInputAttachmentArrayDynamicIndexing = VK_TRUE;
    descriptor_indexing_features.shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptor_indexing_features.shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptor_indexing_features.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    descriptor_indexing_features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    descriptor_indexing_features.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    descriptor_indexing_features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    descriptor_indexing_features.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
    descriptor_indexing_features.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;
    descriptor_indexing_features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    descriptor_indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
    descriptor_indexing_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptor_indexing_features.runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceFeatures2 physical_device_features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    physical_device_features2.features.samplerAnisotropy = VK_TRUE;
    physical_device_features2.features.sampleRateShading = VK_TRUE;
    physical_device_features2.pNext = &descriptor_indexing_features;

    context->physical_device = pick_physical_device(context->instance,
                                                    context->surface,
                                                    physical_device_features2,
                                                    descriptor_indexing_features,
                                                    arena);
    HE_ASSERT(context->physical_device != VK_NULL_HANDLE);

    vkGetPhysicalDeviceMemoryProperties(context->physical_device, &context->physical_device_memory_properties);
    vkGetPhysicalDeviceProperties(context->physical_device, &context->physical_device_properties);

    VkSampleCountFlags counts = context->physical_device_properties.limits.framebufferColorSampleCounts &
                                context->physical_device_properties.limits.framebufferDepthSampleCounts;

    VkSampleCountFlagBits max_sample_count = VK_SAMPLE_COUNT_1_BIT;

    if (counts & VK_SAMPLE_COUNT_64_BIT)
    {
        max_sample_count = VK_SAMPLE_COUNT_64_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_32_BIT)
    {
        max_sample_count = VK_SAMPLE_COUNT_32_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_16_BIT)
    {
        max_sample_count = VK_SAMPLE_COUNT_16_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_8_BIT)
    {
        max_sample_count = VK_SAMPLE_COUNT_8_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_4_BIT)
    {
        max_sample_count = VK_SAMPLE_COUNT_4_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_2_BIT)
    {
        max_sample_count = VK_SAMPLE_COUNT_2_BIT;
    }

    context->msaa_samples = VK_SAMPLE_COUNT_4_BIT;

    if (context->msaa_samples > max_sample_count)
    {
        context->msaa_samples = max_sample_count;
    }

    {
        Temprary_Memory_Arena temprary_arena = {};
        begin_temprary_memory_arena(&temprary_arena, arena);

        HE_DEFER
        {
            end_temprary_memory_arena(&temprary_arena);
        };

        context->graphics_queue_family_index = 0;
        context->present_queue_family_index = 0;

        U32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device,
                                                 &queue_family_count,
                                                 nullptr);

        VkQueueFamilyProperties *queue_families = HE_ALLOCATE_ARRAY(&temprary_arena,
                                                                    VkQueueFamilyProperties,
                                                                    queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device,
                                                 &queue_family_count,
                                                 queue_families);

        bool found_a_queue_family_that_can_do_graphics_and_present = false;

        for (U32 queue_family_index = 0;
                 queue_family_index < queue_family_count;
                 queue_family_index++)
        {
            VkQueueFamilyProperties *queue_family = &queue_families[queue_family_index];

            bool can_queue_family_do_graphics =
                (queue_family->queueFlags & VK_QUEUE_GRAPHICS_BIT);

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(context->physical_device,
                                                 queue_family_index,
                                                 context->surface, &present_support);

            bool can_queue_family_present = present_support == VK_TRUE;

            if (can_queue_family_do_graphics && can_queue_family_present)
            {
                context->graphics_queue_family_index = queue_family_index;
                context->present_queue_family_index = queue_family_index;
                found_a_queue_family_that_can_do_graphics_and_present = true;
                break;
            }
        }

        if (!found_a_queue_family_that_can_do_graphics_and_present)
        {
            for (U32 queue_family_index = 0;
                 queue_family_index < queue_family_count;
                 queue_family_index++)
            {
                VkQueueFamilyProperties *queue_family = &queue_families[queue_family_index];

                if (queue_family->queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    context->graphics_queue_family_index = queue_family_index;
                }

                VkBool32 present_support = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(context->physical_device,
                                                     queue_family_index,
                                                     context->surface, &present_support);
                if (present_support == VK_TRUE)
                {
                    context->present_queue_family_index = queue_family_index;
                }
            }
        }

        context->transfer_queue_family_index = context->graphics_queue_family_index;

        for (U32 queue_family_index = 0;
             queue_family_index < queue_family_count;
             queue_family_index++)
        {
            VkQueueFamilyProperties *queue_family = &queue_families[queue_family_index];
            if ((queue_family->queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queue_family->queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                context->transfer_queue_family_index = queue_family_index;
                break;
            }
        }

        F32 queue_priority = 1.0f;
        VkDeviceQueueCreateInfo *queue_create_infos = HE_ALLOCATE_ARRAY(&temprary_arena,
                                                                        VkDeviceQueueCreateInfo,
                                                                        3);

        queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[0].queueFamilyIndex = context->graphics_queue_family_index;
        queue_create_infos[0].queueCount = 1;
        queue_create_infos[0].pQueuePriorities = &queue_priority;

        U32 queue_create_info_count = 1;

        if (!found_a_queue_family_that_can_do_graphics_and_present)
        {
            U32 queue_create_info_index = queue_create_info_count++;
            queue_create_infos[queue_create_info_index].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_infos[queue_create_info_index].queueFamilyIndex = context->present_queue_family_index;
            queue_create_infos[queue_create_info_index].queueCount = 1;
            queue_create_infos[queue_create_info_index].pQueuePriorities = &queue_priority;
        }

        if ((context->transfer_queue_family_index != context->graphics_queue_family_index)
            && (context->transfer_queue_family_index != context->present_queue_family_index))
        {
            U32 queue_create_info_index = queue_create_info_count++;
            queue_create_infos[queue_create_info_index].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_infos[queue_create_info_index].queueFamilyIndex = context->transfer_queue_family_index;
            queue_create_infos[queue_create_info_index].queueCount = 1;
            queue_create_infos[queue_create_info_index].pQueuePriorities = &queue_priority;
        }

        const char *required_device_extensions[] =
        {
            "VK_KHR_swapchain",
            "VK_KHR_push_descriptor",
            "VK_EXT_descriptor_indexing"
        };

        U32 extension_property_count = 0;
        vkEnumerateDeviceExtensionProperties(context->physical_device,
                                             nullptr, &extension_property_count,
                                             nullptr);

        VkExtensionProperties *extension_properties = HE_ALLOCATE_ARRAY(&temprary_arena,
                                                                        VkExtensionProperties,
                                                                        extension_property_count);

        vkEnumerateDeviceExtensionProperties(context->physical_device,
                                             nullptr, &extension_property_count,
                                             extension_properties);

        bool not_all_required_device_extensions_are_supported = false;

        for (U32 extension_index = 0;
             extension_index < HE_ARRAYCOUNT(required_device_extensions);
             extension_index++)
        {
            String device_extension = HE_STRING(required_device_extensions[extension_index]);
            bool is_extension_supported = false;

            for (U32 extension_property_index = 0;
                 extension_property_index < extension_property_count;
                 extension_property_index++)
            {
                VkExtensionProperties *extension_property = &extension_properties[extension_property_index];
                if (device_extension == extension_property->extensionName)
                {
                    is_extension_supported = true;
                    break;
                }
            }

            if (!is_extension_supported)
            {
                not_all_required_device_extensions_are_supported = true;
            }
        }

        if (not_all_required_device_extensions_are_supported)
        {
            return false;
        }

        VkDeviceCreateInfo device_create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        device_create_info.pQueueCreateInfos = queue_create_infos;
        device_create_info.queueCreateInfoCount = queue_create_info_count;
        device_create_info.pNext = &physical_device_features2;
        device_create_info.ppEnabledExtensionNames = required_device_extensions;
        device_create_info.enabledExtensionCount = HE_ARRAYCOUNT(required_device_extensions);

        HE_CHECK_VKRESULT(vkCreateDevice(context->physical_device,
                                          &device_create_info, nullptr,
                                          &context->logical_device));

        vkGetDeviceQueue(context->logical_device,
                         context->graphics_queue_family_index,
                         0, &context->graphics_queue);

        vkGetDeviceQueue(context->logical_device,
                         context->present_queue_family_index,
                         0, &context->present_queue);

        vkGetDeviceQueue(context->logical_device,
                         context->transfer_queue_family_index,
                         0, &context->transfer_queue);
    }

    VkFormat image_formats[] =
    {
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_SRGB
    };

    VkFormat depth_stencil_formats[] =
    {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    init_swapchain_support(context,
                           image_formats,
                           HE_ARRAYCOUNT(image_formats),
                           depth_stencil_formats,
                           HE_ARRAYCOUNT(depth_stencil_formats),
                           VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
                           arena,
                           &context->swapchain_support);

    VkAttachmentDescription attachments_msaa[3] = {};

    attachments_msaa[0].format = context->swapchain_support.image_format;
    attachments_msaa[0].samples = context->msaa_samples;
    attachments_msaa[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments_msaa[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments_msaa[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments_msaa[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments_msaa[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments_msaa[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments_msaa[1].format = context->swapchain_support.image_format;
    attachments_msaa[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments_msaa[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments_msaa[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments_msaa[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments_msaa[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments_msaa[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments_msaa[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments_msaa[2].format = context->swapchain_support.depth_stencil_format;
    attachments_msaa[2].samples = context->msaa_samples;
    attachments_msaa[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments_msaa[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments_msaa[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments_msaa[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments_msaa[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments_msaa[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attachments[2] = {};

    attachments[0].format = context->swapchain_support.image_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].format = context->swapchain_support.depth_stencil_format;
    attachments[1].samples = context->msaa_samples;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolve_color_attachment_ref = {};
    resolve_color_attachment_ref.attachment = 1;
    resolve_color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_stencil_attachment_ref = {};
    depth_stencil_attachment_ref.attachment = 2;
    depth_stencil_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    if (context->msaa_samples != VK_SAMPLE_COUNT_1_BIT)
    {
        subpass.pResolveAttachments = &resolve_color_attachment_ref;
    }
    else
    {
        depth_stencil_attachment_ref.attachment = 1;
    }

    subpass.pDepthStencilAttachment = &depth_stencil_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info =
        { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    if (context->msaa_samples != VK_SAMPLE_COUNT_1_BIT)
    {
        render_pass_create_info.attachmentCount = HE_ARRAYCOUNT(attachments_msaa);
        render_pass_create_info.pAttachments = attachments_msaa;
    }
    else
    {
        render_pass_create_info.attachmentCount = HE_ARRAYCOUNT(attachments);
        render_pass_create_info.pAttachments = attachments;
    }

    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;

    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    HE_CHECK_VKRESULT(vkCreateRenderPass(context->logical_device,
                                          &render_pass_create_info,
                                          nullptr, &context->render_pass));

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    U32 min_image_count = HE_MAX_FRAMES_IN_FLIGHT;
    U32 width = (U32)engine->window.width;
    U32 height = (U32)engine->window.height;
    bool swapchain_created = create_swapchain(context, width, height,
                                              min_image_count, present_mode, &context->swapchain);
    HE_ASSERT(swapchain_created);

    {
        Temprary_Memory_Arena temprary_arena = {};
        begin_temprary_memory_arena(&temprary_arena, arena);

        HE_DEFER
        {
            end_temprary_memory_arena(&temprary_arena);
        };

        U64 pipeline_cache_size = 0;
        U8 *pipeline_cache_data = nullptr;

        Read_Entire_File_Result result = read_entire_file(HE_PIPELINE_CACHE_FILENAME, &temprary_arena);
        if (result.success)
        {
            VkPipelineCacheHeaderVersionOne *pipeline_cache_header = (VkPipelineCacheHeaderVersionOne *)result.data;
            if (pipeline_cache_header->deviceID == context->physical_device_properties.deviceID &&
                pipeline_cache_header->vendorID == context->physical_device_properties.vendorID)
            {
                pipeline_cache_data = result.data;
                pipeline_cache_size = result.size;
            }
        }

        VkPipelineCacheCreateInfo pipeline_cache_create_info
            = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
        pipeline_cache_create_info.initialDataSize = pipeline_cache_size;
        pipeline_cache_create_info.pInitialData = pipeline_cache_data;
        HE_CHECK_VKRESULT(vkCreatePipelineCache(context->logical_device, &pipeline_cache_create_info,
                                                 nullptr, &context->pipeline_cache));
    }

    Renderer_State *renderer_state = &context->engine->renderer_state;
    renderer_state->mesh_vertex_shader = aquire_handle(&renderer_state->shaders);
    bool shader_loaded = load_shader(renderer_state->mesh_vertex_shader,
                                     "shaders/bin/mesh.vert.spv",
                                     context);
    HE_ASSERT(shader_loaded);

    renderer_state->mesh_fragment_shader = aquire_handle(&renderer_state->shaders);
    shader_loaded = load_shader(renderer_state->mesh_fragment_shader,
                                "shaders/bin/mesh.frag.spv",
                                context);
    HE_ASSERT(shader_loaded);

    renderer_state->mesh_pipeline = aquire_handle(&renderer_state->pipeline_states);
    bool pipeline_created = create_graphics_pipeline(renderer_state->mesh_pipeline,
                                                     { renderer_state->mesh_vertex_shader,
                                                     renderer_state->mesh_fragment_shader },
                                                     context->render_pass,
                                                     context);
    HE_ASSERT(pipeline_created);

    VkCommandPoolCreateInfo graphics_command_pool_create_info
        = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };

    graphics_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    graphics_command_pool_create_info.queueFamilyIndex = context->graphics_queue_family_index;

    HE_CHECK_VKRESULT(vkCreateCommandPool(context->logical_device,
                                           &graphics_command_pool_create_info,
                                           nullptr, &context->graphics_command_pool));

    VkCommandBufferAllocateInfo graphics_command_buffer_allocate_info
        = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };

    graphics_command_buffer_allocate_info.commandPool = context->graphics_command_pool;
    graphics_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    graphics_command_buffer_allocate_info.commandBufferCount = HE_MAX_FRAMES_IN_FLIGHT;
    HE_CHECK_VKRESULT(vkAllocateCommandBuffers(context->logical_device,
                                                &graphics_command_buffer_allocate_info,
                                                context->graphics_command_buffers));

    VkCommandPoolCreateInfo transfer_command_pool_create_info
        = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };

    transfer_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    transfer_command_pool_create_info.queueFamilyIndex = context->transfer_queue_family_index;
    HE_CHECK_VKRESULT(vkCreateCommandPool(context->logical_device,
                                           &transfer_command_pool_create_info,
                                           nullptr, &context->transfer_command_pool));

    VkDescriptorPoolSize descriptor_pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, HE_MAX_BINDLESS_RESOURCE_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, HE_MAX_BINDLESS_RESOURCE_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, HE_MAX_BINDLESS_RESOURCE_DESCRIPTOR_COUNT }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    descriptor_pool_create_info.poolSizeCount = HE_ARRAYCOUNT(descriptor_pool_sizes);
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes;
    descriptor_pool_create_info.maxSets = HE_MAX_BINDLESS_RESOURCE_DESCRIPTOR_COUNT * HE_ARRAYCOUNT(descriptor_pool_sizes);

    HE_CHECK_VKRESULT(vkCreateDescriptorPool(context->logical_device,
                                              &descriptor_pool_create_info,
                                              nullptr, &context->descriptor_pool));

    // set 0
    {
        VkDescriptorSetLayout level0_descriptor_set_layouts[HE_MAX_FRAMES_IN_FLIGHT] = {};

        for (U32 frame_index = 0;
             frame_index < HE_MAX_FRAMES_IN_FLIGHT;
             frame_index++)
        {
            Vulkan_Pipeline_State *mesh_pipeline = &context->pipeline_states[renderer_state->mesh_pipeline.index];
            level0_descriptor_set_layouts[frame_index] = mesh_pipeline->descriptor_set_layouts[0];
        }

        VkDescriptorSetAllocateInfo descriptor_set_allocation_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        descriptor_set_allocation_info.descriptorPool = context->descriptor_pool;
        descriptor_set_allocation_info.descriptorSetCount = HE_MAX_FRAMES_IN_FLIGHT;
        descriptor_set_allocation_info.pSetLayouts = level0_descriptor_set_layouts;

        HE_CHECK_VKRESULT(vkAllocateDescriptorSets(context->logical_device,
                                                    &descriptor_set_allocation_info,
                                                    context->descriptor_sets[0]));
    }

    // set 1
    {
        VkDescriptorSetLayout level1_descriptor_set_layouts[HE_MAX_FRAMES_IN_FLIGHT] = {};

        for (U32 frame_index = 0;
             frame_index < HE_MAX_FRAMES_IN_FLIGHT;
             frame_index++)
        {
            Vulkan_Pipeline_State *mesh_pipeline = &context->pipeline_states[renderer_state->mesh_pipeline.index];
            level1_descriptor_set_layouts[frame_index] = mesh_pipeline->descriptor_set_layouts[1];
        }

        VkDescriptorSetAllocateInfo descriptor_set_allocation_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        descriptor_set_allocation_info.descriptorPool = context->descriptor_pool;
        descriptor_set_allocation_info.descriptorSetCount = U32(HE_MAX_FRAMES_IN_FLIGHT);
        descriptor_set_allocation_info.pSetLayouts = level1_descriptor_set_layouts;

        HE_CHECK_VKRESULT(vkAllocateDescriptorSets(context->logical_device,
                                                    &descriptor_set_allocation_info,
                                                    context->descriptor_sets[1]));
    }

    VkSemaphoreCreateInfo semaphore_create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fence_create_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (U32 sync_primitive_index = 0;
         sync_primitive_index < HE_MAX_FRAMES_IN_FLIGHT;
         sync_primitive_index++)
    {
        HE_CHECK_VKRESULT(vkCreateSemaphore(context->logical_device, &semaphore_create_info, nullptr, &context->image_available_semaphores[sync_primitive_index]));
        HE_CHECK_VKRESULT(vkCreateSemaphore(context->logical_device, &semaphore_create_info, nullptr, &context->rendering_finished_semaphores[sync_primitive_index]));
        HE_CHECK_VKRESULT(vkCreateFence(context->logical_device, &fence_create_info, nullptr, &context->frame_in_flight_fences[sync_primitive_index]));
    }

    context->current_frame_in_flight_index = 0;
    context->frames_in_flight = 2;
    HE_ASSERT(context->frames_in_flight <= HE_MAX_FRAMES_IN_FLIGHT);

    init_imgui(context);
    return true;
}

void deinit_vulkan(Vulkan_Context *context)
{
    vkDeviceWaitIdle(context->logical_device);
    vkDestroyDescriptorPool(context->logical_device, context->descriptor_pool, nullptr);

    vkDestroyDescriptorPool(context->logical_device, context->imgui_descriptor_pool, nullptr);

    ImGui_ImplVulkan_Shutdown();
    
    for (U32 frame_index = 0;
         frame_index < HE_MAX_FRAMES_IN_FLIGHT;
         frame_index++)
    {
        vkDestroySemaphore(context->logical_device, context->image_available_semaphores[frame_index], nullptr);
        vkDestroySemaphore(context->logical_device, context->rendering_finished_semaphores[frame_index], nullptr);
        vkDestroyFence(context->logical_device, context->frame_in_flight_fences[frame_index], nullptr);
    }

    vkDestroyCommandPool(context->logical_device, context->graphics_command_pool, nullptr);
    vkDestroyCommandPool(context->logical_device, context->transfer_command_pool, nullptr);

    destroy_swapchain(context, &context->swapchain);

    U64 pipeline_cache_size = 0;
    vkGetPipelineCacheData(context->logical_device,
                           context->pipeline_cache,
                           &pipeline_cache_size,
                           nullptr);


    if (pipeline_cache_size)
    {
        U8 *pipeline_cache_data = HE_ALLOCATE_ARRAY(context->allocator, U8, pipeline_cache_size);
        vkGetPipelineCacheData(context->logical_device,
                               context->pipeline_cache,
                               &pipeline_cache_size,
                               pipeline_cache_data);

        write_entire_file(HE_PIPELINE_CACHE_FILENAME, pipeline_cache_data, pipeline_cache_size);
    }

    vkDestroyPipelineCache(context->logical_device, context->pipeline_cache, nullptr);

    vkDestroyRenderPass(context->logical_device, context->render_pass, nullptr);

    vkDestroySurfaceKHR(context->instance, context->surface, nullptr);
    vkDestroyDevice(context->logical_device, nullptr);

#if HE_GRAPHICS_DEBUGGING
     PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerExt =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance,
                                                                  "vkDestroyDebugUtilsMessengerEXT");
    HE_ASSERT(vkDestroyDebugUtilsMessengerExt);
    vkDestroyDebugUtilsMessengerExt(context->instance,
                                    context->debug_messenger,
                                    nullptr);
#endif

    vkDestroyInstance(context->instance, nullptr);
}

bool vulkan_renderer_init(Engine *engine)
{
    return init_vulkan(&vulkan_context, engine);
}

void vulkan_renderer_wait_for_gpu_to_finish_all_work()
{
    vkDeviceWaitIdle(vulkan_context.logical_device);
}

void vulkan_renderer_deinit()
{
    deinit_vulkan(&vulkan_context);
}

void vulkan_renderer_on_resize(U32 width, U32 height)
{
    if (width != 0 && height != 0)
    {
        Renderer_State *renderer_state = &vulkan_context.engine->renderer_state;
        renderer_state->back_buffer_width = width;
        renderer_state->back_buffer_height = height;

        recreate_swapchain(&vulkan_context,
                           &vulkan_context.swapchain,
                           width,
                           height,
                           vulkan_context.swapchain.present_mode);
    }
}

void vulkan_renderer_imgui_new_frame()
{
    ImGui_ImplVulkan_NewFrame();
}

void vulkan_renderer_begin_frame(const Scene_Data *scene_data)
{
    Vulkan_Context *context = &vulkan_context;
    Renderer_State *renderer_state = &context->engine->renderer_state;
    U32 current_frame_in_flight_index = context->current_frame_in_flight_index;

    vkWaitForFences(context->logical_device,
                    1, &context->frame_in_flight_fences[current_frame_in_flight_index],
                    VK_TRUE, UINT64_MAX);

    begin_temprary_memory_arena(&context->frame_arena, &context->arena);

    Globals globals = {};
    globals.view = scene_data->view;
    globals.projection = scene_data->projection;
    globals.projection[1][1] *= -1;
    globals.directional_light_direction = glm::vec4(scene_data->directional_light.direction, 0.0f);
    globals.directional_light_color = srgb_to_linear(scene_data->directional_light.color) * scene_data->directional_light.intensity;

    Buffer *global_uniform_buffer = get(&renderer_state->buffers, renderer_state->globals_uniform_buffers[current_frame_in_flight_index]);
    memcpy(global_uniform_buffer->data, &globals, sizeof(Globals));

    Buffer *object_data_storage_buffer = get(&renderer_state->buffers, renderer_state->object_data_storage_buffers[current_frame_in_flight_index]);

    context->object_data_base = (Object_Data *)object_data_storage_buffer->data;
    context->object_data_count = 0;

    U32 width = renderer_state->back_buffer_width;
    U32 height = renderer_state->back_buffer_height;

    if ((width != context->swapchain.width || height != context->swapchain.height) && width != 0 && height != 0)
    {
        recreate_swapchain(context,
                           &context->swapchain,
                           width,
                           height,
                           context->swapchain.present_mode);
    }

    VkResult result = vkAcquireNextImageKHR(context->logical_device,
                                            context->swapchain.handle,
                                            UINT64_MAX,
                                            context->image_available_semaphores[current_frame_in_flight_index],
                                            VK_NULL_HANDLE,
                                            &context->current_swapchain_image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        if (width != 0 && height != 0)
        {
            recreate_swapchain(context,
                               &context->swapchain,
                               width,
                               height,
                               context->swapchain.present_mode);

        }
    }
    else
    {
        HE_ASSERT(result == VK_SUCCESS);
    }

    vkResetFences(context->logical_device, 1, &context->frame_in_flight_fences[current_frame_in_flight_index]);

    VkCommandBuffer command_buffer = context->graphics_command_buffers[current_frame_in_flight_index];
    vkResetCommandBuffer(command_buffer, 0);

    VkCommandBufferBeginInfo command_buffer_begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    command_buffer_begin_info.flags = 0;
    command_buffer_begin_info.pInheritanceInfo = 0;

    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    VkClearValue clear_values[3] = {};
    clear_values[0].color = { 1.0f, 0.0f, 1.0f, 1.0f };
    clear_values[1].color = { 1.0f, 0.0f, 1.0f, 1.0f };
    clear_values[2].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo render_pass_begin_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    render_pass_begin_info.renderPass = context->render_pass;
    render_pass_begin_info.framebuffer = context->swapchain.frame_buffers[context->current_swapchain_image_index];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { context->swapchain.width, context->swapchain.height };
    render_pass_begin_info.clearValueCount = HE_ARRAYCOUNT(clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    for (U32 frame_index = 0; frame_index < HE_MAX_FRAMES_IN_FLIGHT; frame_index++)
    {
        Vulkan_Buffer *globals_uniform_buffer = &context->buffers[renderer_state->globals_uniform_buffers[frame_index].index];

        VkDescriptorBufferInfo globals_uniform_buffer_descriptor_info = {};
        globals_uniform_buffer_descriptor_info.buffer = globals_uniform_buffer->handle;
        globals_uniform_buffer_descriptor_info.offset = 0;
        globals_uniform_buffer_descriptor_info.range = sizeof(Globals);

        VkWriteDescriptorSet globals_uniform_buffer_write_descriptor_set = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        globals_uniform_buffer_write_descriptor_set.dstSet = context->descriptor_sets[0][frame_index];
        globals_uniform_buffer_write_descriptor_set.dstBinding = 0;
        globals_uniform_buffer_write_descriptor_set.dstArrayElement = 0;
        globals_uniform_buffer_write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        globals_uniform_buffer_write_descriptor_set.descriptorCount = 1;
        globals_uniform_buffer_write_descriptor_set.pBufferInfo = &globals_uniform_buffer_descriptor_info;

        Vulkan_Buffer *object_data_storage_buffer = &context->buffers[renderer_state->object_data_storage_buffers[frame_index].index];

        VkDescriptorBufferInfo object_data_storage_buffer_descriptor_info = {};
        object_data_storage_buffer_descriptor_info.buffer = object_data_storage_buffer->handle;
        object_data_storage_buffer_descriptor_info.offset = 0;
        object_data_storage_buffer_descriptor_info.range = sizeof(Object_Data) * HE_MAX_OBJECT_DATA_COUNT;

        VkWriteDescriptorSet object_data_storage_buffer_write_descriptor_set = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        object_data_storage_buffer_write_descriptor_set.dstSet = context->descriptor_sets[0][frame_index];
        object_data_storage_buffer_write_descriptor_set.dstBinding = 1;
        object_data_storage_buffer_write_descriptor_set.dstArrayElement = 0;
        object_data_storage_buffer_write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        object_data_storage_buffer_write_descriptor_set.descriptorCount = 1;
        object_data_storage_buffer_write_descriptor_set.pBufferInfo = &object_data_storage_buffer_descriptor_info;

        VkWriteDescriptorSet write_descriptor_sets[] =
        {
            globals_uniform_buffer_write_descriptor_set,
            object_data_storage_buffer_write_descriptor_set
        };

        vkUpdateDescriptorSets(context->logical_device,
                                HE_ARRAYCOUNT(write_descriptor_sets),
                                write_descriptor_sets, 0, nullptr);
    }

    VkDescriptorImageInfo *descriptor_image_infos = HE_ALLOCATE_ARRAY(&context->frame_arena, VkDescriptorImageInfo, HE_MAX_TEXTURE_COUNT);
    Vulkan_Sampler *default_sampler = &context->samplers[renderer_state->default_sampler.index];

    for (U32 texture_index = 0;
         texture_index < renderer_state->textures.capacity;
         texture_index++)
    {
        Texture *texture = nullptr;
        Vulkan_Image *vulkan_image = nullptr;

        if (renderer_state->textures.is_allocated[texture_index])
        {
            texture = &renderer_state->textures.data[texture_index];
            vulkan_image = &context->textures[texture_index];
        }
        else
        {
            texture = &renderer_state->textures.data[renderer_state->white_pixel_texture.index];
            vulkan_image = &context->textures[renderer_state->white_pixel_texture.index];
        }

        if (texture->data)
        {
            if (vkWaitForFences(context->logical_device, 1, &vulkan_image->is_loaded, VK_FALSE, 0) == VK_SUCCESS)
            {
                deallocate(&renderer_state->transfer_allocator, texture->data);
                texture->data = nullptr;
            }
        }

        descriptor_image_infos[texture_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptor_image_infos[texture_index].imageView = vulkan_image->view;
        descriptor_image_infos[texture_index].sampler = default_sampler->handle;
    }

    VkWriteDescriptorSet write_descriptor_set = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write_descriptor_set.dstSet = context->descriptor_sets[1][current_frame_in_flight_index];
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.descriptorCount = renderer_state->textures.capacity;
    write_descriptor_set.pImageInfo = descriptor_image_infos;

    vkUpdateDescriptorSets(context->logical_device, 1, &write_descriptor_set, 0, nullptr);

    VkDescriptorSet descriptor_sets[] =
    {
        context->descriptor_sets[0][current_frame_in_flight_index],
        context->descriptor_sets[1][current_frame_in_flight_index]
    };

    Vulkan_Pipeline_State *mesh_pipeline = &context->pipeline_states[renderer_state->mesh_pipeline.index];

    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mesh_pipeline->layout,
                            0, HE_ARRAYCOUNT(descriptor_sets),
                            descriptor_sets,
                            0, nullptr);

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (F32)context->swapchain.width;
    viewport.height = (F32)context->swapchain.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = context->swapchain.width;
    scissor.extent.height = context->swapchain.height;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    Vulkan_Buffer *position_buffer = &context->buffers[renderer_state->position_buffer.index];
    Vulkan_Buffer *normal_buffer = &context->buffers[renderer_state->normal_buffer.index];
    Vulkan_Buffer *uv_buffer = &context->buffers[renderer_state->uv_buffer.index];
    Vulkan_Buffer *tangent_buffer = &context->buffers[renderer_state->tangent_buffer.index];

    // todo(amer): this should be outside
    VkBuffer vertex_buffers[] =
    {
        position_buffer->handle,
        normal_buffer->handle,
        uv_buffer->handle,
        tangent_buffer->handle
    };

    VkDeviceSize offsets[] = { 0, 0, 0, 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, HE_ARRAYCOUNT(vertex_buffers), vertex_buffers, offsets);
    
    Vulkan_Buffer *index_buffer = &context->buffers[renderer_state->index_buffer.index];
    vkCmdBindIndexBuffer(command_buffer, index_buffer->handle, 0, VK_INDEX_TYPE_UINT16);
}

void vulkan_renderer_set_vertex_buffers(Buffer_Handle *vertex_buffer_handles, U64 *offsets, U32 count)
{
    Vulkan_Context *context = &vulkan_context;

    U32 current_frame_in_flight_index = context->current_frame_in_flight_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[current_frame_in_flight_index];

    VkBuffer *vulkan_vertex_buffers = HE_ALLOCATE_ARRAY(&context->frame_arena, VkBuffer, count);
    for (U32 vertex_buffer_index = 0; vertex_buffer_index < count; vertex_buffer_index++)
    {
        Buffer_Handle vertex_buffer_handle = vertex_buffer_handles[vertex_buffer_index];
        Vulkan_Buffer *vulkan_vertex_buffer = &context->buffers[vertex_buffer_handle.index];
        vulkan_vertex_buffers[vertex_buffer_index] = vulkan_vertex_buffer->handle;
    }

    vkCmdBindVertexBuffers(command_buffer, 0, count, vulkan_vertex_buffers, offsets);
}

void vulkan_renderer_set_index_buffer(Buffer_Handle index_buffer_handle, U64 offset)
{
    Vulkan_Context *context = &vulkan_context;

    U32 current_frame_in_flight_index = context->current_frame_in_flight_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[current_frame_in_flight_index];

    Vulkan_Buffer *vulkan_index_buffer = &context->buffers[index_buffer_handle.index];
    vkCmdBindIndexBuffer(command_buffer, vulkan_index_buffer->handle, offset, VK_INDEX_TYPE_UINT16);
}

void vulkan_renderer_submit_static_mesh(Static_Mesh_Handle static_mesh_handle, const glm::mat4 &transform)
{
    Vulkan_Context *context = &vulkan_context;
    Renderer_State *renderer_state = &context->engine->renderer_state;

    HE_ASSERT(context->object_data_count < HE_MAX_OBJECT_DATA_COUNT);
    U32 object_data_index = context->object_data_count++;
    Object_Data *object_data = &context->object_data_base[object_data_index];
    object_data->model = transform;
    U32 current_frame_in_flight_index = context->current_frame_in_flight_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[current_frame_in_flight_index];

    Vulkan_Static_Mesh *vulkan_static_mesh = &context->static_meshes[static_mesh_handle.index];

    Static_Mesh *static_mesh = get(&renderer_state->static_meshes, static_mesh_handle);
    Material *material = get(&renderer_state->materials, static_mesh->material_handle);
    Vulkan_Material *vulkan_material = &context->materials[static_mesh->material_handle.index];

    Vulkan_Buffer *material_buffer = &vulkan_material->buffers[current_frame_in_flight_index];
    copy_memory(material_buffer->data, material->data, material->size);

    Vulkan_Pipeline_State *vulkan_pipeline_state = &context->pipeline_states[material->pipeline_state_handle.index];
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_pipeline_state->handle);

    VkDescriptorSet descriptor_sets[] =
    {
        vulkan_material->descriptor_sets[current_frame_in_flight_index]
    };

    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            vulkan_pipeline_state->layout,
                            2, HE_ARRAYCOUNT(descriptor_sets),
                            descriptor_sets,
                            0, nullptr);

    U32 instance_count = 1;
    U32 start_instance = object_data_index;
    U32 first_index = vulkan_static_mesh->first_index;
    S32 first_vertex = vulkan_static_mesh->first_vertex;

    vkCmdDrawIndexed(command_buffer, static_mesh->index_count, instance_count,
                     first_index, first_vertex, start_instance);
}

void vulkan_renderer_end_frame()
{
    Vulkan_Context *context = &vulkan_context;
    Renderer_State *renderer_state = &context->engine->renderer_state;

    U32 current_frame_in_flight_index = context->current_frame_in_flight_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[current_frame_in_flight_index];

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((F32)(renderer_state->back_buffer_width),
                            (F32)(renderer_state->back_buffer_height));

    if (renderer_state->engine->imgui_docking)
    {
        ImGui::End();
    }

    ImGui::Render();

    if (renderer_state->engine->show_imgui)
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
    }

    vkCmdEndRenderPass(command_buffer);
    vkEndCommandBuffer(command_buffer);

    VkPipelineStageFlags wait_stage =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

    submit_info.pWaitDstStageMask = &wait_stage;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context->image_available_semaphores[current_frame_in_flight_index];

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context->rendering_finished_semaphores[current_frame_in_flight_index];

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(context->graphics_queue, 1, &submit_info, context->frame_in_flight_fences[current_frame_in_flight_index]);

    if (io.ConfigFlags&ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &context->rendering_finished_semaphores[current_frame_in_flight_index];

    present_info.swapchainCount = 1;
    present_info.pSwapchains = &context->swapchain.handle;
    present_info.pImageIndices = &context->current_swapchain_image_index;

    VkResult result = vkQueuePresentKHR(context->present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        if (renderer_state->back_buffer_width != 0 && renderer_state->back_buffer_height != 0)
        {
            recreate_swapchain(context,
                               &context->swapchain,
                               renderer_state->back_buffer_width,
                               renderer_state->back_buffer_height,
                               context->swapchain.present_mode);
        }
    }
    else
    {
        HE_ASSERT(result == VK_SUCCESS);
    }

    context->current_frame_in_flight_index++;
    if (context->current_frame_in_flight_index == context->frames_in_flight)
    {
        context->current_frame_in_flight_index = 0;
    }

    end_temprary_memory_arena(&context->frame_arena);
}

bool vulkan_renderer_create_texture(Texture_Handle texture_handle, const Texture_Descriptor &descriptor)
{
    Vulkan_Context *context = &vulkan_context;
    Renderer_State *renderer_state = &context->engine->renderer_state;
    Texture *texture = get(&renderer_state->textures, texture_handle);
    Vulkan_Image *image = &context->textures[texture_handle.index];

    HE_ASSERT(descriptor.format == Texture_Format::RGBA); // todo(amer): only supporting RGBA for now.
    create_image(image, context, descriptor.width, descriptor.height,
                 VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_IMAGE_ASPECT_COLOR_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, descriptor.mipmapping);
    
    // todo(amer): only supporting RGBA for now.
    U64 size = (U64)descriptor.width * (U64)descriptor.height * sizeof(U32);
    U64 transfered_data_offset = (U8 *)descriptor.data - renderer_state->transfer_allocator.base;
    
    Vulkan_Buffer *transfer_buffer = &context->buffers[renderer_state->transfer_buffer.index];
    copy_data_to_image_from_buffer(context, image, descriptor.width, descriptor.height, transfer_buffer, transfered_data_offset, size);

    texture->width = descriptor.width;
    texture->height = descriptor.height;
    texture->data = descriptor.data;
    return true;
}

void vulkan_renderer_destroy_texture(Texture_Handle texture_handle)
{
    Vulkan_Context *context = &vulkan_context;
    Vulkan_Image *vulkan_image = &context->textures[texture_handle.index];
    destroy_image(vulkan_image, &vulkan_context);
}

static VkSamplerAddressMode get_address_mode(Address_Mode address_mode)
{
    switch (address_mode)
    {
        case Address_Mode::REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case Address_Mode::CLAMP: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        default:
        {
            HE_ASSERT(!"unsupported address mode");
        } break;
    }

    return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
}

static VkFilter get_filter(Filter filter)
{
    switch (filter)
    {
        case Filter::NEAREST: return VK_FILTER_NEAREST;
        case Filter::LINEAR: return VK_FILTER_LINEAR;

        default:
        {
            HE_ASSERT(!"unsupported filter");
        } break;
    }

    return VK_FILTER_MAX_ENUM;
}

static VkSamplerMipmapMode get_mipmap_mode(Filter filter)
{
    switch (filter)
    {
        case Filter::NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case Filter::LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;

        default:
        {
            HE_ASSERT(!"unsupported filter");
        } break;
    }

    return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
}

bool vulkan_renderer_create_sampler(Sampler_Handle sampler_handle, const Sampler_Descriptor &descriptor)
{
    Vulkan_Context *context = &vulkan_context;
    Sampler *sampler = get(&context->engine->renderer_state.samplers, sampler_handle);
    Vulkan_Sampler *vulkan_sampler = &context->samplers[sampler_handle.index];

    VkSamplerCreateInfo sampler_create_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_create_info.minFilter = get_filter(descriptor.min_filter);
    sampler_create_info.magFilter = get_filter(descriptor.mag_filter);
    sampler_create_info.addressModeU = get_address_mode(descriptor.address_mode_u);
    sampler_create_info.addressModeV = get_address_mode(descriptor.address_mode_v);
    sampler_create_info.addressModeW = get_address_mode(descriptor.address_mode_w);
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode = get_mipmap_mode(descriptor.mip_filter);
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 16.0f;

    if (descriptor.anisotropic_filtering)
    {
        sampler_create_info.anisotropyEnable = VK_TRUE;
        sampler_create_info.maxAnisotropy = context->physical_device_properties.limits.maxSamplerAnisotropy;
    }

    HE_CHECK_VKRESULT(vkCreateSampler(context->logical_device, &sampler_create_info, nullptr, &vulkan_sampler->handle));
    sampler->descriptor = descriptor;
    return true;
}

void vulkan_renderer_destroy_sampler(Sampler_Handle sampler_handle)
{
    Vulkan_Context *context = &vulkan_context;
    Sampler *sampler = get(&context->engine->renderer_state.samplers, sampler_handle);
    Vulkan_Sampler *vulkan_sampler = &context->samplers[sampler_handle.index];
    vkDestroySampler(context->logical_device, vulkan_sampler->handle, nullptr);
}

bool vulkan_renderer_create_shader(Shader_Handle shader_handle, const Shader_Descriptor &descriptor)
{
    Vulkan_Context *context = &vulkan_context;
    bool loaded = load_shader(shader_handle, descriptor.path, context);
    return loaded;
}

void vulkan_renderer_destroy_shader(Shader_Handle shader_handle)
{
    Vulkan_Context *context = &vulkan_context;
    destroy_shader(shader_handle, context);
}

bool vulkan_renderer_create_pipeline_state(Pipeline_State_Handle pipeline_state_handle, const Pipeline_State_Descriptor &descriptor)
{
    Vulkan_Context *context = &vulkan_context;
    return create_graphics_pipeline(pipeline_state_handle, descriptor.shaders, context->render_pass, context);
}

void vulkan_renderer_destroy_pipeline_state(Pipeline_State_Handle pipeline_state_handle)
{
    Vulkan_Context *context = &vulkan_context;
    destroy_pipeline(pipeline_state_handle, context);
}

bool vulkan_renderer_create_material(Material_Handle material_handle, const Material_Descriptor &descriptor)
{
    Vulkan_Context *context = &vulkan_context;
    Material *material = get(&context->engine->renderer_state.materials, material_handle);
    Vulkan_Material *vulkan_material = &context->materials[material_handle.index];
    Pipeline_State *pipeline_state = get(&context->engine->renderer_state.pipeline_states, descriptor.pipeline_state_handle);
    Vulkan_Pipeline_State *vulkan_pipeline_state = &context->pipeline_states[descriptor.pipeline_state_handle.index];
    Shader_Struct *properties = nullptr;

    for (U32 shader_index = 0; shader_index < pipeline_state->shader_count; shader_index++)
    {
        Shader *shader = get(&context->engine->renderer_state.shaders, pipeline_state->shaders[shader_index]);
        for (U32 struct_index = 0; struct_index < shader->struct_count; struct_index++)
        {
            Shader_Struct *shader_struct = &shader->structs[struct_index];
            if (shader_struct->name == "Material_Properties")
            {
                properties = shader_struct;
                break;
            }
        }
    }

    HE_ASSERT(properties);

    Shader_Struct_Member *last_member = &properties->members[properties->member_count - 1];
    U32 last_member_size = get_size_of_shader_data_type(last_member->data_type);
    U32 size = last_member->offset + last_member_size;

    VkDescriptorSetLayout level2_descriptor_set_layouts[HE_MAX_FRAMES_IN_FLIGHT] = {};

    for (U32 frame_index = 0; frame_index < HE_MAX_FRAMES_IN_FLIGHT; frame_index++)
    {
        Vulkan_Buffer *buffer = &vulkan_material->buffers[frame_index];
        create_buffer(buffer, context, size,
                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        level2_descriptor_set_layouts[frame_index] = vulkan_pipeline_state->descriptor_set_layouts[2];
    }

    VkDescriptorSetAllocateInfo descriptor_set_allocation_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    descriptor_set_allocation_info.descriptorPool = context->descriptor_pool;
    descriptor_set_allocation_info.descriptorSetCount = HE_MAX_FRAMES_IN_FLIGHT;
    descriptor_set_allocation_info.pSetLayouts = level2_descriptor_set_layouts;

    HE_CHECK_VKRESULT(vkAllocateDescriptorSets(context->logical_device,
                                               &descriptor_set_allocation_info,
                                               vulkan_material->descriptor_sets));

    for (U32 frame_index = 0; frame_index < HE_MAX_FRAMES_IN_FLIGHT; frame_index++)
    {
        VkDescriptorBufferInfo material_uniform_buffer_descriptor_info = {};
        material_uniform_buffer_descriptor_info.buffer = vulkan_material->buffers[frame_index].handle;
        material_uniform_buffer_descriptor_info.offset = 0;
        material_uniform_buffer_descriptor_info.range = size;

        VkWriteDescriptorSet material_uniform_buffer_write_descriptor_set = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        material_uniform_buffer_write_descriptor_set.dstSet = vulkan_material->descriptor_sets[frame_index];
        material_uniform_buffer_write_descriptor_set.dstBinding = 0;
        material_uniform_buffer_write_descriptor_set.dstArrayElement = 0;
        material_uniform_buffer_write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        material_uniform_buffer_write_descriptor_set.descriptorCount = 1;
        material_uniform_buffer_write_descriptor_set.pBufferInfo = &material_uniform_buffer_descriptor_info;

        VkWriteDescriptorSet write_descriptor_sets[] =
        {
            material_uniform_buffer_write_descriptor_set,
        };

        vkUpdateDescriptorSets(context->logical_device,
                               HE_ARRAYCOUNT(write_descriptor_sets),
                               write_descriptor_sets, 0, nullptr);
    }

    material->pipeline_state_handle = descriptor.pipeline_state_handle;
    material->data = HE_ALLOCATE_ARRAY(context->allocator, U8, size);
    material->size = size;

    material->properties = properties;

    return true;
}

void vulkan_renderer_destroy_material(Material_Handle material_handle)
{
    Vulkan_Context *context = &vulkan_context;
    Vulkan_Material *vulkan_material = &context->materials[material_handle.index];
    for (U32 frame_index = 0; frame_index < HE_MAX_FRAMES_IN_FLIGHT; frame_index++)
    {
        destroy_buffer(&vulkan_material->buffers[frame_index], context->logical_device);
    }
}

static VkBufferUsageFlags get_buffer_usage(Buffer_Usage usage)
{
    switch (usage)
    {
        case Buffer_Usage::TRANSFER: return 0;
        case Buffer_Usage::VERTEX:   return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        case Buffer_Usage::INDEX:    return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case Buffer_Usage::UNIFORM:  return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case Buffer_Usage::STORAGE:  return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        default:
        {
            HE_ASSERT(!"unsupported buffer usage");
        } break;
    }

    return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
}

bool vulkan_renderer_create_buffer(Buffer_Handle buffer_handle, const Buffer_Descriptor &descriptor)
{
    HE_ASSERT(descriptor.size);
    Vulkan_Context *context = &vulkan_context;

    Buffer *buffer = get(&context->engine->renderer_state.buffers, buffer_handle);
    Vulkan_Buffer *vulkan_buffer = &context->buffers[buffer_handle.index];

    VkBufferUsageFlags usage = get_buffer_usage(descriptor.usage);
    VkMemoryPropertyFlags memory_property_flags = 0;

    if (descriptor.is_device_local)
    {
        usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        memory_property_flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    else
    {
        usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        memory_property_flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    VkBufferCreateInfo buffer_create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_create_info.size = descriptor.size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.flags = 0;

    HE_CHECK_VKRESULT(vkCreateBuffer(context->logical_device, &buffer_create_info, nullptr, &vulkan_buffer->handle));

    VkMemoryRequirements memory_requirements = {};
    vkGetBufferMemoryRequirements(context->logical_device, vulkan_buffer->handle, &memory_requirements);

    S32 memory_type_index = find_memory_type_index(context, memory_requirements, memory_property_flags);
    HE_ASSERT(memory_type_index != -1);

    VkMemoryAllocateInfo memory_allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;

    HE_CHECK_VKRESULT(vkAllocateMemory(context->logical_device, &memory_allocate_info, nullptr, &vulkan_buffer->memory));
    HE_CHECK_VKRESULT(vkBindBufferMemory(context->logical_device, vulkan_buffer->handle, vulkan_buffer->memory, 0));

    if ((memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        vkMapMemory(context->logical_device, vulkan_buffer->memory, 0, descriptor.size, 0, &buffer->data);
    }

    buffer->size = memory_requirements.size;
    return true;
}

void vulkan_renderer_destroy_buffer(Buffer_Handle buffer_handle)
{
    Vulkan_Context *context = &vulkan_context;
    Vulkan_Buffer *vulkan_buffer = &context->buffers[buffer_handle.index];

    vkFreeMemory(context->logical_device, vulkan_buffer->memory, nullptr);
    vkDestroyBuffer(context->logical_device, vulkan_buffer->handle, nullptr);
}

bool vulkan_renderer_create_static_mesh(Static_Mesh_Handle static_mesh_handle, const Static_Mesh_Descriptor &descriptor)
{
    Vulkan_Context *context = &vulkan_context;
    Renderer_State *renderer_state = &context->engine->renderer_state;
    Static_Mesh *static_mesh = get(&renderer_state->static_meshes, static_mesh_handle);

    U64 position_size = descriptor.vertex_count * sizeof(glm::vec3);
    U64 normal_size = descriptor.vertex_count * sizeof(glm::vec3);
    U64 uv_size = descriptor.vertex_count * sizeof(glm::vec2);
    U64 tangent_size = descriptor.vertex_count * sizeof(glm::vec4);
    U64 index_size = descriptor.index_count * sizeof(U16);

    HE_ASSERT(renderer_state->vertex_count + descriptor.vertex_count <= renderer_state->max_vertex_count);
    static_mesh->index_count = descriptor.index_count;
    static_mesh->vertex_count = descriptor.vertex_count;

    Vulkan_Static_Mesh *vulkan_static_mesh = &context->static_meshes[static_mesh_handle.index];

    VkFenceCreateInfo fence_create_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    HE_CHECK_VKRESULT(vkCreateFence(context->logical_device, &fence_create_info, nullptr, &vulkan_static_mesh->is_loaded));

    U64 position_offset = (U8 *)descriptor.positions - renderer_state->transfer_allocator.base;
    U64 normal_offset = (U8 *)descriptor.normals - renderer_state->transfer_allocator.base;
    U64 uv_offset = (U8 *)descriptor.uvs - renderer_state->transfer_allocator.base;
    U64 tangent_offset = (U8 *)descriptor.tangents - renderer_state->transfer_allocator.base;
    U64 indicies_offset = (U8 *)descriptor.indices - renderer_state->transfer_allocator.base;

    VkCommandBufferAllocateInfo command_buffer_allocate_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    command_buffer_allocate_info.commandPool = context->transfer_command_pool;
    command_buffer_allocate_info.commandBufferCount = 1;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer command_buffer = {};
    vkAllocateCommandBuffers(context->logical_device, &command_buffer_allocate_info, &command_buffer); // @Leak
    vkResetCommandBuffer(command_buffer, 0);

    VkCommandBufferBeginInfo command_buffer_begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    command_buffer_begin_info.flags = 0;
    command_buffer_begin_info.pInheritanceInfo = 0;

    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    Vulkan_Buffer *transfer_buffer = &context->buffers[renderer_state->transfer_buffer.index];
    Vulkan_Buffer *position_buffer = &context->buffers[renderer_state->position_buffer.index];
    Vulkan_Buffer *normal_buffer = &context->buffers[renderer_state->normal_buffer.index];
    Vulkan_Buffer *uv_buffer = &context->buffers[renderer_state->uv_buffer.index];
    Vulkan_Buffer *tangent_buffer = &context->buffers[renderer_state->tangent_buffer.index];

    VkBufferCopy position_copy_region = {};
    position_copy_region.srcOffset = position_offset;
    position_copy_region.dstOffset = renderer_state->vertex_count * sizeof(glm::vec3);
    position_copy_region.size = position_size;
    vkCmdCopyBuffer(command_buffer, transfer_buffer->handle, position_buffer->handle, 1, &position_copy_region);

    VkBufferCopy normal_copy_region = {};
    normal_copy_region.srcOffset = normal_offset;
    normal_copy_region.dstOffset = renderer_state->vertex_count * sizeof(glm::vec3);
    normal_copy_region.size = normal_size;
    vkCmdCopyBuffer(command_buffer, transfer_buffer->handle, normal_buffer->handle, 1, &normal_copy_region);

    VkBufferCopy uv_copy_region = {};
    uv_copy_region.srcOffset = uv_offset;
    uv_copy_region.dstOffset = renderer_state->vertex_count * sizeof(glm::vec2);
    uv_copy_region.size = uv_size;
    vkCmdCopyBuffer(command_buffer, transfer_buffer->handle, uv_buffer->handle, 1, &uv_copy_region);

    VkBufferCopy tangent_copy_region = {};
    tangent_copy_region.srcOffset = tangent_offset;
    tangent_copy_region.dstOffset = renderer_state->vertex_count * sizeof(glm::vec4);
    tangent_copy_region.size = tangent_size;
    vkCmdCopyBuffer(command_buffer, transfer_buffer->handle, tangent_buffer->handle, 1, &tangent_copy_region);

    Vulkan_Buffer *index_buffer = &context->buffers[renderer_state->index_buffer.index];

    VkBufferCopy index_copy_region = {};
    index_copy_region.srcOffset = indicies_offset;
    index_copy_region.dstOffset = renderer_state->index_offset;
    index_copy_region.size = index_size;
    vkCmdCopyBuffer(command_buffer, transfer_buffer->handle, index_buffer->handle, 1, &index_copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    vkQueueSubmit(context->transfer_queue, 1, &submit_info, vulkan_static_mesh->is_loaded);

    vulkan_static_mesh->first_vertex = (S32)u64_to_u32(renderer_state->vertex_count);
    vulkan_static_mesh->first_index = u64_to_u32(renderer_state->index_offset / sizeof(U16));

    renderer_state->vertex_count += descriptor.vertex_count;
    renderer_state->index_offset += index_size;
    return true;
}

// todo(amer): static mesh allocator...
void vulkan_renderer_destroy_static_mesh(Static_Mesh_Handle static_mesh_handle)
{
    Vulkan_Context *context = &vulkan_context;
    Vulkan_Static_Mesh *vulkan_static_mesh = &context->static_meshes[static_mesh_handle.index];
    vkDestroyFence(context->logical_device, vulkan_static_mesh->is_loaded, nullptr);
}