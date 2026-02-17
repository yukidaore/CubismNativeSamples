/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "VulkanManager.hpp"
#include "SwapchainManager.hpp"
#include "LAppPal.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <set>
#include <string>
#include <cstring>

const char* const VulkanManager::deviceExtensions[2] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME
};

const char* const VulkanManager::validationLayers[1] = {
    "VK_LAYER_KHRONOS_validation"
};

VulkanManager* VulkanManager::s_instance = nullptr;

VulkanManager* VulkanManager::GetInstance()
{
    return s_instance;
}

void VulkanManager::Create(SDL_Window* window)
{
    if (!s_instance)
    {
        s_instance = new VulkanManager(window);
    }
}

void VulkanManager::Delete()
{
    if (s_instance)
    {
        delete s_instance;
        s_instance = nullptr;
    }
}

VulkanManager::VulkanManager(SDL_Window* window)
    : _window(window)
    , _instance(VK_NULL_HANDLE)
    , _surface(VK_NULL_HANDLE)
    , _physicalDevice(VK_NULL_HANDLE)
    , _device(VK_NULL_HANDLE)
    , _graphicQueue(VK_NULL_HANDLE)
    , _presentQueue(VK_NULL_HANDLE)
    , _commandPool(VK_NULL_HANDLE)
    , _imageAvailableSemaphore(VK_NULL_HANDLE)
    , _swapchainManager(nullptr)
    , _debugMessenger(VK_NULL_HANDLE)
    , _depthFormat(VK_FORMAT_UNDEFINED)
{
}

VulkanManager::~VulkanManager()
{
    Destroy();
}

bool VulkanManager::CheckValidationLayerSupport()
{
    Csm::csmUint32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    Csm::csmVector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.GetPtr());

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;
        for (Csm::csmUint32 i = 0; i < availableLayers.GetSize(); i++)
        {
            if (strcmp(layerName, availableLayers[i].layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
        {
            return false;
        }
    }
    return true;
}

Csm::csmVector<const char*> VulkanManager::GetRequiredExtensions()
{
    Csm::csmVector<const char*> extensions;

    // SDL3 Vulkan extensions
    Csm::csmUint32 count = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&count);
    if (sdlExtensions)
    {
        for (Csm::csmUint32 i = 0; i < count; i++)
        {
            extensions.PushBack(sdlExtensions[i]);
        }
    }

    if (_enableValidationLayers)
    {
        extensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanManager::CreateInstance()
{
    if (_enableValidationLayers && !CheckValidationLayerSupport())
    {
        LAppPal::PrintLog("Validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Live2D Cubism SDL3 Sample";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    Csm::csmVector<const char*> extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.GetSize());
    createInfo.ppEnabledExtensionNames = extensions.GetPtr();

    if (_enableValidationLayers)
    {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validationLayers;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
    {
        LAppPal::PrintLog("Failed to create Vulkan instance!");
    }
}

void VulkanManager::CreateSurface()
{
    if (!SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface))
    {
        LAppPal::PrintLog("Failed to create Vulkan surface!");
    }
}

void VulkanManager::FindQueueFamilies(VkPhysicalDevice device)
{
    Csm::csmUint32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    Csm::csmVector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.GetPtr());

    for (Csm::csmUint32 i = 0; i < queueFamilies.GetSize(); i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }
    }
}

bool VulkanManager::CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
    Csm::csmUint32 extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    Csm::csmVector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.GetPtr());

    std::set<std::string> requiredExtensions;
    for (const char* ext : deviceExtensions)
    {
        requiredExtensions.insert(ext);
    }

    for (Csm::csmUint32 i = 0; i < availableExtensions.GetSize(); i++)
    {
        requiredExtensions.erase(availableExtensions[i].extensionName);
    }

    return requiredExtensions.empty();
}

bool VulkanManager::IsDeviceSuitable(VkPhysicalDevice device)
{
    FindQueueFamilies(device);
    bool extensionsSupported = CheckDeviceExtensionSupport(device);
    return indices.isComplete() && extensionsSupported;
}

void VulkanManager::PickPhysicalDevice()
{
    Csm::csmUint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        LAppPal::PrintLog("Failed to find GPUs with Vulkan support!");
        return;
    }

    Csm::csmVector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.GetPtr());

    for (Csm::csmUint32 i = 0; i < devices.GetSize(); i++)
    {
        if (IsDeviceSuitable(devices[i]))
        {
            _physicalDevice = devices[i];
            break;
        }
    }

    if (_physicalDevice == VK_NULL_HANDLE)
    {
        LAppPal::PrintLog("Failed to find a suitable GPU!");
    }
}

