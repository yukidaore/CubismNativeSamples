/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppView.hpp"
#include <math.h>
#include <string>
#include "LAppPal.hpp"
#include "LAppDelegate.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppTextureManager.hpp"
#include "LAppDefine.hpp"
#include "TouchManager_Common.hpp"
#include "LAppSprite.hpp"
#include "LAppModel.hpp"

#if defined(CSM_TARGET_OPENGL)
#include <GL/glew.h>
#include "LAppSpriteShader.hpp"
#elif defined(CSM_TARGET_VULKAN)
#include "LAppSpritePipeline.hpp"
#include "LAppModelSpritePipeline.hpp"
#elif defined(CSM_TARGET_GPU)
#include "LAppPal.hpp"
#include <Rendering/SDL3_GPU/CubismClass_SDL3.hpp>
#endif

using namespace std;
using namespace LAppDefine;

#if defined(CSM_TARGET_GPU)
namespace {
    /**
     * @brief デバイスのサポートするシェーダーフォーマット情報
     */
    struct ShaderFormatInfo
    {
        SDL_GPUShaderFormat format;
        const char* subDir;
        const char* extension;
        const char* entrypoint;
    };

    /**
     * @brief デバイスがサポートするシェーダーフォーマットを検出する
     */
    ShaderFormatInfo DetectShaderFormat(SDL_GPUDevice* device)
    {
        SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
        if (formats & SDL_GPU_SHADERFORMAT_SPIRV)
        {
            return { SDL_GPU_SHADERFORMAT_SPIRV, "spv/", ".spv", "main" };
        }
        else if (formats & SDL_GPU_SHADERFORMAT_DXIL)
        {
            return { SDL_GPU_SHADERFORMAT_DXIL, "dxil/", ".dxil", "main" };
        }
        else if (formats & SDL_GPU_SHADERFORMAT_MSL)
        {
            return { SDL_GPU_SHADERFORMAT_MSL, "msl/", ".msl", "main0" };
        }
        // フォールバック
        return { SDL_GPU_SHADERFORMAT_SPIRV, "spv/", ".spv", "main" };
    }

    /**
     * @brief シェーダーをロードしてGPUシェーダーを作成する
     */
    SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const char* filename, Uint32 samplerCount, Uint32 uniformBufferCount, SDL_GPUShaderStage stage)
    {
        ShaderFormatInfo fmtInfo = DetectShaderFormat(device);

        // baseName からフルパスを構築: ShaderPath + subDir + baseName + extension
        std::string fullPath = std::string(ShaderPath) + fmtInfo.subDir + filename + fmtInfo.extension;

        Csm::csmSizeInt shaderSize;
        Csm::csmByte* shaderCode = LAppPal::LoadFileAsBytes(fullPath.c_str(), &shaderSize);
        if (!shaderCode)
        {
            LAppPal::PrintLogLn("[APP]failed to load shader file: %s", fullPath.c_str());
            return nullptr;
        }

        SDL_GPUShaderCreateInfo shaderInfo = {};
        shaderInfo.code = shaderCode;
        shaderInfo.code_size = static_cast<size_t>(shaderSize);
        shaderInfo.entrypoint = fmtInfo.entrypoint;
        shaderInfo.format = fmtInfo.format;
        shaderInfo.stage = stage;
        shaderInfo.num_samplers = samplerCount;
        shaderInfo.num_uniform_buffers = uniformBufferCount;

        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
        LAppPal::ReleaseBytes(shaderCode);

        if (!shader)
        {
            LAppPal::PrintLogLn("[APP]failed to create GPU shader from: %s", fullPath.c_str());
        }
        else
        {
            LAppPal::PrintLogLn("[APP]create GPU shader from: %s", fullPath.c_str());
        }
        return shader;
    }

    /**
     * @brief スプライト用パイプラインを作成する共通ヘルパー
     *
     * @param[in] device            GPUデバイス
     * @param[in] swapchainFormat   スワップチェーンフォーマット
     * @param[in] srcBlendFactor    ソースブレンドファクタ（カラー・アルファ共通）
     * @return パイプライン。失敗時はnullptr
     */
    SDL_GPUGraphicsPipeline* CreateSpritePipelineCore(SDL_GPUDevice* device,
                                                      SDL_GPUTextureFormat swapchainFormat,
                                                      SDL_GPUBlendFactor srcBlendFactor)
    {
        // シェーダーをロード
        SDL_GPUShader* vertShader = LoadShader(device,
            "VertSprite",
            0, 0, SDL_GPU_SHADERSTAGE_VERTEX);
        // FragSprite.frag は s_texture0 と s_texture1 の2サンプラー宣言を含む
        SDL_GPUShader* fragShader = LoadShader(device,
            "FragSprite",
            2, 1, SDL_GPU_SHADERSTAGE_FRAGMENT);

        if (!vertShader || !fragShader)
        {
            if (vertShader) SDL_ReleaseGPUShader(device, vertShader);
            if (fragShader) SDL_ReleaseGPUShader(device, fragShader);
            return nullptr;
        }

        // 頂点入力設定
        SDL_GPUVertexBufferDescription vertexBufferDesc = {};
        vertexBufferDesc.slot = 0;
        vertexBufferDesc.pitch = sizeof(LAppSprite::SpriteVertex);
        vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        SDL_GPUVertexAttribute vertexAttributes[2] = {};
        // Position
        vertexAttributes[0].location = 0;
        vertexAttributes[0].buffer_slot = 0;
        vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertexAttributes[0].offset = offsetof(LAppSprite::SpriteVertex, x);
        // UV
        vertexAttributes[1].location = 1;
        vertexAttributes[1].buffer_slot = 0;
        vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertexAttributes[1].offset = offsetof(LAppSprite::SpriteVertex, u);

        SDL_GPUVertexInputState vertexInput = {};
        vertexInput.vertex_buffer_descriptions = &vertexBufferDesc;
        vertexInput.num_vertex_buffers = 1;
        vertexInput.vertex_attributes = vertexAttributes;
        vertexInput.num_vertex_attributes = 2;

        // カラーターゲット設定
        SDL_GPUColorTargetDescription colorTargetDesc = {};
        colorTargetDesc.format = swapchainFormat;
        colorTargetDesc.blend_state.src_color_blendfactor = srcBlendFactor;
        colorTargetDesc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargetDesc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        colorTargetDesc.blend_state.src_alpha_blendfactor = srcBlendFactor;
        colorTargetDesc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargetDesc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        colorTargetDesc.blend_state.enable_blend = true;
        colorTargetDesc.blend_state.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G |
                                                       SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;

        // パイプライン作成
        SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.vertex_shader = vertShader;
        pipelineInfo.fragment_shader = fragShader;
        pipelineInfo.vertex_input_state = vertexInput;
        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        pipelineInfo.target_info.num_color_targets = 1;
        pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;

        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);

        SDL_ReleaseGPUShader(device, vertShader);
        SDL_ReleaseGPUShader(device, fragShader);

        return pipeline;
    }

    /**
     * @brief スプライト用パイプラインを作成する（通常ブレンド）
     */
    SDL_GPUGraphicsPipeline* CreateSpritePipeline(SDL_GPUDevice* device, SDL_GPUTextureFormat swapchainFormat)
    {
        return CreateSpritePipelineCore(device, swapchainFormat, SDL_GPU_BLENDFACTOR_SRC_ALPHA);
    }

    /**
     * @brief モデル描画結果表示用スプライトパイプラインを作成する（乗算ブレンド）
     */
    SDL_GPUGraphicsPipeline* CreateModelSpritePipeline(SDL_GPUDevice* device, SDL_GPUTextureFormat swapchainFormat)
    {
        return CreateSpritePipelineCore(device, swapchainFormat, SDL_GPU_BLENDFACTOR_ONE);
    }
}
#endif

