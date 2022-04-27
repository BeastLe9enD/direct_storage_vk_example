#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <DirectStorage/dstorage.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#include <format>
#include <limits>
#include <string_view>
#include <stdexcept>
#include <system_error>
#include <vector>

#ifdef max
#undef max
#endif

void throw_if_failed(HRESULT result, const std::string_view& message) {
    if(FAILED(result)) {
        throw std::runtime_error(std::format("{} failed: {}", message, std::system_category().message(result)));
    }
}

void throw_if_failed(VkResult result, const std::string_view& message) {
    if(result != VK_SUCCESS) {
        throw std::runtime_error(std::format("{} failed: {}", message, static_cast<int>(result)));
    }
}

std::vector<int8_t> read_binary_file(const std::string_view& path) {
    auto* file = fopen(path.data(), "rb");
    if(!file) {
        throw std::runtime_error("fopen");
    }

    fseek(file, 0, SEEK_END);
    const auto length = ftell(file);
    if(!length) {
        throw std::runtime_error("ftell");
    }
    fseek(file, 0, SEEK_SET);

    std::vector<int8_t> buffer(length);
    const auto length_read = fread(buffer.data(), 1, length, file);
    if(length != length_read) {
        throw std::runtime_error("fread");
    }

    fclose(file);

    return buffer;
}

void create_shader_module(VkDevice device, const std::string_view& path, VkShaderStageFlagBits stage, std::vector<VkPipelineShaderStageCreateInfo>& pipeline_shader_stage_create_infos) {
    const auto bytes = read_binary_file(path);

    VkShaderModuleCreateInfo shader_module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = bytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(bytes.data())
    };

    VkShaderModule shader_module;
    throw_if_failed(vkCreateShaderModule(device, &shader_module_create_info, nullptr, &shader_module), "vkCreateShaderModule");

    pipeline_shader_stage_create_infos.push_back(VkPipelineShaderStageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = shader_module,
        .pName = "main"
    });
}

VkPipeline create_pipeline(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, VkPipelineLayout& pipeline_layout) {
    std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_infos;

    create_shader_module(device, "example.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, pipeline_shader_stage_create_infos);
    create_shader_module(device, "example.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, pipeline_shader_stage_create_infos);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout
    };

    throw_if_failed(vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline_layout), "vkCreatePipelineLayout");

    auto format = VK_FORMAT_B8G8R8A8_UNORM;

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format
    };

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    VkViewport viewport = {
        .width = 1600.0,
        .height = 900.0,
        .maxDepth = 1.0
    };

    VkRect2D scissor = {
        .extent = { .width = 1600, .height = 900 }
    };

    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0
    };

    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &pipeline_color_blend_attachment_state
    };

    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO
    };

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_rendering_create_info,
        .stageCount = static_cast<uint32_t>(pipeline_shader_stage_create_infos.size()),
        .pStages = pipeline_shader_stage_create_infos.data(),
        .pVertexInputState = &pipeline_vertex_input_state_create_info,
        .pInputAssemblyState = &pipeline_input_assembly_state_create_info,
        .pViewportState = &pipeline_viewport_state_create_info,
        .pRasterizationState = &pipeline_rasterization_state_create_info,
        .pMultisampleState = &pipeline_multisample_state_create_info,
        .pColorBlendState = &pipeline_color_blend_state_create_info,
        .pDynamicState = &pipeline_dynamic_state_create_info,
        .layout = pipeline_layout
    };

    VkPipeline pipeline;
    throw_if_failed(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &pipeline), "vkCreateGraphicsPipelines");

    for(const auto& pipeline_shader_stage_create_info : pipeline_shader_stage_create_infos) {
        vkDestroyShaderModule(device, pipeline_shader_stage_create_info.module, nullptr);
    }

    return pipeline;
}

uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t memory_type_bits, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);

    for (auto i = 0; i < physical_device_memory_properties.memoryTypeCount; i++) {
        if ((memory_type_bits & (1 << i)) && (physical_device_memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Invalid memory type!");
}

VkImage create_image(VkDevice device, ID3D12Device8* d3d12_device, IDStorageFactory* dstorage_factory, IDStorageQueue* dstorage_queue,
                     const std::wstring_view& path, VkDeviceMemory& memory, VkImageView& image_view) {
    uint32_t width = 2048, height = 2048;

    IDStorageFile* dstorage_file;
    throw_if_failed(dstorage_factory->OpenFile(path.data(), IID_PPV_ARGS(&dstorage_file)), "IDStorageFactory::OpenFile"); //TODO: release me

    BY_HANDLE_FILE_INFORMATION dstorage_file_information = {};
    throw_if_failed(dstorage_file->GetFileInformation(&dstorage_file_information), "IDStorageFile::GetFileInformation");

    D3D12_RESOURCE_DESC resource_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Width = width,
        .Height = height,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_NONE
    };

    D3D12_HEAP_PROPERTIES heap_properties = {
        .Type = D3D12_HEAP_TYPE_DEFAULT
    };

    ID3D12Resource* resource;
    throw_if_failed(d3d12_device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_SHARED, &resource_desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource)),
                    "ID3D12Device::CreateCommittedResource");

    HANDLE handle;
    throw_if_failed(d3d12_device->CreateSharedHandle(resource, nullptr, GENERIC_ALL, nullptr, &handle), "ID3D12Device::CreateSharedHandle");

    VkExternalMemoryImageCreateInfo external_memory_image_create_info = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT
    };

    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = &external_memory_image_create_info,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = { .width = width, .height = height, .depth = 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT
    };

    VkImage image;
    throw_if_failed(vkCreateImage(device, &image_create_info, nullptr, &image), "vkCreateImage");

    VkImportMemoryWin32HandleInfoKHR import_memory_win32_handle_info = {
        .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT,
        .handle = handle
    };

    VkMemoryAllocateInfo memory_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &import_memory_win32_handle_info
    };

    throw_if_failed(vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory), "vkAllocateMemory");
    CloseHandle(handle);

    throw_if_failed(vkBindImageMemory(device, image, memory, 0), "vkBindImageMemory");

    ID3D12Fence* fence;
    throw_if_failed(d3d12_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "ID3D12Device::CreateFence");

    auto fenceEvent = CreateEvent(nullptr, false, false, nullptr);
    if(!fenceEvent) {
        throw std::runtime_error("CreateEvent failed");
    }

    DSTORAGE_REQUEST request = {
        .Options = {
            .SourceType = DSTORAGE_REQUEST_SOURCE_FILE,
            .DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION,
        },
        .Source = {
            .File = {
                .Source = dstorage_file,
                .Size = dstorage_file_information.nFileSizeLow,
            }
        },
        .Destination = {
            .Texture = {
                .Resource = resource,
                .Region = {
                    .left = 0,
                    .top = 0,
                    .front = 0,
                    .right = width,
                    .bottom = height,
                    .back = 1
                }
            }
        },
        .UncompressedSize = dstorage_file_information.nFileSizeLow
    };

    dstorage_queue->EnqueueRequest(&request);

    dstorage_queue->EnqueueSignal(fence, 1);
    dstorage_queue->Submit();

    fence->SetEventOnCompletion(1, fenceEvent);
    if(fence->GetCompletedValue() < 1) {
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    CloseHandle(fenceEvent);
    fence->Release();

    dstorage_file->Release();

    VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .components = VkComponentMapping {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = VkImageSubresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };

    throw_if_failed(vkCreateImageView(device, &image_view_create_info, nullptr, &image_view), "vkCreateImageView");

    return image;
}

