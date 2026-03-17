/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppDelegate.hpp"
#include <iostream>
#include <SDL3/SDL.h>

#if defined(CSM_TARGET_OPENGL)
#include <GL/glew.h>
#elif defined(CSM_TARGET_VULKAN)
#include <Rendering/Vulkan/CubismRenderer_Vulkan.hpp>
#elif defined(CSM_TARGET_GPU)
#include <Rendering/SDL3_GPU/CubismRenderer_SDL3.hpp>
#endif

#include "LAppView.hpp"
#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppTextureManager.hpp"

using namespace Csm;
using namespace std;
using namespace LAppDefine;

namespace {
    LAppDelegate* s_instance = NULL;
#if defined(CSM_TARGET_VULKAN)
    VulkanManager* s_vulkanManager = NULL;
#elif defined(CSM_TARGET_GPU)
    constexpr Csm::csmUint32 s_gpuAllowedFramesInFlight = 2;
    constexpr SDL_GPUTextureFormat s_gpuDepthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

    struct SDL3GPUFrameContext
    {
        SDL_GPUCommandBuffer* CommandBuffer;
        SDL_GPUTexture* SwapchainTexture;
        Csm::csmUint32 Width;
        Csm::csmUint32 Height;
    };

    SDL3GPUFrameContext s_gpuFrameContext = {NULL, NULL, 0, 0};
#endif
}

LAppDelegate* LAppDelegate::GetInstance()
{
    if (s_instance == NULL)
    {
        s_instance = new LAppDelegate();
    }

    return s_instance;
}

void LAppDelegate::ReleaseInstance()
{
    if (s_instance != NULL)
    {
        delete s_instance;
    }

    s_instance = NULL;
}

bool LAppDelegate::Initialize()
{
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("START");
    }

    // SDLの初期化
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't initialize SDL: %s", SDL_GetError());
        }
        return false;
    }

#if defined(CSM_TARGET_OPENGL)
    // OpenGL属性の設定
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Windowの生成
    _window = SDL_CreateWindow(
        "Live2D Cubism SDK Sample (OpenGL)",
        RenderTargetWidth, RenderTargetHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (_window == NULL)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't create SDL window: %s", SDL_GetError());
        }
        SDL_Quit();
        return false;
    }

    // OpenGLコンテキストの作成
    _glContext = SDL_GL_CreateContext(_window);
    if (_glContext == NULL)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't create OpenGL context: %s", SDL_GetError());
        }
        SDL_DestroyWindow(_window);
        SDL_Quit();
        return false;
    }

    SDL_GL_MakeCurrent(_window, _glContext);
    SDL_GL_SetSwapInterval(1);

    // GLEW初期化
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't initialize GLEW");
        }
        SDL_GL_DestroyContext(_glContext);
        SDL_DestroyWindow(_window);
        SDL_Quit();
        return false;
    }

    // テクスチャサンプリング設定
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // 透過設定
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

#elif defined(CSM_TARGET_VULKAN)
    // Windowの生成（Vulkan用）
    _window = SDL_CreateWindow(
        "Live2D Cubism SDK Sample (Vulkan)",
        RenderTargetWidth, RenderTargetHeight,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );

    if (_window == NULL)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't create SDL window: %s", SDL_GetError());
        }
        SDL_Quit();
        return false;
    }

    VulkanManager::Create(_window);
    s_vulkanManager = VulkanManager::GetInstance();

    // vulkanデバイスの作成
    s_vulkanManager->Initialize();
    SwapchainManager* swapchainManager = s_vulkanManager->GetSwapchainManager();
    // レンダラにvulkanManagerの変数を渡す
    Live2D::Cubism::Framework::Rendering::CubismRenderer_Vulkan::InitializeConstantSettings(
        s_vulkanManager->GetDevice(), s_vulkanManager->GetPhysicalDevice(),
        s_vulkanManager->GetCommandPool(), s_vulkanManager->GetGraphicQueue(),
        swapchainManager->GetImageCount(), swapchainManager->GetExtent(),
        s_vulkanManager->GetSwapchainImageView(), swapchainManager->GetSwapchainImageFormat(),
        s_vulkanManager->GetDepthFormat()
    );
#elif defined(CSM_TARGET_GPU)
    // Windowの生成（SDL3_GPU用）
    _window = SDL_CreateWindow(
        "Live2D Cubism SDK Sample (SDL3 GPU)",
        RenderTargetWidth, RenderTargetHeight,
        SDL_WINDOW_RESIZABLE
    );

    if (_window == NULL)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't create SDL window: %s", SDL_GetError());
        }
        SDL_Quit();
        return false;
    }

    // SDL GPUデバイスの作成（デバッグモード有効で詳細なバリデーションエラーを出力）
    SDL_GPUShaderFormat shaderFormats = 0;
    const char* gpuDriverHint = NULL;
    int backendCount = 0;
