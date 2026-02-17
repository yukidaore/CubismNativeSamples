/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once
#include <vulkan/vulkan.h>
#include "Type/csmVector.hpp"
#include "Type/csmRectF.hpp"

/**
 * @brief   スワップチェーンの管理を行うクラス
 */
class SwapchainManager
{
public:
    /**
     * @brief   コンストラクタ
     */
    SwapchainManager(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                     VkFormat surfaceFormat, VkFormat depthFormat,
                     Csm::csmUint32 width, Csm::csmUint32 height);

    /**
     * @brief   デストラクタ
     */
    ~SwapchainManager();

    /**
     * @brief   スワップチェーンを取得する
     */
    VkSwapchainKHR GetSwapchain() const { return _swapchain; }

    /**
     * @brief   イメージを取得する
     */
    Csm::csmVector<VkImage>& GetImages() { return _images; }

    /**
     * @brief   イメージビューを取得する
     */
    Csm::csmVector<VkImageView>& GetImageViews() { return _imageViews; }

    /**
     * @brief   深度イメージを取得する
     */
    VkImage GetDepthImage() const { return _depthImage; }

    /**
     * @brief   深度イメージビューを取得する
     */
    VkImageView GetDepthImageView() const { return _depthImageView; }

    /**
     * @brief   エクステントを取得する
     */
    VkExtent2D GetExtent() const { return _extent; }

    /**
     * @brief   イメージ数を取得する
     */
    Csm::csmUint32 GetImageCount() const { return static_cast<Csm::csmUint32>(_images.GetSize()); }

    /**
     * @brief   スワップチェーンの画像フォーマットを取得する
     */
    VkFormat GetSwapchainImageFormat() const { return _surfaceFormat; }

private:
    /**
     * @brief   イメージビューを作成する
     */
    void CreateImageViews();

    /**
     * @brief   深度リソースを作成する
     */
    void CreateDepthResources();

    VkDevice _device;
    VkPhysicalDevice _physicalDevice;
    VkFormat _surfaceFormat;
    VkFormat _depthFormat;
    VkSwapchainKHR _swapchain;
    Csm::csmVector<VkImage> _images;
    Csm::csmVector<VkImageView> _imageViews;
    VkExtent2D _extent;
    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;
};