void init() {
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(std::format("{} failed: {}", "SDL_Init", SDL_GetError()));
    }

    auto* window = SDL_CreateWindow("direct_storage_vk", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_VULKAN);
    if(!window) {
        throw std::runtime_error(std::format("{} failed: {}", "SDL_CreateWindow", SDL_GetError()));
    }

    throw_if_failed(volkInitialize(), "volkInitialize");

    uint32_t num_sdl2_extensions;
    if(!SDL_Vulkan_GetInstanceExtensions(window, &num_sdl2_extensions, nullptr)) {
        throw std::runtime_error(std::format("{} failed: {}", "SDL_Vulkan_GetInstanceExtensions", SDL_GetError()));
    }

    std::vector<const char*> enabled_instance_extensions(num_sdl2_extensions);

    if(!SDL_Vulkan_GetInstanceExtensions(window, &num_sdl2_extensions, enabled_instance_extensions.data())) {
        throw std::runtime_error(std::format("{} failed: {}", "SDL_Vulkan_GetInstanceExtensions", SDL_GetError()));
    }

    VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "direct_storage_example",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pEngineName = "direct_storage_example",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    std::vector<const char*> enabled_instance_layers = { "VK_LAYER_KHRONOS_validation" };

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = static_cast<uint32_t>(enabled_instance_layers.size()),
        .ppEnabledLayerNames = enabled_instance_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(enabled_instance_extensions.size()),
        .ppEnabledExtensionNames = enabled_instance_extensions.data()
    };

    VkInstance instance;
    throw_if_failed(vkCreateInstance(&instance_create_info, nullptr, &instance), "vkCreateInstance");

    volkLoadInstance(instance);

    VkSurfaceKHR surface;
    if(!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
        throw std::runtime_error(std::format("{} failed: {}", "SDL_Vulkan_GetInstanceExtensions", SDL_GetError()));
    }

    uint32_t num_physical_devices;
    throw_if_failed(vkEnumeratePhysicalDevices(instance, &num_physical_devices, nullptr), "vkEnumeratePhysicalDevices");

    std::vector<VkPhysicalDevice> physical_devices(num_physical_devices);

    throw_if_failed(vkEnumeratePhysicalDevices(instance, &num_physical_devices, physical_devices.data()), "vkEnumeratePhysicalDevices");

    auto physical_device = physical_devices[0];

    VkPhysicalDeviceDynamicRenderingFeatures physical_device_dynamic_rendering_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = VK_TRUE
    };

    VkPhysicalDeviceFeatures2 physical_device_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &physical_device_dynamic_rendering_features
    };

    const float singlePriority = 1.0f;

    VkDeviceQueueCreateInfo device_queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount = 1,
        .pQueuePriorities = &singlePriority
    };

    std::vector<const char*> enabled_device_layers = {};
    std::vector<const char*> enabled_device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &physical_device_features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &device_queue_create_info,
        .enabledLayerCount = static_cast<uint32_t>(enabled_device_layers.size()),
        .ppEnabledLayerNames = enabled_device_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(enabled_device_extensions.size()),
        .ppEnabledExtensionNames = enabled_device_extensions.data()
    };

    VkDevice device;
    throw_if_failed(vkCreateDevice(physical_device, &device_create_info, nullptr, &device), "vkCreateDevice");

    volkLoadDevice(device);

    VkQueue queue;
    vkGetDeviceQueue(device, 0, 0, &queue);

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = 2,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = VkExtent2D { .width = 1600, .height = 900 },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR
    };

    VkSwapchainKHR swapchain;
    throw_if_failed(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain), "VkSwapchainKHR");

    uint32_t num_swapchain_images;
    throw_if_failed(vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, nullptr), "vkGetSwapchainImagesKHR");

    std::vector<VkImage> swapchain_images(num_swapchain_images);

    throw_if_failed(vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, swapchain_images.data()), "vkGetSwapchainImagesKHR");

    std::vector<VkImageView> swapchain_image_views(num_swapchain_images);
    for(auto i = 0; i < num_swapchain_images; i++) {
        VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .components = VkComponentMapping {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = VkImageSubresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };

        throw_if_failed(vkCreateImageView(device, &image_view_create_info, nullptr, &swapchain_image_views[i]), "vkCreateImageView");
    }

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkSemaphore present_semaphore, render_semaphore;
    throw_if_failed(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &present_semaphore), "vkCreateSemaphore");
    throw_if_failed(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_semaphore), "vkCreateSemaphore");

    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };

    VkFence fence;
    throw_if_failed(vkCreateFence(device, &fence_create_info, nullptr, &fence), "vkCreateFence");

    VkCommandPoolCreateInfo command_pool_craete_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    };

    VkCommandPool command_pool;
    throw_if_failed(vkCreateCommandPool(device, &command_pool_craete_info, nullptr, &command_pool), "vkCreateCommandPool");

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer);

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
        .bindingCount = 1,
        .pBindings = &descriptor_set_layout_binding
    };

    VkDescriptorSetLayout descriptor_set_layout;
    throw_if_failed(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout), "vkCreateDescriptorSetLayout");

    VkPipelineLayout pipeline_layout;
    auto pipeline = create_pipeline(device, descriptor_set_layout, pipeline_layout);

    ID3D12Device8* d3d12_device;
    throw_if_failed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3d12_device)), "D3D12CreateDevice");

    DSTORAGE_QUEUE_DESC queue_desc = {
        .SourceType = DSTORAGE_REQUEST_SOURCE_FILE,
        .Capacity = DSTORAGE_MAX_QUEUE_CAPACITY,
        .Priority = DSTORAGE_PRIORITY_NORMAL,
        .Device = d3d12_device
    };

    IDStorageFactory* dstorage_factory;
    throw_if_failed(DStorageGetFactory(IID_PPV_ARGS(&dstorage_factory)), "DStorageGetFactory");

    IDStorageQueue* dstorage_queue;
    throw_if_failed(dstorage_factory->CreateQueue(&queue_desc, IID_PPV_ARGS(&dstorage_queue)), "IDStorageFactory::CreateQueue");

    VkDeviceMemory image_memory;
    VkImageView image_view;
    auto image = create_image(device, d3d12_device, dstorage_factory, dstorage_queue, L"example.dds", image_memory, image_view);

    VkSamplerCreateInfo sampler_create_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
    };

    VkSampler sampler;
    throw_if_failed(vkCreateSampler(device, &sampler_create_info, nullptr, &sampler), "vkCreateSampler");

    bool running = true;
    SDL_Event ev;

    while(running) {
        while(SDL_PollEvent(&ev)) {
            if(ev.type == SDL_QUIT) {
                running = false;
            }
        }

        throw_if_failed(vkResetCommandBuffer(command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT), "vkResetCommandBuffer");
        throw_if_failed(vkResetCommandPool(device, command_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT), "vkResetCommandPool");

        uint32_t image_index;
        throw_if_failed(vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), present_semaphore, VK_NULL_HANDLE, &image_index), "vkAcquireNextImageKHR");

        VkCommandBufferBeginInfo command_buffer_begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        throw_if_failed(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info), "vkBeginCommandBuffer");

        VkImageMemoryBarrier image_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image = swapchain_images[image_index],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr,
                             1, &image_memory_barrier);

        VkRenderingAttachmentInfo rendering_attachment_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchain_image_views[image_index],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {
                .color = {
                    .float32 = { 100.0 / 255.0, 149.0 / 255.0, 237.0 / 255.0, 1.0 }
                }
            }
        };

        static bool imageTransitioned = false;
        VkImageMemoryBarrier image_image_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image = image,
            .subresourceRange = VkImageSubresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };

        if(!imageTransitioned) {
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                                 1, &image_image_memory_barrier);
            imageTransitioned = true;
        }

        VkRenderingInfo rendering_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {
                .extent = { 1600, 900 }
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &rendering_attachment_info
        };

        vkCmdBeginRendering(command_buffer, &rendering_info);

        VkDescriptorImageInfo descriptor_image_info = {
            .sampler = sampler,
            .imageView = image_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet write_descriptor_set = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &descriptor_image_info
        };

        vkCmdPushDescriptorSetKHR(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &write_descriptor_set);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdDraw(command_buffer, 6, 1, 0, 0);

        vkCmdEndRendering(command_buffer);

        image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        image_memory_barrier.dstAccessMask = 0;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr,
                             1, &image_memory_barrier);

        throw_if_failed(vkEndCommandBuffer(command_buffer), "vkEndCommandBuffer");

        VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &present_semaphore,
            .pWaitDstStageMask = &wait_dst_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_semaphore
        };

        throw_if_failed(vkQueueSubmit(queue, 1, &submit_info, fence), "vkQueueSubmit");

        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &image_index
        };

        throw_if_failed(vkQueuePresentKHR(queue, &present_info), "vkQueuePresentKHR");

        throw_if_failed(vkWaitForFences(device, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max()), "vkWaitForFences");
        throw_if_failed(vkResetFences(device, 1, &fence), "vkResetFences");
    }

    throw_if_failed(vkDeviceWaitIdle(device), "vkDeviceWaitIdle");

    vkDestroySampler(device, sampler, nullptr);

    vkDestroyImageView(device, image_view, nullptr);
    vkFreeMemory(device, image_memory, nullptr);
    vkDestroyImage(device, image, nullptr);

    dstorage_queue->Release();
    dstorage_factory->Release();
    d3d12_device->Release();

    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    vkDestroyCommandPool(device, command_pool, nullptr);

    vkDestroyFence(device, fence, nullptr);

    vkDestroySemaphore(device, render_semaphore, nullptr);
    vkDestroySemaphore(device, present_semaphore, nullptr);

    for(auto image_view : swapchain_image_views) {
        vkDestroyImageView(device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    SDL_DestroyWindow(window);

    SDL_Quit();
}

int main(int argc, char** args) {
    try {
        init();
    } catch(const std::exception& ex) {
        printf("%s\n", ex.what());
        return 1;
    }

    return 0;
}