LAppView::LAppView()
    : LAppView_Common()
    , _touchManager(NULL)
    , _back(NULL)
    , _gear(NULL)
    , _power(NULL)
    , _renderSprite(NULL)
    , _renderTarget(SelectTarget_None)
#if defined(CSM_TARGET_OPENGL)
    , _spriteShader(NULL)
#elif defined(CSM_TARGET_VULKAN)
    , _spritePipeline(NULL)
    , _modelSpritePipeline(NULL)
#elif defined(CSM_TARGET_GPU)
    , _spritePipeline(NULL)
    , _modelSpritePipeline(NULL)
#endif
{
    _clearColor[0] = 1.0f;
    _clearColor[1] = 1.0f;
    _clearColor[2] = 1.0f;
    _clearColor[3] = 0.0f;

    // タッチ関係のイベント管理
    _touchManager = new TouchManager_Common();
}

LAppView::~LAppView()
{
    _renderBuffer.DestroyRenderTarget();
#if defined(CSM_TARGET_OPENGL)
    if (_spriteShader)
    {
        delete _spriteShader;
        _spriteShader = NULL;
    }
#elif defined(CSM_TARGET_VULKAN)
    if (_spritePipeline)
    {
        delete _spritePipeline;
        _spritePipeline = NULL;
    }
    if (_modelSpritePipeline)
    {
        delete _modelSpritePipeline;
        _modelSpritePipeline = NULL;
    }
#elif defined(CSM_TARGET_GPU)
    SDL_GPUDevice* device = LAppDelegate::GetInstance()->GetGPUDevice();
    if (_renderSprite)
    {
        _renderSprite->Release(device);
        delete _renderSprite;
        _renderSprite = NULL;
    }
    if (_spritePipeline)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, _spritePipeline);
        _spritePipeline = NULL;
    }
    if (_modelSpritePipeline)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, _modelSpritePipeline);
        _modelSpritePipeline = NULL;
    }
#endif

#if !defined(CSM_TARGET_GPU)
    if (_renderSprite)
    {
        delete _renderSprite;
        _renderSprite = NULL;
    }
#endif

    if (_touchManager)
    {
        delete _touchManager;
        _touchManager = NULL;
    }
    if (_back)
    {
#if defined(CSM_TARGET_GPU)
        _back->Release(device);
#endif
        delete _back;
        _back = NULL;
    }
    if (_gear)
    {
#if defined(CSM_TARGET_GPU)
        _gear->Release(device);
#endif
        delete _gear;
        _gear = NULL;
    }
    if (_power)
    {
#if defined(CSM_TARGET_GPU)
        _power->Release(device);
#endif
        delete _power;
        _power = NULL;
    }
}

void LAppView::Initialize(int width, int height)
{
    LAppView_Common::Initialize(width, height);

#if defined(CSM_TARGET_OPENGL)
    // シェーダー作成
    if (_spriteShader == NULL)
    {
        _spriteShader = new LAppSpriteShader();
    }
#endif

    InitializeSprite();
}

void LAppView::Render()
{
    LAppLive2DManager* live2DManager = LAppLive2DManager::GetInstance();
    if (!live2DManager)
    {
        return;
    }

#if defined(CSM_TARGET_OPENGL)
    int maxWidth = LAppDelegate::GetInstance()->GetWindowWidth();
    int maxHeight = LAppDelegate::GetInstance()->GetWindowHeight();

    _back->SetWindowSize(maxWidth, maxHeight);
    _gear->SetWindowSize(maxWidth, maxHeight);
    _power->SetWindowSize(maxWidth, maxHeight);

    _back->Render();
    _gear->Render();
    _power->Render();

    live2DManager->SetViewMatrix(_viewMatrix);

    // Cubism更新・描画
    live2DManager->OnUpdate();

    // 各モデルが持つ描画ターゲットをテクスチャとする場合
    if (_renderTarget == SelectTarget_ModelFrameBuffer && _renderSprite)
    {
        const GLfloat uvVertex[] =
        {
            1.0f, 1.0f,
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
        };

        for (Csm::csmUint32 i = 0; i < live2DManager->GetModelNum(); i++)
        {
            LAppModel* model = live2DManager->GetModel(i);
            float alpha = i < 1 ? 1.0f : model->GetOpacity();
            _renderSprite->SetColor(1.0f * alpha, 1.0f * alpha, 1.0f * alpha, alpha);

            if (model)
            {
                _renderSprite->SetWindowSize(maxWidth, maxHeight);
                _renderSprite->RenderImmidiate(model->GetRenderBuffer().GetColorBuffer(), uvVertex);
            }
        }
    }
#elif defined(CSM_TARGET_VULKAN)
    //スプライト描画
    int width, height;
    SDL_GetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
    VulkanManager* vkManager = LAppDelegate::GetInstance()->GetVulkanManager();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VkCommandBuffer commandBuffer = vkManager->BeginSingleTimeCommands();
    BeginRendering(commandBuffer, 0.0, 0.0, 0.0, 1.0, true);
    if(_spritePipeline->GetPipeline() != NULL && _spritePipeline->GetPipelineLayout() != NULL)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _spritePipeline->GetPipeline());
        _back->Render(commandBuffer, vkManager, width, height);
        _gear->Render(commandBuffer, vkManager, width, height);
        _power->Render(commandBuffer, vkManager, width, height);
    }
    EndRendering(commandBuffer);
    vkManager->SubmitCommand(commandBuffer, true);

    live2DManager->SetViewMatrix(_viewMatrix);

    // Cubism更新・描画
    live2DManager->OnUpdate();

    // 各モデルが持つ描画ターゲットをテクスチャとする場合
    if (_renderTarget == SelectTarget_ModelFrameBuffer && _renderSprite)
    {
        if(_modelSpritePipeline->GetPipeline() != NULL && _modelSpritePipeline->GetPipelineLayout() != NULL)
        {
            for (csmUint32 i = 0; i < live2DManager->GetModelNum(); i++)
            {
                LAppModel* model = live2DManager->GetModel(i);
                commandBuffer = vkManager->BeginSingleTimeCommands();
                _renderSprite->SetDescriptorUpdated(false);
                _renderSprite->UpdateDescriptorSet(vkManager->GetDevice(),
                                                            model->GetRenderBuffer().GetTextureView(),
                                                            model->GetRenderBuffer().GetTextureSampler());
                BeginRendering(commandBuffer, 0.f, 0.f, 0.3, 1.f, false);
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _modelSpritePipeline->GetPipeline());

                float alpha = i < 1 ? 1.0f : model->GetOpacity(); // 片方のみ不透明度を取得できるようにする
                _renderSprite->SetColor(alpha, alpha, alpha, alpha);
                if (model)
                {
                    _renderSprite->Render(commandBuffer, vkManager, width, height);
                }
                EndRendering(commandBuffer);
                vkManager->SubmitCommand(commandBuffer);
            }
        }
    }

    commandBuffer = vkManager->BeginSingleTimeCommands();
    ChangeEndLayout(commandBuffer);
    vkManager->SubmitCommand(commandBuffer);
