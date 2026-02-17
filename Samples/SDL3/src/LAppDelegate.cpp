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
#endif

#if defined(CSM_TARGET_VULKAN)
#include <Rendering/Vulkan/CubismRenderer_Vulkan.hpp>
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
#endif
#if defined(CSM_TARGET_VULKAN)
    VulkanManager::Delete();
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
#endif
#if defined(CSM_TARGET_VULKAN)
                if (s_vulkanManager)
                {
                    s_vulkanManager->SetFrameBufferResized(true);
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
#endif
