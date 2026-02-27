/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "LAppAllocator_Common.hpp"
#include <SDL3/SDL.h>

#if defined(CSM_TARGET_VULKAN)
#include "VulkanManager.hpp"
#elif defined(CSM_TARGET_GPU)
#include <Rendering/SDL3_GPU/CubismRenderer_SDL3.hpp>
#endif

class LAppView;
class LAppTextureManager;

/**
* @brief アプリケーションクラス。
*   Cubism SDK の管理を行う。
*/
class LAppDelegate
{
public:
    /**
    * @brief クラスのインスタンス（シングルトン）を返す。
    *        インスタンスが生成されていない場合は内部でインスタンを生成する。
    *
    * @return クラスのインスタンス
    */
    static LAppDelegate* GetInstance();

    /**
    * @brief クラスのインスタンス（シングルトン）を解放する。
    */
    static void ReleaseInstance();

    /**
    * @brief APPに必要なものを初期化する。
    */
    bool Initialize();

    /**
    * @brief 解放する。
    */
    void Release();

    /**
    * @brief 実行処理。
    */
    void Run();

    /**
    * @brief   マウスボタンイベント処理。
    *
    * @param[in]       button            ボタン種類
    * @param[in]       state             ボタン状態
    * @param[in]       x                 x座標
    * @param[in]       y                 y座標
    */
    void OnMouseEvent(Uint8 button, bool pressed, float x, float y);

    /**
    * @brief   マウス移動イベント処理。
    *
    * @param[in]       x                 x座標
    * @param[in]       y                 y座標
    */
    void OnMouseMoved(float x, float y);


#if defined(CSM_TARGET_VULKAN)
    /**
    * @brief スワップチェーンの再作成
    */
    bool RecreateSwapchain();
#elif defined(CSM_TARGET_GPU)
    /**
    * @brief ウィンドウサイズ変更処理
    */
    void ResizeWindow(int width, int height);
#endif

    /**
    * @brief Window情報を取得する。
    */
    SDL_Window* GetWindow() { return _window; }

    /**
    * @brief View情報を取得する。
    */
    LAppView* GetView() { return _view; }

    /**
    * @brief アプリケーションを終了するかどうか。
    */
    bool GetIsEnd() { return _isEnd; }

    /**
    * @brief アプリケーションを終了する。
    */
    void AppEnd() { _isEnd = true; }

    /**
    * @brief テクスチャマネージャーを取得する。
    */
    LAppTextureManager* GetTextureManager() { return _textureManager; }

#if defined(CSM_TARGET_VULKAN)
    /**
    * @brief VulkanManagerを取得する。
    */
    VulkanManager* GetVulkanManager();
#elif defined(CSM_TARGET_GPU)
    /**
    * @brief SDL GPUデバイスを取得する。
    */
    SDL_GPUDevice* GetGPUDevice() { return _gpuDevice; }

    /**
    * @brief スワップチェーンのテクスチャフォーマットを取得する。
    */
    SDL_GPUTextureFormat GetSwapchainFormat() { return _swapchainFormat; }

    /**
    * @brief 深度テクスチャフォーマットを取得する。
    */
    SDL_GPUTextureFormat GetDepthFormat() const { return _gpuDepthFormat; }

    /**
    * @brief アプリで設定したGPUフレーム数（in-flight）を取得する。
    */
    Csm::csmUint32 GetGPUAllowedFramesInFlight() const { return _gpuAllowedFramesInFlight; }

    /**
    * @brief 現在フレームのGPUコマンドバッファを取得する。
    */
    SDL_GPUCommandBuffer* GetCurrentGPUCommandBuffer() const;

    /**
    * @brief 現在フレームのスワップチェーンテクスチャを取得する。
    */
    SDL_GPUTexture* GetCurrentGPUSwapchainTexture() const;

    /**
    * @brief 現在フレームのスワップチェーン幅を取得する。
    */
    Csm::csmUint32 GetCurrentGPUSwapchainWidth() const;

    /**
    * @brief 現在フレームのスワップチェーン高さを取得する。
    */
    Csm::csmUint32 GetCurrentGPUSwapchainHeight() const;
#endif

#if defined(CSM_TARGET_OPENGL)
    /**
    * @brief OpenGLコンテキストを取得する。
    */
    SDL_GLContext GetGLContext() { return _glContext; }
#endif

    /**
    * @brief ウインドウの幅を取得する。
    */
    int GetWindowWidth() { return _windowWidth; }

    /**
    * @brief ウインドウの高さを取得する。
    */
    int GetWindowHeight() { return _windowHeight; }

private:
    /**
    * @brief コンストラクタ
    */
    LAppDelegate();

    /**
    * @brief デストラクタ
    */
    ~LAppDelegate();

    /**
    * @brief Cubism SDK の初期化
    */
    void InitializeCubism();

    LAppAllocator_Common _cubismAllocator;       ///< Cubism SDK Allocator
    Csm::CubismFramework::Option _cubismOption;  ///< Cubism SDK Option
    SDL_Window* _window;                         ///< SDL ウィンドウ
    LAppView* _view;                             ///< View情報
    bool _captured;                              ///< クリックしているか
    float _mouseX;                               ///< マウスX座標
    float _mouseY;                               ///< マウスY座標
    bool _isEnd;                                 ///< APP終了しているか
    LAppTextureManager* _textureManager;         ///< テクスチャマネージャー

    int _windowWidth;                            ///< Initialize関数で設定したウィンドウ幅
    int _windowHeight;                           ///< Initialize関数で設定したウィンドウ高さ

#if defined(CSM_TARGET_OPENGL)
    SDL_GLContext _glContext;                    ///< OpenGL コンテキスト
#elif defined(CSM_TARGET_GPU)
    SDL_GPUDevice* _gpuDevice;                   ///< SDL GPU デバイス
    SDL_GPUTextureFormat _swapchainFormat;       ///< スワップチェーンのテクスチャフォーマット
    SDL_GPUTextureFormat _gpuDepthFormat;        ///< 深度テクスチャフォーマット
    Csm::csmUint32 _gpuAllowedFramesInFlight;    ///< 設定したGPUフレーム数（in-flight）
#endif
};