#elif defined(CSM_TARGET_GPU)
    LAppDelegate* delegate = LAppDelegate::GetInstance();
    SDL_GPUCommandBuffer* commandBuffer = delegate->GetCurrentGPUCommandBuffer();
    SDL_GPUTexture* swapchainTexture = delegate->GetCurrentGPUSwapchainTexture();
    csmUint32 width = delegate->GetCurrentGPUSwapchainWidth();
    csmUint32 height = delegate->GetCurrentGPUSwapchainHeight();

    if (commandBuffer == NULL || swapchainTexture == NULL || width == 0 || height == 0)
    {
        return;
    }

    // 診断ログ（初回のみ）
    static bool s_diagOnce = false;
    if (!s_diagOnce)
    {
        s_diagOnce = true;
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("[APP]Render: cmdBuf=%p, swapTex=%p, size=%ux%u", commandBuffer, swapchainTexture, width, height);
            LAppPal::PrintLogLn("[APP]Render: _spritePipeline=%p", _spritePipeline);
            LAppPal::PrintLogLn("[APP]Render: _back=%p, _gear=%p, _power=%p", _back, _gear, _power);
        }
    }

    // 頂点データをアップロード（コピーパスはレンダーパスの前に行う必要がある）
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (copyPass)
    {
        if (_back)
        {
            _back->UploadVertexData(copyPass, width, height);
        }
        if (_gear)
        {
            _gear->UploadVertexData(copyPass, width, height);
        }
        if (_power)
        {
            _power->UploadVertexData(copyPass, width, height);
        }
        SDL_EndGPUCopyPass(copyPass);
    }
    else
    {
        LAppPal::PrintLogLn("[APP]ERROR: SDL_BeginGPUCopyPass returned NULL: %s", SDL_GetError());
    }

    // スプライト描画
    SDL_GPUColorTargetInfo colorTargetInfo = {};
    colorTargetInfo.texture = swapchainTexture;
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    colorTargetInfo.cycle = false;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);
    if (renderPass && _spritePipeline)
    {
        if (_back)
        {
            _back->Render(renderPass, commandBuffer, width, height);
        }
        if (_gear)
        {
            _gear->Render(renderPass, commandBuffer, width, height);
        }
        if (_power)
        {
            _power->Render(renderPass, commandBuffer, width, height);
        }
    }
    else if (!renderPass)
    {
        LAppPal::PrintLogLn("[APP]ERROR: SDL_BeginGPURenderPass returned NULL: %s", SDL_GetError());
    }
    else if (!_spritePipeline)
    {
        static bool s_pipelineWarn = false;
        if (!s_pipelineWarn)
        {
            s_pipelineWarn = true;
            LAppPal::PrintLogLn("[APP]WARNING: _spritePipeline is NULL, skipping sprite draw!");
        }
    }
    if (renderPass)
    {
        SDL_EndGPURenderPass(renderPass);
    }

    live2DManager->SetViewMatrix(_viewMatrix);

    // Cubism更新・描画
    live2DManager->OnUpdate();

    // 外部コマンドバッファの共有を解除
    Csm::Rendering::CubismRenderer_SDL3::SetExternalCommandBuffer(nullptr);

    // 各モデルが持つ描画ターゲットをテクスチャとする場合
    if (_renderTarget == SelectTarget_ModelFrameBuffer && _renderSprite)
    {
        if (_modelSpritePipeline)
        {
            for (csmUint32 i = 0; i < live2DManager->GetModelNum(); i++)
            {
                LAppModel* model = live2DManager->GetModel(i);
                if (model)
                {
                    // モデルのレンダーバッファのテクスチャを使用
                    Csm::Rendering::CubismRenderTarget_SDL3* renderBuffer = &model->GetRenderBuffer();
                    if (renderBuffer->IsValid())
                    {
                        _renderSprite->UpdateTexture(renderBuffer->GetTexture(), renderBuffer->GetTextureSampler());

                        // 頂点データをアップロード（コピーパスはレンダーパスの前に行う必要がある）
                        SDL_GPUCopyPass* modelCopyPass = SDL_BeginGPUCopyPass(commandBuffer);
                        if (modelCopyPass)
                        {
                            _renderSprite->UploadVertexData(modelCopyPass, width, height);
                            SDL_EndGPUCopyPass(modelCopyPass);
                        }

                        SDL_GPUColorTargetInfo modelColorTarget = {};
                        modelColorTarget.texture = swapchainTexture;
                        modelColorTarget.load_op = SDL_GPU_LOADOP_LOAD;
                        modelColorTarget.store_op = SDL_GPU_STOREOP_STORE;
                        modelColorTarget.cycle = false;

                        SDL_GPURenderPass* modelRenderPass = SDL_BeginGPURenderPass(commandBuffer, &modelColorTarget, 1, NULL);
                        if (modelRenderPass)
                        {
                            float alpha = i < 1 ? 1.0f : model->GetOpacity();
                            _renderSprite->SetColor(alpha, alpha, alpha, alpha);
                            _renderSprite->Render(modelRenderPass, commandBuffer, width, height);
                            SDL_EndGPURenderPass(modelRenderPass);
                        }
                    }
                }
            }
        }
    }
#endif
}

