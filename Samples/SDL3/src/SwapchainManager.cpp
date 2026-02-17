/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "SwapchainManager.hpp"
#include "LAppPal.hpp"
#include <algorithm>

SwapchainManager::SwapchainManager(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                   VkFormat surfaceFormat, VkFormat depthFormat,
                                   Csm::csmUint32 width, Csm::csmUint32 height)
    : _device(device)
    , _physicalDevice(physicalDevice)
    , _surfaceFormat(surfaceFormat)
    , _depthFormat(depthFormat)
    , _swapchain(VK_NULL_HANDLE)
    , _depthImage(VK_NULL_HANDLE)
    , _depthImageMemory(VK_NULL_HANDLE)
    , _depthImageView(VK_NULL_HANDLE)
{
    _extent.width = width;
    _extent.height = height;

    // Query surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    // Choose extent
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        _extent = capabilities.currentExtent;
    }
    else
    {
        _extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        _extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    // Choose image count
    Csm::csmUint32 imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    // Create swapchain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = _extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &_swapchain) != VK_SUCCESS)
    {
        LAppPal::PrintLog("Failed to create swap chain!");
        return;
    }

    // Get swapchain images
    vkGetSwapchainImagesKHR(device, _swapchain, &imageCount, nullptr);
    _images.Resize(imageCount);
    vkGetSwapchainImagesKHR(device, _swapchain, &imageCount, _images.GetPtr());

    CreateImageViews();
    CreateDepthResources();
}

SwapchainManager::~SwapchainManager()
{
    if (_depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(_device, _depthImageView, nullptr);
    }
    if (_depthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(_device, _depthImage, nullptr);
    }
    if (_depthImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(_device, _depthImageMemory, nullptr);
    }

    for (Csm::csmUint32 i = 0; i < _imageViews.GetSize(); i++)
    {
        vkDestroyImageView(_device, _imageViews[i], nullptr);
    }

    if (_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    }
}

void SwapchainManager::CreateImageViews()
{
    _imageViews.Resize(_images.GetSize());

    for (Csm::csmUint32 i = 0; i < _images.GetSize(); i++)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = _surfaceFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(_device, &viewInfo, nullptr, &_imageViews[i]) != VK_SUCCESS)
        {
            LAppPal::PrintLog("Failed to create image views!");
        }
    }
}

void SwapchainManager::CreateDepthResources()
{
    // Create depth image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = _extent.width;
    imageInfo.extent.height = _extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = _depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(_device, &imageInfo, nullptr, &_depthImage) != VK_SUCCESS)
    {
        LAppPal::PrintLog("Failed to create depth image!");
        return;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_device, _depthImage, &memRequirements);

    // Find suitable memory type
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

    Csm::csmUint32 memoryTypeIndex = 0;
    for (Csm::csmUint32 i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            memoryTypeIndex = i;
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_depthImageMemory) != VK_SUCCESS)
    {
        LAppPal::PrintLog("Failed to allocate depth image memory!");
        return;
    }

    vkBindImageMemory(_device, _depthImage, _depthImageMemory, 0);

    // Create depth image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = _depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(_device, &viewInfo, nullptr, &_depthImageView) != VK_SUCCESS)
    {
        LAppPal::PrintLog("Failed to create depth image view!");
    }
}
