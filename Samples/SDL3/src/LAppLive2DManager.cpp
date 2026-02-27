/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppLive2DManager.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <Rendering/CubismRenderer.hpp>
#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "LAppDelegate.hpp"
#include "LAppModel.hpp"
#include "LAppView.hpp"

#if defined(CSM_TARGET_OPENGL)
#include <Rendering/OpenGL/CubismOffscreenManager_OpenGLES2.hpp>
#elif defined(CSM_TARGET_VULKAN)
#include <Rendering/Vulkan/CubismOffscreenManager_Vulkan.hpp>
#elif defined(CSM_TARGET_GPU)
#include <Rendering/SDL3_GPU/CubismOffscreenManager_SDL3.hpp>
#include <Rendering/SDL3_GPU/CubismRenderer_SDL3.hpp>
#endif

#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <dirent.h>
#include <sys/stat.h>
#endif

using namespace Csm;
using namespace LAppDefine;

namespace {
    LAppLive2DManager* s_instance = NULL;

    void BeganMotion(ACubismMotion* self)
    {
        LAppPal::PrintLogLn("Motion Began: %x", self);
    }

    void FinishedMotion(ACubismMotion* self)
    {
        LAppPal::PrintLogLn("Motion Finished: %x", self);
    }

    int CompareCsmString(const void* a, const void* b)
    {
        return strcmp(reinterpret_cast<const Csm::csmString*>(a)->GetRawString(),
            reinterpret_cast<const Csm::csmString*>(b)->GetRawString());
    }
}

LAppLive2DManager* LAppLive2DManager::GetInstance()
{
    if (s_instance == NULL)
    {
        s_instance = new LAppLive2DManager();
    }

    return s_instance;
}

void LAppLive2DManager::ReleaseInstance()
{
    if (s_instance != NULL)
    {
        delete s_instance;
    }

    s_instance = NULL;
}

LAppLive2DManager::LAppLive2DManager()
    : _viewMatrix(NULL)
    , _sceneIndex(0)
{
    _viewMatrix = new CubismMatrix44();
    SetUpModel();

    ChangeScene(_sceneIndex);
}

LAppLive2DManager::~LAppLive2DManager()
{
    ReleaseAllModel();
    delete _viewMatrix;
#if defined(CSM_TARGET_OPENGL)
    Csm::Rendering::CubismOffscreenManager_OpenGLES2::ReleaseInstance();
#elif defined(CSM_TARGET_VULKAN)
    Csm::Rendering::CubismOffscreenManager_Vulkan::ReleaseInstance();
#elif defined(CSM_TARGET_GPU)
    Csm::Rendering::CubismOffscreenManager_SDL3::ReleaseInstance();
#endif
}

void LAppLive2DManager::ReleaseAllModel()
{
    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        delete _models[i];
    }

    _models.Clear();
}