void LAppView::InitializeSprite()
{
    int width = LAppDelegate::GetInstance()->GetWindowWidth();
    int height = LAppDelegate::GetInstance()->GetWindowHeight();

    LAppTextureManager* textureManager = LAppDelegate::GetInstance()->GetTextureManager();
    const string resourcesPath = ResourcesPath;

#if defined(CSM_TARGET_OPENGL)
    GLuint programId = _spriteShader->GetShaderId();

    string imageName = BackImageName;
    LAppTextureManager::TextureInfo* backgroundTexture = textureManager->CreateTextureFromPngFile(resourcesPath + imageName);

    float x = width * 0.5f;
    float y = height * 0.5f;
    float fHeight = static_cast<float>(height * 0.95f);
    float ratio = fHeight / static_cast<float>(backgroundTexture->height);
    float fWidth = static_cast<float>(backgroundTexture->width) * ratio;
    _back = new LAppSprite(x, y, fWidth, fHeight, backgroundTexture->id, programId);

    imageName = GearImageName;
    LAppTextureManager::TextureInfo* gearTexture = textureManager->CreateTextureFromPngFile(resourcesPath + imageName);

    x = static_cast<float>(width - gearTexture->width * 0.5f);
    y = static_cast<float>(height - gearTexture->height * 0.5f);
    fWidth = static_cast<float>(gearTexture->width);
    fHeight = static_cast<float>(gearTexture->height);
    _gear = new LAppSprite(x, y, fWidth, fHeight, gearTexture->id, programId);

    imageName = PowerImageName;
    LAppTextureManager::TextureInfo* powerTexture = textureManager->CreateTextureFromPngFile(resourcesPath + imageName);

    x = static_cast<float>(width - powerTexture->width * 0.5f);
    y = static_cast<float>(powerTexture->height * 0.5f);
    fWidth = static_cast<float>(powerTexture->width);
    fHeight = static_cast<float>(powerTexture->height);
    _power = new LAppSprite(x, y, fWidth, fHeight, powerTexture->id, programId);

    // 画面全体を覆うサイズ
    x = width * 0.5f;
    y = height * 0.5f;
    _renderSprite = new LAppSprite(x, y, static_cast<float>(width), static_cast<float>(height), 0, programId);

#elif defined(CSM_TARGET_VULKAN)
    VulkanManager* vkManager = LAppDelegate::GetInstance()->GetVulkanManager();
    VkDevice device = vkManager->GetDevice();
    VkPhysicalDevice physicalDevice = vkManager->GetPhysicalDevice();
    SwapchainManager* swapchainManager = vkManager->GetSwapchainManager();

    // パイプライン作成
    _spritePipeline = new LAppSpritePipeline(
        device,
        swapchainManager->GetExtent(),
        swapchainManager->GetSwapchainImageFormat()
    );

    _modelSpritePipeline = new LAppModelSpritePipeline(
        device,
        swapchainManager->GetExtent(),
        swapchainManager->GetSwapchainImageFormat()
    );

    string imageName;
    CubismImageVulkan textureImage;
    float x;
    float y;
    float fWidth;
    float fHeight;

    imageName = BackImageName;
    LAppTextureManager::TextureInfo* backgroundTexture = textureManager->CreateTextureFromPngFile(
        resourcesPath + imageName, vkManager->GetImageFormat(), VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0.0);
    LAppDelegate::GetInstance()->GetTextureManager()->GetTexture(backgroundTexture->id, textureImage);

    x = width * 0.5f;
    y = height * 0.5f;
    fWidth = static_cast<float>(backgroundTexture->width * 2.0f);
    fHeight = static_cast<float>(height * 0.95f);
    _back = new LAppSprite(device, physicalDevice, vkManager, x, y, fWidth, fHeight, backgroundTexture->id,
                           _spritePipeline, textureImage.GetView(), textureImage.GetSampler());

    imageName = GearImageName;
    LAppTextureManager::TextureInfo* gearTexture = textureManager->CreateTextureFromPngFile(
        resourcesPath + imageName, vkManager->GetImageFormat(), VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0.0);
    LAppDelegate::GetInstance()->GetTextureManager()->GetTexture(gearTexture->id, textureImage);

    //X：右向き正、Y：下向き正
    x = static_cast<float>(width - gearTexture->width * 0.5f);
    y = static_cast<float>(gearTexture->height * 0.5f);
    fWidth = static_cast<float>(gearTexture->width);
    fHeight = static_cast<float>(gearTexture->height);
    _gear = new LAppSprite(device, physicalDevice, vkManager, x, y, fWidth, fHeight, gearTexture->id,
                           _spritePipeline, textureImage.GetView(), textureImage.GetSampler());

    imageName = PowerImageName;
    LAppTextureManager::TextureInfo* powerTexture = textureManager->CreateTextureFromPngFile(
        resourcesPath + imageName, vkManager->GetImageFormat(), VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0.0);
    LAppDelegate::GetInstance()->GetTextureManager()->GetTexture(powerTexture->id, textureImage);

    //X：右向き正、Y：下向き正
    x = static_cast<float>(width - powerTexture->width * 0.5f);
    y = static_cast<float>(height - powerTexture->height * 0.5f);
    fWidth = static_cast<float>(powerTexture->width);
    fHeight = static_cast<float>(powerTexture->height);
    _power = new LAppSprite(device, physicalDevice, vkManager, x, y, fWidth, fHeight, powerTexture->id,
                            _spritePipeline, textureImage.GetView(), textureImage.GetSampler());

    // 画面全体を覆うサイズ
    x = width * 0.5f;
    y = height * 0.5f;
    _renderSprite = new LAppSprite(device, physicalDevice, vkManager, x, y, static_cast<float>(width), static_cast<float>(height), 0, _modelSpritePipeline, NULL, NULL);
#elif defined(CSM_TARGET_GPU)
    SDL_GPUDevice* device = LAppDelegate::GetInstance()->GetGPUDevice();
    SDL_GPUTextureFormat swapchainFormat = LAppDelegate::GetInstance()->GetSwapchainFormat();

    // パイプライン作成
    _spritePipeline = CreateSpritePipeline(device, swapchainFormat);
    _modelSpritePipeline = CreateModelSpritePipeline(device, swapchainFormat);

    // 診断ログ: パイプライン作成結果
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("[APP]_spritePipeline: %s", _spritePipeline ? "OK" : "NULL (FAILED!)");
        LAppPal::PrintLogLn("[APP]_modelSpritePipeline: %s", _modelSpritePipeline ? "OK" : "NULL (FAILED!)");
    }

    string imageName;
    Live2D::Cubism::Framework::CubismImageSDL3 textureImage;
    float x;
    float y;
    float fWidth;
    float fHeight;

    imageName = BackImageName;
    LAppTextureManager::TextureInfo* backgroundTexture = textureManager->CreateTextureFromPngFile(resourcesPath + imageName, device, 1.0f);
    if (!backgroundTexture)
    {
        LAppPal::PrintLogLn("[APP]FAILED to load background texture: %s", imageName.c_str());
    }
    else
    {
        textureManager->GetTexture(backgroundTexture->id, textureImage);

        // 診断ログ: テクスチャ・サンプラー
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("[APP]background texture=%p, sampler=%p", textureImage.GetTexture(), textureImage.GetSampler());
        }

        x = width * 0.5f;
        y = height * 0.5f;
        fWidth = static_cast<float>(backgroundTexture->width * 2.0f);
        fHeight = static_cast<float>(height * 0.95f);
        _back = new LAppSprite(device, x, y, fWidth, fHeight, backgroundTexture->id,
                               _spritePipeline, textureImage.GetTexture(), textureImage.GetSampler());
    }

    imageName = GearImageName;
    LAppTextureManager::TextureInfo* gearTexture = textureManager->CreateTextureFromPngFile(resourcesPath + imageName, device, 1.0f);
    if (!gearTexture)
    {
        LAppPal::PrintLogLn("[APP]FAILED to load gear texture: %s", imageName.c_str());
    }
    else
    {
        textureManager->GetTexture(gearTexture->id, textureImage);

        //X：右向き正、Y：下向き正
        x = static_cast<float>(width - gearTexture->width * 0.5f);
        y = static_cast<float>(gearTexture->height * 0.5f);
        fWidth = static_cast<float>(gearTexture->width);
        fHeight = static_cast<float>(gearTexture->height);
        _gear = new LAppSprite(device, x, y, fWidth, fHeight, gearTexture->id,
                               _spritePipeline, textureImage.GetTexture(), textureImage.GetSampler());
    }

    imageName = PowerImageName;
    LAppTextureManager::TextureInfo* powerTexture = textureManager->CreateTextureFromPngFile(resourcesPath + imageName, device, 1.0f);
    if (!powerTexture)
    {
        LAppPal::PrintLogLn("[APP]FAILED to load power texture: %s", imageName.c_str());
    }
    else
    {
        textureManager->GetTexture(powerTexture->id, textureImage);

        //X：右向き正、Y：下向き正
        x = static_cast<float>(width - powerTexture->width * 0.5f);
        y = static_cast<float>(height - powerTexture->height * 0.5f);
        fWidth = static_cast<float>(powerTexture->width);
        fHeight = static_cast<float>(powerTexture->height);
        _power = new LAppSprite(device, x, y, fWidth, fHeight, powerTexture->id,
                                _spritePipeline, textureImage.GetTexture(), textureImage.GetSampler());
    }

    // 画面全体を覆うサイズ
    x = width * 0.5f;
    y = height * 0.5f;
    _renderSprite = new LAppSprite(device, x, y, static_cast<float>(width), static_cast<float>(height), 0,
                                    _modelSpritePipeline, NULL, NULL);