#if defined(CSM_TARGET_GPU_VULKAN)
    shaderFormats |= SDL_GPU_SHADERFORMAT_SPIRV;
    gpuDriverHint = "vulkan";
    backendCount++;
#endif
#if defined(CSM_TARGET_GPU_D3D12)
    shaderFormats |= SDL_GPU_SHADERFORMAT_DXIL;
    gpuDriverHint = "direct3d12";
    backendCount++;
#endif
#if defined(CSM_TARGET_GPU_METAL)
    shaderFormats |= SDL_GPU_SHADERFORMAT_MSL;
    gpuDriverHint = "metal";
    backendCount++;
#endif
    // マクロ未定義または複数定義時は全フォーマットを提示してSDLに選択させる
    if (shaderFormats == 0)
    {
        shaderFormats = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL;
    }
    if (backendCount != 1)
    {
        gpuDriverHint = NULL;
    }

    _gpuDevice = SDL_CreateGPUDevice(
        shaderFormats,
        true,           // debug mode - バリデーション有効化
        gpuDriverHint   // driver hint
    );

    if (_gpuDevice == NULL)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't create GPU device: %s", SDL_GetError());
        }
        SDL_DestroyWindow(_window);
        SDL_Quit();
        return false;
    }

    // 使用中のGPUバックエンドをログ出力
    const char* driverName = SDL_GetGPUDeviceDriver(_gpuDevice);
    LAppPal::PrintLogLn("[APP]GPU driver: %s", driverName ? driverName : "(unknown)");

    // スワップチェーンの関連付け
    if (!SDL_ClaimWindowForGPUDevice(_gpuDevice, _window))
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't claim window for GPU device: %s", SDL_GetError());
        }
        SDL_DestroyGPUDevice(_gpuDevice);
        SDL_DestroyWindow(_window);
        SDL_Quit();
        return false;
    }

    // スワップチェーンフォーマットを取得
    _swapchainFormat = SDL_GetGPUSwapchainTextureFormat(_gpuDevice, _window);

    // フレーム数（in-flight）を設定
    // SDL3 GPUには実際のスワップチェーン枚数を問い合わせる公開APIがないため、
    // アプリで設定した値をレンダラー初期化にも使用する。
    _gpuAllowedFramesInFlight = s_gpuAllowedFramesInFlight;
    if (!SDL_SetGPUAllowedFramesInFlight(_gpuDevice, _gpuAllowedFramesInFlight))
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Failed to set GPU allowed frames in flight: %s", SDL_GetError());
        }
    }

    _gpuDepthFormat = s_gpuDepthFormat;

    // レンダラの初期化設定
    Live2D::Cubism::Framework::Rendering::CubismRenderer_SDL3::InitializeConstantSettings(
        _gpuDevice,
        _gpuAllowedFramesInFlight,
        RenderTargetWidth, RenderTargetHeight,
        _swapchainFormat,
        _gpuDepthFormat
    );
#endif

    // ウィンドウサイズ記憶
    int width, height;
    SDL_GetWindowSize(_window, &width, &height);
    _windowWidth = width;
    _windowHeight = height;

    // Cubism SDK の初期化
    InitializeCubism();

    // AppViewの初期化
    _view->Initialize(width, height);

    return true;
}