void LAppLive2DManager::SetUpModel()
{
    // ResourcesPathの中にあるフォルダ名を全てクロールし、モデルが存在するフォルダを定義する。
    // フォルダはあるが同名の.model3.jsonが見つからなかった場合はリストに含めない。

    csmString crawlPath(ResourcesPath);

    _modelDir.Clear();

#if defined(_WIN32)
    crawlPath += "*.*";

    wchar_t wideStr[MAX_PATH];
    csmChar name[MAX_PATH];
    LAppPal::ConvertMultiByteToWide(crawlPath.GetRawString(), wideStr, MAX_PATH);

    struct _wfinddata_t fdata;
    intptr_t fh = _wfindfirst(wideStr, &fdata);
    if (fh == -1)
    {
        return;
    }

    while (_wfindnext(fh, &fdata) == 0)
    {
        if ((fdata.attrib & _A_SUBDIR) && wcscmp(fdata.name, L"..") != 0)
        {
            LAppPal::ConvertWideToMultiByte(fdata.name, name, MAX_PATH);

            // フォルダと同名の.model3.jsonがあるか探索する
            csmString model3jsonPath(ResourcesPath);
            model3jsonPath += name;
            model3jsonPath.Append(1, '/');
            model3jsonPath += name;
            model3jsonPath += ".model3.json";

            LAppPal::ConvertMultiByteToWide(model3jsonPath.GetRawString(), wideStr, MAX_PATH);

            struct _wfinddata_t fdata2;
            if (_wfindfirst(wideStr, &fdata2) != -1)
            {
                _modelDir.PushBack(csmString(name));
            }
        }
    }
    _findclose(fh);
#elif defined(__linux__) || defined(__APPLE__)
    DIR* dir = opendir(crawlPath.GetRawString());
    if (dir == NULL)
    {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            // フォルダと同名の.model3.jsonがあるか探索する
            csmString model3jsonPath(ResourcesPath);
            model3jsonPath += entry->d_name;
            model3jsonPath.Append(1, '/');
            model3jsonPath += entry->d_name;
            model3jsonPath += ".model3.json";

            struct stat st;
            if (stat(model3jsonPath.GetRawString(), &st) == 0)
            {
                _modelDir.PushBack(csmString(entry->d_name));
            }
        }
    }
    closedir(dir);
#endif

    qsort(_modelDir.GetPtr(), _modelDir.GetSize(), sizeof(csmString), CompareCsmString);
}

csmVector<csmString> LAppLive2DManager::GetModelDir() const
{
    return _modelDir;
}

csmInt32 LAppLive2DManager::GetModelDirSize() const
{
    return _modelDir.GetSize();
}

LAppModel* LAppLive2DManager::GetModel(csmUint32 no) const
{
    if (no < _models.GetSize())
    {
        return _models[no];
    }

    return NULL;
}

void LAppLive2DManager::SetRenderTargetSize(csmUint32 width, csmUint32 height)
{
    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        LAppModel* model = GetModel(i);
        model->SetRenderTargetSize(width, height);
    }
}

void LAppLive2DManager::OnDrag(csmFloat32 x, csmFloat32 y) const
{
    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        LAppModel* model = GetModel(i);
        model->SetDragging(x, y);
    }
}

void LAppLive2DManager::OnTap(csmFloat32 x, csmFloat32 y)
{
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("[APP]tap point: {x:%.2f y:%.2f}", x, y);
    }

    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        if (_models[i]->HitTest(HitAreaNameHead, x, y))
        {
            if (DebugLogEnable)
            {
                LAppPal::PrintLogLn("[APP]hit area: [%s]", HitAreaNameHead);
            }
            _models[i]->SetRandomExpression();
        }
        else if (_models[i]->HitTest(HitAreaNameBody, x, y))
        {
            if (DebugLogEnable)
            {
                LAppPal::PrintLogLn("[APP]hit area: [%s]", HitAreaNameBody);
            }
            _models[i]->StartRandomMotion(MotionGroupTapBody, PriorityNormal, FinishedMotion, BeganMotion);
        }
    }
}