#endif
}

void LAppView::OnTouchesBegan(float px, float py) const
{
    _touchManager->TouchesBegan(px, py);
}

void LAppView::OnTouchesMoved(float px, float py) const
{
    float viewX = this->TransformViewX(_touchManager->GetX());
    float viewY = this->TransformViewY(_touchManager->GetY());

    _touchManager->TouchesMoved(px, py);

    LAppLive2DManager* Live2DManager = LAppLive2DManager::GetInstance();
    Live2DManager->OnDrag(viewX, viewY);
}

void LAppView::OnTouchesEnded(float px, float py) const
{
    // タッチ終了
    LAppLive2DManager* live2DManager = LAppLive2DManager::GetInstance();
    live2DManager->OnDrag(0.0f, 0.0f);
    {
        // シングルタップ
        float x = _deviceToScreen->TransformX(_touchManager->GetX());
        float y = _deviceToScreen->TransformY(_touchManager->GetY());
        if (DebugTouchLogEnable)
        {
            LAppPal::PrintLogLn("[APP]touchesEnded x:%.2f y:%.2f", x, y);
        }
        live2DManager->OnTap(x, y);

#if defined(CSM_TARGET_OPENGL)
        // 歯車にタップしたか
        if (_gear->IsHit(px, py))
        {
            live2DManager->NextScene();
        }

        // 電源ボタンにタップしたか
        if (_power->IsHit(px, py))
        {
            LAppDelegate::GetInstance()->AppEnd();
        }
#elif defined(CSM_TARGET_VULKAN)
        // 画面サイズの取得
        int windowWidth, windowHeight;
        SDL_GetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &windowWidth, &windowHeight);

        // 歯車にタップしたか
        if (_gear->IsHit(windowWidth, windowHeight, px, py))
        {
            live2DManager->NextScene();
        }

        // 電源ボタンにタップしたか
        if (_power->IsHit(windowWidth, windowHeight, px, py))
        {
            LAppDelegate::GetInstance()->AppEnd();
        }
#elif defined(CSM_TARGET_GPU)
        // 画面サイズの取得
        int windowWidth, windowHeight;
        SDL_GetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &windowWidth, &windowHeight);

        // 歯車にタップしたか
        if (_gear && _gear->IsHit(windowWidth, windowHeight, px, py))
        {
            live2DManager->NextScene();
        }

        // 電源ボタンにタップしたか
        if (_power && _power->IsHit(windowWidth, windowHeight, px, py))
        {
            LAppDelegate::GetInstance()->AppEnd();
        }
#endif
    }
}

