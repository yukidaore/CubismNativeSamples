/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once
#include <vulkan/vulkan.h>
#include "Type/csmMap.hpp"

// Forward declaration
#include "SwapchainManager.hpp"
struct SDL_Window;

/**
 * @brief   vulkanに必要なリソースを扱うクラス
 */
class VulkanManager
{
public:
    // キューファミリの情報を格納する構造体
    struct QueueFamilyIndices
    {
        // 描画用と表示用のキューファミリは必ずしも一致するとは限らないので分ける
        Csm::csmInt32 graphicsFamily = -1; // 描画コマンドに使用するキューファミリ
        Csm::csmInt32 presentFamily = -1; // 表示に使用するキューファミリ

        // 対応するキューファミリがあるか
        bool isComplete()
        {
            return graphicsFamily != -1 && presentFamily != -1;
        }
    } indices;

    static const char* const deviceExtensions[2];
    static const char* const validationLayers[1];

    /**
     * @brief   シングルトンインスタンスを取得する
     */
    static VulkanManager* GetInstance();

    /**
     * @brief   シングルトンインスタンスを作成する
     */
    static void Create(SDL_Window* window);

    /**
     * @brief   シングルトンインスタンスを破棄する
     */
    static void Delete();

    /**
     * @brief   検証レイヤーのサポートを確認する
     */
    bool CheckValidationLayerSupport();

    /**
     * @brief   必要な拡張機能を取得する
     * @return  必要な拡張の配列
     */
    Csm::csmVector<const char*> GetRequiredExtensions();

    /**
     * @brief   インスタンスを生成する
     */
    void CreateInstance();

    /**
     * @brief   サーフェスを作る
     */
    void CreateSurface();

    /**
     * @brief   キューファミリを見つける
     */
    void FindQueueFamilies(VkPhysicalDevice device);

    /**
     * @brief   デバイスが使えるか確認する
     */
    bool IsDeviceSuitable(VkPhysicalDevice device);

    /**
     * @brief   物理デバイスを取得する
     */
    void PickPhysicalDevice();

    /**
     * @brief   論理デバイスを作成する
     */
    void CreateLogicalDevice();

    /**
     * @brief   深度フォーマットを作成する
     */
    void ChooseSupportedDepthFormat();

    /**
     * @brief   コマンドプールを作成する
     */
    void CreateCommandPool();

    /**
     * @brief   同期オブジェクトを作成する
     */
    void CreateSyncObjects();

    /**
     * @brief   初期化する
     */
    void Initialize();

    /**
     * @brief   コマンドの記録を開始する
     */
    VkCommandBuffer BeginSingleTimeCommands();

    /**
     * @brief   コマンドを提出する
     */
    void SubmitCommand(VkCommandBuffer commandBuffer, bool isFirstDraw = false);

    /**
     * @brief   描画する
     */
    void UpdateDrawFrame();

    /**
     * @brief   描画完了の追加処理
     */
    void PostDraw();

    /**
     * @brief   スワップチェーンを再構成する
     */
    void RecreateSwapchain();

    /**
     * @brief   リソースを破棄する
     */
    void Destroy();

    /**
     * @brief   デバイスを取得する
     */
    VkDevice GetDevice() { return _device; }

    /**
     * @brief   物理デバイスを取得する
     */
    VkPhysicalDevice GetPhysicalDevice() { return _physicalDevice; }

    /**
     * @brief   グラフィックキューを取得する
     */
    VkQueue GetGraphicQueue() { return _graphicQueue; }

    /**
     * @brief   コマンドプールを取得する
     */
    VkCommandPool GetCommandPool() { return _commandPool; }

    /**
     * @brief   スワップチェーンマネージャーを取得する
     */
    SwapchainManager* GetSwapchainManager() { return _swapchainManager; }

    /**
     * @brief   インスタンスを取得する
     */
    VkInstance GetVulkanInstance() { return _instance; }

    /**
     * @brief   サーフェスを取得する
     */
    VkSurfaceKHR GetSurface() { return _surface; }

    /**
     * @brief   ウィンドウサイズが変更されたかのフラグを取得する
     */
    bool GetIsWindowSizeChanged() { return _isSwapchainInvalid; }

    /**
     * @brief   ウィンドウサイズが変更されたかのフラグをセットする
     */
    void SetIsWindowSizeChanged(bool flag) { _isSwapchainInvalid = flag; }

    /**
     * @brief   フレームバッファのフラグを更新する
     */
    void SetFrameBufferResized(bool flag) { _framebufferResized = flag; }

    /**
     * @brief   深度フォーマットを取得する
     */
    VkFormat GetDepthFormat() const { return _depthFormat; }

    /**
     * @brief   サーフェスのフォーマットを取得する
     */
    VkFormat GetImageFormat() { return _surfaceFormat; }

    /**
     * @brief   スワップチェーンイメージを取得する
     */
    VkImage GetSwapchainImage();

    /**
     * @brief   スワップチェーンイメージビューを取得する
     */
    VkImageView GetSwapchainImageView();

    /**
     * @brief   現在のイメージインデックスを取得する
     */
    Csm::csmUint32 GetImageIndex() const { return _imageIndex; }

    /**
     * @brief   バッファを作成する
     */
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    /**
     * @brief   メモリタイプを検索する
     */
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief   イメージレイアウトを遷移する
     */
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    /**
     * @brief   バッファをイメージにコピーする
     */
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

private:
    /**
     * @brief   コンストラクタ
     */
    VulkanManager(SDL_Window* window);

    /**
     * @brief   デストラクタ
     */
    ~VulkanManager();

    /**
     * @brief   デバイスの拡張をチェックする
     */
    bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);

    static VulkanManager* s_instance;

    SDL_Window* _window;
    VkInstance _instance;
    VkSurfaceKHR _surface;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkQueue _graphicQueue;
    VkQueue _presentQueue;
    VkCommandPool _commandPool;
    VkSemaphore _imageAvailableSemaphore;
    SwapchainManager* _swapchainManager;
    bool _isSwapchainInvalid = false;
    const bool _enableValidationLayers = true;
    VkDebugUtilsMessengerEXT _debugMessenger;
    Csm::csmUint32 _imageIndex = 0;
    VkFormat _depthFormat;
    const VkFormat _surfaceFormat = VK_FORMAT_R8G8B8A8_UNORM;
    bool _framebufferResized = false;
};