void VulkanManager::CreateLogicalDevice()
{
    std::set<Csm::csmInt32> uniqueQueueFamilies = {
        indices.graphicsFamily,
        indices.presentFamily
    };

    Csm::csmVector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    for (int queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.PushBack(queueCreateInfo);
    }

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures{};
    extendedDynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extendedDynamicStateFeatures.extendedDynamicState = VK_TRUE;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &extendedDynamicStateFeatures;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.GetSize());
    createInfo.pQueueCreateInfos = queueCreateInfos.GetPtr();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    if (_enableValidationLayers)
    {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validationLayers;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS)
    {
        LAppPal::PrintLog("Failed to create logical device!");
    }

    vkGetDeviceQueue(_device, indices.graphicsFamily, 0, &_graphicQueue);
    vkGetDeviceQueue(_device, indices.presentFamily, 0, &_presentQueue);
}

void VulkanManager::ChooseSupportedDepthFormat()
{
    VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            _depthFormat = format;
            return;
        }
    }

    _depthFormat = VK_FORMAT_D24_UNORM_S8_UINT; // fallback
}

void VulkanManager::CreateCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily;

    if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS)
    {
        LAppPal::PrintLog("Failed to create command pool!");
    }
}

void VulkanManager::CreateSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphore) != VK_SUCCESS)
    {
        LAppPal::PrintLog("Failed to create semaphore!");
    }
}

void VulkanManager::Initialize()
{
    CreateInstance();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    ChooseSupportedDepthFormat();

    int width, height;
    SDL_GetWindowSizeInPixels(_window, &width, &height);
    _swapchainManager = new SwapchainManager(_device, _physicalDevice, _surface, _surfaceFormat, _depthFormat,
                                              static_cast<Csm::csmUint32>(width), static_cast<Csm::csmUint32>(height));

    CreateCommandPool();
    CreateSyncObjects();
}

VkCommandBuffer VulkanManager::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanManager::SubmitCommand(VkCommandBuffer commandBuffer, bool isFirstDraw)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (isFirstDraw)
    {
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &_imageAvailableSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
    }

    vkQueueSubmit(_graphicQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_graphicQueue);

    vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}

void VulkanManager::UpdateDrawFrame()
{
    VkResult result = vkAcquireNextImageKHR(_device, _swapchainManager->GetSwapchain(), UINT64_MAX,
                                            _imageAvailableSemaphore, VK_NULL_HANDLE, &_imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapchain();
        return;
    }
}

void VulkanManager::PostDraw()
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 0;
    presentInfo.pWaitSemaphores = nullptr;

    VkSwapchainKHR swapChains[] = { _swapchainManager->GetSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &_imageIndex;

    VkResult result = vkQueuePresentKHR(_presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized)
    {
        _framebufferResized = false;
        RecreateSwapchain();
    }
}

void VulkanManager::RecreateSwapchain()
{
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(_window, &width, &height);
    while (width == 0 || height == 0)
    {
        SDL_GetWindowSizeInPixels(_window, &width, &height);
        SDL_Delay(10);
    }

    vkDeviceWaitIdle(_device);

    delete _swapchainManager;
    _swapchainManager = new SwapchainManager(_device, _physicalDevice, _surface, _surfaceFormat, _depthFormat,
                                              static_cast<Csm::csmUint32>(width), static_cast<Csm::csmUint32>(height));
    _isSwapchainInvalid = true;
}

void VulkanManager::Destroy()
{
    vkDeviceWaitIdle(_device);

    if (_swapchainManager)
    {
        delete _swapchainManager;
        _swapchainManager = nullptr;
    }

    if (_imageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(_device, _imageAvailableSemaphore, nullptr);
    }

    if (_commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(_device, _commandPool, nullptr);
    }

    if (_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(_device, nullptr);
    }

    if (_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
    }

    if (_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(_instance, nullptr);
    }
}

VkImage VulkanManager::GetSwapchainImage()
{
    return _swapchainManager->GetImages()[_imageIndex];
}

VkImageView VulkanManager::GetSwapchainImageView()
{
    return _swapchainManager->GetImageViews()[_imageIndex];
}

void VulkanManager::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                  VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        LAppPal::PrintLog("Failed to create buffer!");
        return;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        LAppPal::PrintLog("Failed to allocate buffer memory!");
        return;
    }

    vkBindBufferMemory(_device, buffer, bufferMemory, 0);
}

uint32_t VulkanManager::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    LAppPal::PrintLog("Failed to find suitable memory type!");
    return 0;
}

void VulkanManager::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    (void)format;
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    SubmitCommand(commandBuffer);
}

void VulkanManager::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    SubmitCommand(commandBuffer);
}