void LAppView::PreModelDraw(LAppModel& refModel)
{
#if defined(CSM_TARGET_VULKAN)
    if (_renderTarget == SelectTarget_None)
    {
        // 通常のスワップチェーンイメージへと描画
        VulkanManager* vkManager = LAppDelegate::GetInstance()->GetVulkanManager();
        Csm::Rendering::CubismRenderer_Vulkan::SetRenderTarget(vkManager->GetSwapchainImage(),
                                                                vkManager->GetSwapchainImageView(),
                                                                vkManager->GetSwapchainManager()->GetSwapchainImageFormat(),
                                                                vkManager->GetSwapchainManager()->GetExtent());
    }
    else
    {
        // 別のレンダリングターゲットへ向けて描画する場合の使用するフレームバッファ
        Csm::Rendering::CubismRenderTarget_Vulkan* useTarget = NULL;
        useTarget = (_renderTarget == SelectTarget_ViewFrameBuffer) ? &_renderBuffer                // LAppView が持つバッファ
                                                                    : &refModel.GetRenderBuffer();  // Model が持つバッファ

        // 別のレンダリングターゲットへ向けて描画する場合
        if (!useTarget->IsValid())
        {
            // 描画ターゲット内部未作成の場合はここで作成
            int width, height;
            SDL_GetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
            VulkanManager* vkManager = LAppDelegate::GetInstance()->GetVulkanManager();
            if (width != 0 && height != 0)
            {
                useTarget->CreateRenderTarget(vkManager->GetDevice(), vkManager->GetPhysicalDevice(),
                    static_cast<csmUint32>(width), static_cast<csmUint32>(height),
                    vkManager->GetImageFormat(),
                    vkManager->GetDepthFormat()
                );
                _renderSprite->SetDescriptorUpdated(false);
            }
        }
        // 描画先を別のレンダリングターゲットへ指定
        LAppLive2DManager* live2DManager = LAppLive2DManager::GetInstance();
        Csm::Rendering::CubismRenderer_Vulkan::SetRenderTarget(useTarget->GetTextureImage(), useTarget->GetTextureView(),
                                                               LAppDelegate::GetInstance()->GetVulkanManager()->GetImageFormat(),
                                                               VkExtent2D{useTarget->GetBufferWidth(), useTarget->GetBufferHeight()});
    }
#elif defined(CSM_TARGET_OPENGL)
    if (_renderTarget != SelectTarget_None)
    {
        // 別のレンダリングターゲットへ向けて描画する場合
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        Csm::Rendering::CubismRenderTarget_OpenGLES2* useTarget;
        useTarget = (_renderTarget == SelectTarget_ViewFrameBuffer) ? &_renderBuffer : &refModel.GetRenderBuffer();

        if (!useTarget->IsValid())
        {
            int width = LAppDelegate::GetInstance()->GetWindowWidth();
            int height = LAppDelegate::GetInstance()->GetWindowHeight();
            if (width != 0 && height != 0)
            {
                useTarget->CreateRenderTarget(static_cast<Csm::csmUint32>(width), static_cast<Csm::csmUint32>(height));
            }
        }

        useTarget->BeginDraw();
        useTarget->Clear(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
    }
#elif defined(CSM_TARGET_GPU)
    // SDL3_GPU用のPreModelDraw処理
    {
        // メインレンダーループのコマンドバッファを共有する
        // （スワップチェインテクスチャのVulkanレイアウト追跡を正しく行うため）
        LAppDelegate* delegate = LAppDelegate::GetInstance();
        Csm::Rendering::CubismRenderer_SDL3::SetExternalCommandBuffer(
            delegate->GetCurrentGPUCommandBuffer()
        );
    }
    if (_renderTarget == SelectTarget_None)
    {
        // 通常のスワップチェーンテクスチャへと描画
        LAppDelegate* delegate = LAppDelegate::GetInstance();
        Csm::Rendering::CubismRenderer_SDL3::SetRenderTarget(
            delegate->GetCurrentGPUSwapchainTexture(),
            delegate->GetSwapchainFormat(),
            delegate->GetCurrentGPUSwapchainWidth(),
            delegate->GetCurrentGPUSwapchainHeight()
        );
    }
    else
    {
        // 別のレンダリングターゲットへ向けて描画する場合
        Csm::Rendering::CubismRenderTarget_SDL3* useTarget;
        useTarget = (_renderTarget == SelectTarget_ViewFrameBuffer) ? &_renderBuffer : &refModel.GetRenderBuffer();

        if (!useTarget->IsValid())
        {
            int width, height;
            SDL_GetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
            LAppDelegate* delegate = LAppDelegate::GetInstance();
            SDL_GPUDevice* device = delegate->GetGPUDevice();
            if (width != 0 && height != 0)
            {
                useTarget->CreateRenderTarget(device, static_cast<csmUint32>(width), static_cast<csmUint32>(height),
                    delegate->GetSwapchainFormat(), delegate->GetDepthFormat());
            }
        }

        // 描画先を別のレンダリングターゲットへ指定
        Csm::Rendering::CubismRenderer_SDL3::SetRenderTarget(
            useTarget->GetTexture(),
            LAppDelegate::GetInstance()->GetSwapchainFormat(),
            useTarget->GetBufferWidth(),
            useTarget->GetBufferHeight()
        );
    }
#endif
}

#if defined(CSM_TARGET_OPENGL)
void LAppView::PostModelDraw(LAppModel& refModel)
{
    if (_renderTarget != SelectTarget_None)
    {
        Csm::Rendering::CubismRenderTarget_OpenGLES2* useTarget;
        useTarget = (_renderTarget == SelectTarget_ViewFrameBuffer) ? &_renderBuffer : &refModel.GetRenderBuffer();

        useTarget->EndDraw();

        if (_renderTarget == SelectTarget_ViewFrameBuffer && _renderSprite)
        {
            const GLfloat uvVertex[] =
            {
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 0.0f,
            };

            _renderSprite->SetColor(1.0f * GetSpriteAlpha(0), 1.0f * GetSpriteAlpha(0), 1.0f * GetSpriteAlpha(0), GetSpriteAlpha(0));

            int maxWidth = LAppDelegate::GetInstance()->GetWindowWidth();
            int maxHeight = LAppDelegate::GetInstance()->GetWindowHeight();
            _renderSprite->SetWindowSize(maxWidth, maxHeight);

            _renderSprite->RenderImmidiate(useTarget->GetColorBuffer(), uvVertex);
        }
    }
}
#elif defined(CSM_TARGET_VULKAN)
void LAppView::PostModelDraw(LAppModel& refModel, csmInt32 modelIndex)
{
    // 別のレンダリングターゲットへ向けて描画する場合の使用するフレームバッファ
    Csm::Rendering::CubismRenderTarget_Vulkan* useTarget = NULL;

    if (_renderTarget != SelectTarget_None)
    {
        // 別のレンダリングターゲットへ向けて描画する場合

        // 使用するターゲット
        useTarget = (_renderTarget == SelectTarget_ViewFrameBuffer) ? &_renderBuffer : &refModel.GetRenderBuffer();

        // LAppViewの持つフレームバッファを使うなら、スプライトへの描画はここ
        if (_renderTarget == SelectTarget_ViewFrameBuffer && _renderSprite)
        {
            int width, height;
            SDL_GetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
            VulkanManager* vkManager = LAppDelegate::GetInstance()->GetVulkanManager();
            VkCommandBuffer commandBuffer = vkManager->BeginSingleTimeCommands();
            _renderSprite->UpdateDescriptorSet(vkManager->GetDevice(), useTarget->GetTextureView(),
                                                        useTarget->GetTextureSampler());
            BeginRendering(commandBuffer, 0.f, 0.f, 0.3, 1.f, false);
            if(_modelSpritePipeline->GetPipeline() != NULL && _modelSpritePipeline->GetPipelineLayout() != NULL)
            {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _modelSpritePipeline->GetPipeline());
                float alpha = GetSpriteAlpha(0);
                _renderSprite->SetColor(alpha, alpha, alpha, alpha);
                _renderSprite->Render(commandBuffer, vkManager, width, height);
            }
            EndRendering(commandBuffer);
            vkManager->SubmitCommand(commandBuffer);
        }
    }

    // レンダーターゲットをスワップチェーンのものへと戻す
    VulkanManager* vkManager = LAppDelegate::GetInstance()->GetVulkanManager();
    Csm::Rendering::CubismRenderer_Vulkan::SetRenderTarget(vkManager->GetSwapchainImage(),
                                                            vkManager->GetSwapchainImageView(),
                                                            vkManager->GetSwapchainManager()->GetSwapchainImageFormat(),
                                                            vkManager->GetSwapchainManager()->GetExtent());
}
#elif defined(CSM_TARGET_GPU)
void LAppView::PostModelDraw(LAppModel& refModel, csmInt32 modelIndex)
{
    LAppDelegate* delegate = LAppDelegate::GetInstance();
    SDL_GPUCommandBuffer* commandBuffer = delegate->GetCurrentGPUCommandBuffer();
    SDL_GPUTexture* swapchainTexture = delegate->GetCurrentGPUSwapchainTexture();
    csmUint32 width = delegate->GetCurrentGPUSwapchainWidth();
    csmUint32 height = delegate->GetCurrentGPUSwapchainHeight();

    if (commandBuffer == NULL || swapchainTexture == NULL || width == 0 || height == 0)
    {
        return;
    }

    // SDL3_GPU用のPostModelDraw処理
    if (_renderTarget == SelectTarget_ViewFrameBuffer && _renderSprite)
    {
        Csm::Rendering::CubismRenderTarget_SDL3* useTarget = &_renderBuffer;
        if (useTarget->IsValid())
        {
            _renderSprite->UpdateTexture(useTarget->GetTexture(), useTarget->GetTextureSampler());

            // 頂点データをアップロード（コピーパスはレンダーパスの前に行う必要がある）
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
            if (copyPass)
            {
                _renderSprite->UploadVertexData(copyPass, width, height);
                SDL_EndGPUCopyPass(copyPass);
            }

            SDL_GPUColorTargetInfo colorTargetInfo = {};
            colorTargetInfo.texture = swapchainTexture;
            colorTargetInfo.load_op = SDL_GPU_LOADOP_LOAD;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
            colorTargetInfo.cycle = false;

            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);
            if (renderPass && _modelSpritePipeline)
            {
                _renderSprite->Render(renderPass, commandBuffer, width, height);
                SDL_EndGPURenderPass(renderPass);
            }
        }
    }
}
#endif