void LAppDelegate::Release()
{
    delete _textureManager;
    delete _view;

    // リソースを解放
    LAppLive2DManager::ReleaseInstance();

    // Cubism SDK の解放
    CubismFramework::Dispose();

#if defined(CSM_TARGET_OPENGL)
    SDL_GL_DestroyContext(_glContext);
#elif defined(CSM_TARGET_VULKAN)
    VulkanManager::Delete();
#elif defined(CSM_TARGET_GPU)
    SDL_ReleaseWindowFromGPUDevice(_gpuDevice, _window);
    SDL_DestroyGPUDevice(_gpuDevice);
#endif
    // Windowの削除
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

#if defined(CSM_TARGET_VULKAN)
bool LAppDelegate::RecreateSwapchain()
{
    int width = 0, height = 0;
    if (s_vulkanManager->GetIsWindowSizeChanged())
    {
        SDL_GetWindowSize(_window, &width, &height);
        while (width == 0 || height == 0)
        {
            SDL_GetWindowSize(_window, &width, &height);
            SDL_WaitEvent(NULL);
        }

        s_vulkanManager->RecreateSwapchain();
        Live2D::Cubism::Framework::Rendering::CubismRenderer_Vulkan::SetRenderTarget(
            s_vulkanManager->GetSwapchainImage(),
            s_vulkanManager->GetSwapchainImageView(),
            s_vulkanManager->GetSwapchainManager()->GetSwapchainImageFormat(),
            s_vulkanManager->GetSwapchainManager()->GetExtent()
        );

        // AppViewの初期化
        _view->Initialize(width, height);
        // スプライトサイズを再設定
        _view->ResizeSprite(width, height);
        // オフスクリーンを再作成する
        _view->DestroyRenderTarget();
        // モデルのオフスクリーンのサイズを再設定
        LAppLive2DManager::GetInstance()->SetRenderTargetSize(width, height);
        // サイズを保存しておく
        _windowWidth = width;
        _windowHeight = height;
        s_vulkanManager->SetIsWindowSizeChanged(false);
        return true;
    }
    return false;
}
#elif defined(CSM_TARGET_GPU)
void LAppDelegate::ResizeWindow(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    // レンダーターゲットのサイズを更新
    Live2D::Cubism::Framework::Rendering::CubismRenderer_SDL3::SetRenderTarget(
        nullptr,  // will be acquired from swapchain
        _swapchainFormat,
        width, height
    );

    // AppViewの初期化
    _view->Initialize(width, height);
    // スプライトサイズを再設定
    _view->ResizeSprite(width, height);
    // オフスクリーンを再作成する
    _view->DestroyRenderTarget();
    // モデルのオフスクリーンのサイズを再設定
    LAppLive2DManager::GetInstance()->SetRenderTargetSize(width, height);
    // サイズを保存しておく
    _windowWidth = width;
    _windowHeight = height;
}
#endif

void LAppDelegate::Run()
{
    // メインループ
    SDL_Event event;

    while (!_isEnd)
    {
        // イベント処理
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                _isEnd = true;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                OnMouseEvent(event.button.button, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN,
                           static_cast<float>(event.button.x), static_cast<float>(event.button.y));
                break;
            case SDL_EVENT_MOUSE_MOTION:
                OnMouseMoved(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
                break;
            case SDL_EVENT_WINDOW_RESIZED:
#if defined(CSM_TARGET_OPENGL)
                {
                    int width = event.window.data1;
                    int height = event.window.data2;
                    if (width > 0 && height > 0)
                    {
                        // AppViewの初期化
                        _view->Initialize(width, height);
                        // スプライトサイズを再設定
                        _view->ResizeSprite();
                        // オフスクリーンのサイズ変更
                        LAppLive2DManager::GetInstance()->SetRenderTargetSize(width, height);
                        // サイズを保存しておく
                        _windowWidth = width;
                        _windowHeight = height;
                        // ビューポート変更
                        glViewport(0, 0, width, height);
                    }
                }
#elif defined(CSM_TARGET_VULKAN)
                if (s_vulkanManager)
                {
                    s_vulkanManager->SetFrameBufferResized(true);
                }
#elif defined(CSM_TARGET_GPU)
                {
                    int width = event.window.data1;
                    int height = event.window.data2;
                    ResizeWindow(width, height);
                }
#endif
                break;
            }
        }

        // 時間更新
        LAppPal::UpdateTime();

#if defined(CSM_TARGET_OPENGL)
        // 画面の初期化
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearDepth(1.0);

        // 描画更新
        _view->Render();

        // バッファの入れ替え
        SDL_GL_SwapWindow(_window);
#elif defined(CSM_TARGET_VULKAN)
        s_vulkanManager->UpdateDrawFrame();

        if (RecreateSwapchain())
        {
            continue;
        }

        _view->Render();
        s_vulkanManager->PostDraw();
        RecreateSwapchain();
#elif defined(CSM_TARGET_GPU)
        // スワップチェーンが利用可能になるまでブロック待機（スピンループ防止）
        if (!SDL_WaitForGPUSwapchain(_gpuDevice, _window))
        {
            if (DebugLogEnable)
            {
                LAppPal::PrintLogLn("Failed to wait for GPU swapchain: %s", SDL_GetError());
            }
            continue;
        }

        // コマンドバッファを取得
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(_gpuDevice);
        if (commandBuffer == NULL)
        {
            if (DebugLogEnable)
            {
                LAppPal::PrintLogLn("Failed to acquire command buffer: %s", SDL_GetError());
            }
            continue;
        }

        // スワップチェーンテクスチャを取得
        SDL_GPUTexture* swapchainTexture = NULL;
        Csm::csmUint32 swapchainWidth = 0, swapchainHeight = 0;
        if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, _window, &swapchainTexture, &swapchainWidth, &swapchainHeight))
        {
            if (DebugLogEnable)
            {
                LAppPal::PrintLogLn("Failed to acquire swapchain texture: %s", SDL_GetError());
            }
            s_gpuFrameContext.CommandBuffer = NULL;
            s_gpuFrameContext.SwapchainTexture = NULL;
            s_gpuFrameContext.Width = 0;
            s_gpuFrameContext.Height = 0;
            SDL_CancelGPUCommandBuffer(commandBuffer);
            continue;
        }

        if (swapchainTexture != NULL)
        {
            s_gpuFrameContext.CommandBuffer = commandBuffer;
            s_gpuFrameContext.SwapchainTexture = swapchainTexture;
            s_gpuFrameContext.Width = swapchainWidth;
            s_gpuFrameContext.Height = swapchainHeight;

            // 描画更新
            _view->Render();

            // コマンドバッファを送信
            SDL_SubmitGPUCommandBuffer(commandBuffer);

            s_gpuFrameContext.CommandBuffer = NULL;
            s_gpuFrameContext.SwapchainTexture = NULL;
            s_gpuFrameContext.Width = 0;
            s_gpuFrameContext.Height = 0;
        }
        else
        {
            s_gpuFrameContext.CommandBuffer = NULL;
            s_gpuFrameContext.SwapchainTexture = NULL;
            s_gpuFrameContext.Width = 0;
            s_gpuFrameContext.Height = 0;
            SDL_CancelGPUCommandBuffer(commandBuffer);
        }