void LAppLive2DManager::OnUpdate() const
{
    csmUint32 width = 0;
    csmUint32 height = 0;

#if defined(CSM_TARGET_GPU)
    LAppDelegate* delegate = LAppDelegate::GetInstance();
    SDL_GPUCommandBuffer* commandBuffer = delegate->GetCurrentGPUCommandBuffer();
    SDL_GPUTexture* swapchainTexture = delegate->GetCurrentGPUSwapchainTexture();
    width = delegate->GetCurrentGPUSwapchainWidth();
    height = delegate->GetCurrentGPUSwapchainHeight();

    if (commandBuffer == NULL || swapchainTexture == NULL || width == 0 || height == 0)
    {
        return;
    }
#else
    width = static_cast<csmUint32>(LAppDelegate::GetInstance()->GetWindowWidth());
    height = static_cast<csmUint32>(LAppDelegate::GetInstance()->GetWindowHeight());
#endif

#if defined(CSM_TARGET_OPENGL)
    // モデルで使用するオフスクリーン管理の開始処理
    Csm::Rendering::CubismOffscreenManager_OpenGLES2::GetInstance()->BeginFrameProcess();
#elif defined(CSM_TARGET_VULKAN)
    // モデルで使用するオフスクリーン管理の開始処理
    Csm::Rendering::CubismOffscreenManager_Vulkan::GetInstance()->BeginFrameProcess();
#elif defined(CSM_TARGET_GPU)
    // モデルで使用するオフスクリーン管理の開始処理
    Csm::Rendering::CubismOffscreenManager_SDL3::GetInstance()->BeginFrameProcess();
#endif

    csmUint32 modelCount = _models.GetSize();
    for (csmUint32 i = 0; i < modelCount; ++i)
    {
        CubismMatrix44 projection;
        LAppModel* model = GetModel(i);

        if (model->GetModel() == NULL)
        {
            LAppPal::PrintLogLn("Failed to model->GetModel().");
            continue;
        }

        if (model->GetModel()->GetCanvasWidth() > 1.0f && width < height)
        {
            // 横に長いモデルを縦長ウィンドウに表示する際モデルの横サイズでscaleを算出する
            model->GetModelMatrix()->SetWidth(2.0f);
            projection.Scale(1.0f, static_cast<float>(width) / static_cast<float>(height));
        }
        else
        {
            projection.Scale(static_cast<float>(height) / static_cast<float>(width), 1.0f);
        }

        // 必要があればここで乗算
        if (_viewMatrix != NULL)
        {
            projection.MultiplyByMatrix(_viewMatrix);
        }

        // モデル1体描画前コール
        LAppDelegate::GetInstance()->GetView()->PreModelDraw(*model);

        model->Update();
        model->Draw(projection); ///< 参照渡しなのでprojectionは変質する

#if defined(CSM_TARGET_OPENGL)
        // モデル1体描画後コール
        LAppDelegate::GetInstance()->GetView()->PostModelDraw(*model);
#elif defined(CSM_TARGET_VULKAN) || defined(CSM_TARGET_GPU)
        // モデル1体描画後コール
        LAppDelegate::GetInstance()->GetView()->PostModelDraw(*model, i);
#endif
    }

#if defined(CSM_TARGET_OPENGL)
    // モデルで使用するオフスクリーン管理の終了処理
    Csm::Rendering::CubismOffscreenManager_OpenGLES2::GetInstance()->EndFrameProcess();
    // もし余っているオフスクリーンのリソースを解放したい場合行う処理
    Csm::Rendering::CubismOffscreenManager_OpenGLES2::GetInstance()->ReleaseStaleRenderTextures();
#elif defined(CSM_TARGET_VULKAN)
    // モデルで使用するオフスクリーン管理の終了処理
    Csm::Rendering::CubismOffscreenManager_Vulkan::GetInstance()->EndFrameProcess();
    // もし余っているオフスクリーンのリソースを解放したい場合行う処理
    Csm::Rendering::CubismOffscreenManager_Vulkan::GetInstance()->ReleaseStaleRenderTextures();
#elif defined(CSM_TARGET_GPU)
    // モデルで使用するオフスクリーン管理の終了処理
    Csm::Rendering::CubismOffscreenManager_SDL3::GetInstance()->EndFrameProcess();
    // もし余っているオフスクリーンのリソースを解放したい場合行う処理
    Csm::Rendering::CubismOffscreenManager_SDL3::GetInstance()->ReleaseStaleRenderTextures();
#endif
}

void LAppLive2DManager::NextScene()
{
    csmInt32 no = (_sceneIndex + 1) % GetModelDirSize();
    ChangeScene(no);
}