#if defined(CSM_TARGET_VULKAN)
void LAppView::BeginRendering(VkCommandBuffer commandBuffer, float r, float g, float b, float a, bool isClear)
{
    VulkanManager* vkManager = LAppDelegate::GetInstance()->GetVulkanManager();
    VkRenderingAttachmentInfoKHR colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView = vkManager->GetSwapchainImageView();
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    if (isClear)
    {
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    }
    else
    {
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    }
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = {r, g, b, a};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, {vkManager->GetSwapchainManager()->GetExtent()}};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
}

void LAppView::ChangeEndLayout(VkCommandBuffer commandBuffer)
{
    VulkanManager* vkManager = LAppDelegate::GetInstance()->GetVulkanManager();
    VkImageMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    memoryBarrier.dstAccessMask = 0;
    memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    memoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    memoryBarrier.image = vkManager->GetSwapchainImage();
    memoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
}

void LAppView::EndRendering(VkCommandBuffer commandBuffer)
{
    vkCmdEndRendering(commandBuffer);
}

void LAppView::ResizeSprite(int width, int height)
{
    VulkanManager* vkManager = LAppDelegate::GetInstance()->GetVulkanManager();
    VkDevice device = vkManager->GetDevice();
    SwapchainManager* swapchainManager = vkManager->GetSwapchainManager();
    if (_spritePipeline)
    {
        delete _spritePipeline;
    }
    if (_modelSpritePipeline)
    {
        delete _modelSpritePipeline;
    }
    _spritePipeline = new LAppSpritePipeline(device, swapchainManager->GetExtent(), vkManager->GetSwapchainManager()->GetSwapchainImageFormat());
    _modelSpritePipeline = new LAppModelSpritePipeline(device, swapchainManager->GetExtent(), vkManager->GetSwapchainManager()->GetSwapchainImageFormat());

    LAppTextureManager* textureManager = LAppDelegate::GetInstance()->GetTextureManager();
    if (!textureManager)
    {
        return;
    }

    float x = 0.0f;
    float y = 0.0f;
    float fWidth = 0.0f;
    float fHeight = 0.0f;

    if (_back)
    {
        _back->SetPipeline(_spritePipeline);
        uint32_t id = _back->GetTextureId();
        LAppTextureManager::TextureInfo* texInfo = textureManager->GetTextureInfoById(id);
        if (texInfo)
        {
            x = width * 0.5f;
            y = height * 0.5f;
            fWidth = static_cast<float>(texInfo->width * 2);
            fHeight = static_cast<float>(height) * 0.95f;
            _back->ResetRect(x, y, fWidth, fHeight);
        }
    }

    if (_power)
    {
        _power->SetPipeline(_spritePipeline);
        uint32_t id = _power->GetTextureId();
        LAppTextureManager::TextureInfo* texInfo = textureManager->GetTextureInfoById(id);
        if (texInfo)
        {
            x = static_cast<float>(width - texInfo->width * 0.5f);
            y = static_cast<float>(height - texInfo->height * 0.5f);
            fWidth = static_cast<float>(texInfo->width);
            fHeight = static_cast<float>(texInfo->height);
            _power->ResetRect(x, y, fWidth, fHeight);
        }
    }

    if (_gear)
    {
        _gear->SetPipeline(_spritePipeline);
        uint32_t id = _gear->GetTextureId();
        LAppTextureManager::TextureInfo* texInfo = textureManager->GetTextureInfoById(id);
        if (texInfo)
        {
            x = static_cast<float>(width - texInfo->width * 0.5f);
            y = static_cast<float>(texInfo->height * 0.5f);
            fWidth = static_cast<float>(texInfo->width);
            fHeight = static_cast<float>(texInfo->height);
            _gear->ResetRect(x, y, fWidth, fHeight);
        }
    }
    if (_renderSprite)
    {
        _renderSprite->SetPipeline(_modelSpritePipeline);
        x = width * 0.5f;
        y = height * 0.5f;
        _renderSprite->ResetRect(x, y, static_cast<float>(width), static_cast<float>(height));
    }
}