#endif
    }

    Release();
    ReleaseInstance();
}

LAppDelegate::LAppDelegate()
    : _cubismOption()
    , _window(NULL)
    , _captured(false)
    , _mouseX(0.0f)
    , _mouseY(0.0f)
    , _isEnd(false)
    , _windowWidth(0)
    , _windowHeight(0)
#if defined(CSM_TARGET_OPENGL)
    , _glContext(NULL)
#elif defined(CSM_TARGET_GPU)
    , _gpuDevice(NULL)
    , _swapchainFormat(SDL_GPU_TEXTUREFORMAT_INVALID)
    , _gpuDepthFormat(SDL_GPU_TEXTUREFORMAT_D32_FLOAT)
    , _gpuAllowedFramesInFlight(s_gpuAllowedFramesInFlight)
#endif
{
    _view = new LAppView();
    _textureManager = new LAppTextureManager();
}

LAppDelegate::~LAppDelegate()
{
}

void LAppDelegate::InitializeCubism()
{
    // setup cubism
    _cubismOption.LogFunction = LAppPal::PrintMessage;
    _cubismOption.LoggingLevel = LAppDefine::CubismLoggingLevel;
    _cubismOption.LoadFileFunction = LAppPal::LoadFileAsBytes;
    _cubismOption.ReleaseBytesFunction = LAppPal::ReleaseBytes;
    Csm::CubismFramework::StartUp(&_cubismAllocator, &_cubismOption);

    // Initialize cubism
    CubismFramework::Initialize();

    // load model
    LAppLive2DManager::GetInstance();

    // default proj
    CubismMatrix44 projection;

    LAppPal::UpdateTime();
}

void LAppDelegate::OnMouseEvent(Uint8 button, bool pressed, float x, float y)
{
    if (_view == NULL)
    {
        return;
    }
    if (SDL_BUTTON_LEFT != button)
    {
        return;
    }

    _mouseX = x;
    _mouseY = y;

    if (pressed)
    {
        _captured = true;
        _view->OnTouchesBegan(_mouseX, _mouseY);
    }
    else
    {
        if (_captured)
        {
            _captured = false;
            _view->OnTouchesEnded(_mouseX, _mouseY);
        }
    }
}

void LAppDelegate::OnMouseMoved(float x, float y)
{
    _mouseX = x;
    _mouseY = y;

    if (!_captured)
    {
        return;
    }
    if (_view == NULL)
    {
        return;
    }

    _view->OnTouchesMoved(_mouseX, _mouseY);
}

#if defined(CSM_TARGET_VULKAN)
VulkanManager* LAppDelegate::GetVulkanManager()
{
    return s_vulkanManager;
}
#elif defined(CSM_TARGET_GPU)
SDL_GPUCommandBuffer* LAppDelegate::GetCurrentGPUCommandBuffer() const
{
    return s_gpuFrameContext.CommandBuffer;
}

SDL_GPUTexture* LAppDelegate::GetCurrentGPUSwapchainTexture() const
{
    return s_gpuFrameContext.SwapchainTexture;
}

Csm::csmUint32 LAppDelegate::GetCurrentGPUSwapchainWidth() const
{
    return s_gpuFrameContext.Width;
}

Csm::csmUint32 LAppDelegate::GetCurrentGPUSwapchainHeight() const
{
    return s_gpuFrameContext.Height;
}
#endif