void LAppLive2DManager::ChangeScene(Csm::csmInt32 index)
{
    _sceneIndex = index;
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("[APP]model index: %d", _sceneIndex);
    }

    // model3.jsonのパスを決定する.
    // ディレクトリ名とmodel3.jsonの名前を一致していることが条件
    const csmString& model = _modelDir[index];

    csmString modelPath(ResourcesPath);
    modelPath += model;
    modelPath.Append(1, '/');

    csmString modelJsonName(model);
    modelJsonName += ".model3.json";

#if defined(CSM_TARGET_VULKAN)
    VulkanManager* vulkanManager = LAppDelegate::GetInstance()->GetVulkanManager();
    vkDeviceWaitIdle(vulkanManager->GetDevice());
#elif defined(CSM_TARGET_GPU)
    SDL_GPUDevice* device = LAppDelegate::GetInstance()->GetGPUDevice();
    SDL_WaitForGPUIdle(device);
#endif

    ReleaseAllModel();
    _models.PushBack(new LAppModel());
#if defined(CSM_TARGET_VULKAN) && (defined(USE_RENDER_TARGET) || defined(USE_MODEL_RENDER_TARGET))
    Csm::Rendering::CubismRenderer_Vulkan::EnableChangeRenderTarget();
#endif

#if defined(CSM_TARGET_OPENGL)
    _models[0]->LoadAssets(modelPath.GetRawString(), modelJsonName.GetRawString());
#elif defined(CSM_TARGET_VULKAN)
    _models[0]->LoadAssets(vulkanManager->GetDevice(), vulkanManager->GetImageFormat(), modelPath.GetRawString(), modelJsonName.GetRawString());
#elif defined(CSM_TARGET_GPU)
    _models[0]->LoadAssets(device, modelPath.GetRawString(), modelJsonName.GetRawString());
#endif

    /*
     * モデル半透明表示を行うサンプルを提示する。
     * ここでUSE_RENDER_TARGET、USE_MODEL_RENDER_TARGETが定義されている場合
     * 別のレンダリングターゲットにモデルを描画し、描画結果をテクスチャとして別のスプライトに張り付ける。
     */
    {
#if defined(USE_RENDER_TARGET)
        // LAppViewの持つターゲットに描画を行う場合、こちらを選択
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_ViewFrameBuffer;
#elif defined(USE_MODEL_RENDER_TARGET)
        // 各LAppModelの持つターゲットに描画を行う場合、こちらを選択
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_ModelFrameBuffer;
#else
        // デフォルトのメインフレームバッファへレンダリングする(通常)
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_None;
#endif

#if defined(USE_RENDER_TARGET) || defined(USE_MODEL_RENDER_TARGET)
        // モデル個別にαを付けるサンプルとして、もう1体モデルを作成し、少し位置をずらす
        _models.PushBack(new LAppModel());
#if defined(CSM_TARGET_OPENGL)
        _models[1]->LoadAssets(modelPath.GetRawString(), modelJsonName.GetRawString());
#elif defined(CSM_TARGET_VULKAN)
        _models[1]->LoadAssets(vulkanManager->GetDevice(), vulkanManager->GetImageFormat(), modelPath.GetRawString(), modelJsonName.GetRawString());
#elif defined(CSM_TARGET_GPU)
        _models[1]->LoadAssets(device, modelPath.GetRawString(), modelJsonName.GetRawString());
#endif
        _models[1]->GetModelMatrix()->TranslateX(0.2f);
#endif

        LAppDelegate::GetInstance()->GetView()->SwitchRenderingTarget(useRenderTarget);

        // 別レンダリング先を選択した際の背景クリア色
        float clearColor[3] = { 0.0f, 0.0f, 0.0f };
        LAppDelegate::GetInstance()->GetView()->SetRenderTargetClearColor(clearColor[0], clearColor[1], clearColor[2]);
    }
}

csmUint32 LAppLive2DManager::GetModelNum() const
{
    return _models.GetSize();
}

void LAppLive2DManager::SetViewMatrix(CubismMatrix44* m)
{
    for (int i = 0; i < 16; i++)
    {
        _viewMatrix->GetArray()[i] = m->GetArray()[i];
    }
}