void LAppView::DestroyRenderTarget()
{
    LAppLive2DManager* live2DManager = LAppLive2DManager::GetInstance();
    if (_renderTarget == SelectTarget_ViewFrameBuffer)
    {
        _renderBuffer.DestroyRenderTarget();
    }
    else if (_renderTarget == SelectTarget_ModelFrameBuffer)
    {
        for (csmUint32 i = 0; i < live2DManager->GetModelNum(); i++)
        {
            LAppModel* model = live2DManager->GetModel(i);
            model->GetRenderBuffer().
                   DestroyRenderTarget();
        }
    }
}
#elif defined(CSM_TARGET_GPU)
void LAppView::ResizeSprite(int width, int height)
{
    SDL_GPUDevice* device = LAppDelegate::GetInstance()->GetGPUDevice();
    SDL_GPUTextureFormat swapchainFormat = LAppDelegate::GetInstance()->GetSwapchainFormat();

    if (_spritePipeline)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, _spritePipeline);
    }
    if (_modelSpritePipeline)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, _modelSpritePipeline);
    }
    _spritePipeline = CreateSpritePipeline(device, swapchainFormat);
    _modelSpritePipeline = CreateModelSpritePipeline(device, swapchainFormat);

    LAppTextureManager* textureManager = LAppDelegate::GetInstance()->GetTextureManager();
    if (!textureManager)
    {
        return;
    }

    float x = 0.0f;
    float y = 0.0f;
    float fWidth = 0.0f;
    float fHeight = 0.0f;

    if (_back)
    {
        _back->SetPipeline(_spritePipeline);
        csmUint32 id = _back->GetTextureId();
        LAppTextureManager::TextureInfo* texInfo = textureManager->GetTextureInfoById(id);
        if (texInfo)
        {
            x = width * 0.5f;
            y = height * 0.5f;
            fWidth = static_cast<float>(texInfo->width * 2);
            fHeight = static_cast<float>(height) * 0.95f;
            _back->ResetRect(x, y, fWidth, fHeight);
        }
    }

    if (_power)
    {
        _power->SetPipeline(_spritePipeline);
        csmUint32 id = _power->GetTextureId();
        LAppTextureManager::TextureInfo* texInfo = textureManager->GetTextureInfoById(id);
        if (texInfo)
        {
            x = static_cast<float>(width - texInfo->width * 0.5f);
            y = static_cast<float>(height - texInfo->height * 0.5f);
            fWidth = static_cast<float>(texInfo->width);
            fHeight = static_cast<float>(texInfo->height);
            _power->ResetRect(x, y, fWidth, fHeight);
        }
    }

    if (_gear)
    {
        _gear->SetPipeline(_spritePipeline);
        csmUint32 id = _gear->GetTextureId();
        LAppTextureManager::TextureInfo* texInfo = textureManager->GetTextureInfoById(id);
        if (texInfo)
        {
            x = static_cast<float>(width - texInfo->width * 0.5f);
            y = static_cast<float>(texInfo->height * 0.5f);
            fWidth = static_cast<float>(texInfo->width);
            fHeight = static_cast<float>(texInfo->height);
            _gear->ResetRect(x, y, fWidth, fHeight);
        }
    }
    if (_renderSprite)
    {
        _renderSprite->SetPipeline(_modelSpritePipeline);
        x = width * 0.5f;
        y = height * 0.5f;
        _renderSprite->ResetRect(x, y, static_cast<float>(width), static_cast<float>(height));
    }
}

void LAppView::DestroyRenderTarget()
{
    LAppLive2DManager* live2DManager = LAppLive2DManager::GetInstance();
    if (_renderTarget == SelectTarget_ViewFrameBuffer)
    {
        _renderBuffer.DestroyRenderTarget();
    }
    else if (_renderTarget == SelectTarget_ModelFrameBuffer)
    {
        for (csmUint32 i = 0; i < live2DManager->GetModelNum(); i++)
        {
            LAppModel* model = live2DManager->GetModel(i);
            model->GetRenderBuffer().DestroyRenderTarget();
        }
    }
}
#endif

#if defined(CSM_TARGET_OPENGL)
void LAppView::ResizeSprite()
{
    LAppTextureManager* textureManager = LAppDelegate::GetInstance()->GetTextureManager();
    if (!textureManager)
    {
        return;
    }

    int width = LAppDelegate::GetInstance()->GetWindowWidth();
    int height = LAppDelegate::GetInstance()->GetWindowHeight();

    float x = 0.0f;
    float y = 0.0f;
    float fWidth = 0.0f;
    float fHeight = 0.0f;

    if (_back)
    {
        GLuint id = _back->GetTextureId();
        LAppTextureManager::TextureInfo* texInfo = textureManager->GetTextureInfoById(id);
        if (texInfo)
        {
            x = width * 0.5f;
            y = height * 0.5f;
            fHeight = static_cast<float>(height) * 0.95f;
            float ratio = fHeight / static_cast<float>(texInfo->height);
            fWidth = static_cast<float>(texInfo->width) * ratio;
            _back->ResetRect(x, y, fWidth, fHeight);
        }
    }

    if (_power)
    {
        GLuint id = _power->GetTextureId();
        LAppTextureManager::TextureInfo* texInfo = textureManager->GetTextureInfoById(id);
        if (texInfo)
        {
            x = static_cast<float>(width - texInfo->width * 0.5f);
            y = static_cast<float>(texInfo->height * 0.5f);
            fWidth = static_cast<float>(texInfo->width);
            fHeight = static_cast<float>(texInfo->height);
            _power->ResetRect(x, y, fWidth, fHeight);
        }
    }

    if (_gear)
    {
        GLuint id = _gear->GetTextureId();
        LAppTextureManager::TextureInfo* texInfo = textureManager->GetTextureInfoById(id);
        if (texInfo)
        {
            x = static_cast<float>(width - texInfo->width * 0.5f);
            y = static_cast<float>(height - texInfo->height * 0.5f);
            fWidth = static_cast<float>(texInfo->width);
            fHeight = static_cast<float>(texInfo->height);
            _gear->ResetRect(x, y, fWidth, fHeight);
        }
    }
}
#endif

void LAppView::SwitchRenderingTarget(SelectTarget targetType)
{
    _renderTarget = targetType;
}

void LAppView::SetRenderTargetClearColor(float r, float g, float b)
{
    _clearColor[0] = r;
    _clearColor[1] = g;
    _clearColor[2] = b;
}

float LAppView::GetSpriteAlpha(int assign) const
{
    float alpha = 0.4f + static_cast<float>(assign) * 0.5f;
    if (alpha > 1.0f)
    {
        alpha = 1.0f;
    }
    if (alpha < 0.1f)
    {
        alpha = 0.1f;
    }

    return alpha;
}
