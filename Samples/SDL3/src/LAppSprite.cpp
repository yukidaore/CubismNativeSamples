/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppSprite.hpp"
#include "LAppPal.hpp"

#if defined(CSM_TARGET_VULKAN)
#include "VulkanManager.hpp"
#include "LAppSpritePipeline.hpp"
#elif defined(CSM_TARGET_GPU)
#include "LAppDelegate.hpp"
#endif

using namespace Csm;

#if defined(CSM_TARGET_OPENGL)
LAppSprite::LAppSprite(float x, float y, float width, float height, GLuint textureId, GLuint programId)
    : _textureId(textureId)
    , _maxWidth(0)
    , _maxHeight(0)
{
    _rect.left = (x - width * 0.5f);
    _rect.right = (x + width * 0.5f);
    _rect.up = (y + height * 0.5f);
    _rect.down = (y - height * 0.5f);

    // 何番目のattribute変数か
    _positionLocation = glGetAttribLocation(programId, "position");
    _uvLocation = glGetAttribLocation(programId, "uv");
    _textureLocation = glGetUniformLocation(programId, "texture");
    _colorLocation = glGetUniformLocation(programId, "baseColor");

    _spriteColor[0] = 1.0f;
    _spriteColor[1] = 1.0f;
    _spriteColor[2] = 1.0f;
    _spriteColor[3] = 1.0f;
}

LAppSprite::~LAppSprite()
{
}

void LAppSprite::Render() const
{
    if (_maxWidth == 0 || _maxHeight == 0)
    {
        return; // この際は描画できず
    }

    const GLfloat uvVertex[] =
    {
        1.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
    };

    //透過設定
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // attribute属性を有効にする
    glEnableVertexAttribArray(_positionLocation);
    glEnableVertexAttribArray(_uvLocation);

    // uniform属性の登録
    glUniform1i(_textureLocation, 0);

    // 頂点データ
    float positionVertex[] =
    {
        (_rect.right - _maxWidth * 0.5f) / (_maxWidth * 0.5f), (_rect.up   - _maxHeight * 0.5f) / (_maxHeight * 0.5f),
        (_rect.left  - _maxWidth * 0.5f) / (_maxWidth * 0.5f), (_rect.up   - _maxHeight * 0.5f) / (_maxHeight * 0.5f),
        (_rect.left  - _maxWidth * 0.5f) / (_maxWidth * 0.5f), (_rect.down - _maxHeight * 0.5f) / (_maxHeight * 0.5f),
        (_rect.right - _maxWidth * 0.5f) / (_maxWidth * 0.5f), (_rect.down - _maxHeight * 0.5f) / (_maxHeight * 0.5f)
    };

    // attribute属性を登録
    glVertexAttribPointer(_positionLocation, 2, GL_FLOAT, false, 0, positionVertex);
    glVertexAttribPointer(_uvLocation, 2, GL_FLOAT, false, 0, uvVertex);

    glUniform4f(_colorLocation, _spriteColor[0], _spriteColor[1], _spriteColor[2], _spriteColor[3]);

    // モデルの描画
    glBindTexture(GL_TEXTURE_2D, _textureId);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void LAppSprite::RenderImmidiate(GLuint textureId, const GLfloat uvVertex[8]) const
{
    if (_maxWidth == 0 || _maxHeight == 0)
    {
        return; // この際は描画できず
    }

    // attribute属性を有効にする
    glEnableVertexAttribArray(_positionLocation);
    glEnableVertexAttribArray(_uvLocation);

    // uniform属性の登録
    glUniform1i(_textureLocation, 0);

    // 頂点データ
    float positionVertex[] =
    {
        (_rect.right - _maxWidth * 0.5f) / (_maxWidth * 0.5f), (_rect.up - _maxHeight * 0.5f) / (_maxHeight * 0.5f),
        (_rect.left - _maxWidth * 0.5f) / (_maxWidth * 0.5f), (_rect.up - _maxHeight * 0.5f) / (_maxHeight * 0.5f),
        (_rect.left - _maxWidth * 0.5f) / (_maxWidth * 0.5f), (_rect.down - _maxHeight * 0.5f) / (_maxHeight * 0.5f),
        (_rect.right - _maxWidth * 0.5f) / (_maxWidth * 0.5f), (_rect.down - _maxHeight * 0.5f) / (_maxHeight * 0.5f)
    };

    // attribute属性を登録
    glVertexAttribPointer(_positionLocation, 2, GL_FLOAT, false, 0, positionVertex);
    glVertexAttribPointer(_uvLocation, 2, GL_FLOAT, false, 0, uvVertex);

    glUniform4f(_colorLocation, _spriteColor[0], _spriteColor[1], _spriteColor[2], _spriteColor[3]);

    // モデルの描画
    glBindTexture(GL_TEXTURE_2D, textureId);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

bool LAppSprite::IsHit(float pointX, float pointY) const
{
    if (_maxWidth == 0 || _maxHeight == 0)
    {
        return false; // この際は描画できず
    }

    //Y座標は変換する必要あり
    float y = _maxHeight - pointY;

    return (pointX >= _rect.left && pointX <= _rect.right && y <= _rect.up && y >= _rect.down);
}

#elif defined(CSM_TARGET_VULKAN)

LAppSprite::LAppSprite(
    VkDevice device, VkPhysicalDevice physicalDevice, VulkanManager* vkManager,
    float x, float y, float width, float height,
    Csm::csmUint32 textureId, LAppSpritePipeline* pipeline,
    VkImageView view, VkSampler sampler)
    : LAppSprite_Common(textureId),
    _rect(),
    _pipeline(pipeline)
{
    _rect.left = (x - width * 0.5f);
    _rect.right = (x + width * 0.5f);
    _rect.up = (y - height * 0.5f);
    _rect.down = (y + height * 0.5f);

    _spriteColor[0] = 1.0f;
    _spriteColor[1] = 1.0f;
    _spriteColor[2] = 1.0f;
    _spriteColor[3] = 1.0f;

    if (_vertexBuffer.GetBuffer() == VK_NULL_HANDLE)
    {
        VkDeviceSize bufferSize = sizeof(SpriteVertex) * VertexNum; // 総長 構造体サイズ*個数
        _stagingBuffer.CreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        _stagingBuffer.Map(device, bufferSize);
        _vertexBuffer.CreateBuffer(device, physicalDevice, bufferSize,
                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    if (_indexBuffer.GetBuffer() == VK_NULL_HANDLE)
    {
        uint16_t idx[IndexNum] = {
            0, 1, 2,
            1, 3, 2
        };

        uint64_t bufferSize = sizeof(uint16_t) * IndexNum;

        CubismBufferVulkan stagingBuffer;
        stagingBuffer.CreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stagingBuffer.Map(device, bufferSize);
        stagingBuffer.MemCpy(idx, bufferSize);
        stagingBuffer.UnMap(device);

        _indexBuffer.CreateBuffer(device, physicalDevice, bufferSize,
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkCommandBuffer commandBuffer = vkManager->BeginSingleTimeCommands();
        VkBufferCopy copyRegion{};
        copyRegion.size = bufferSize;
        vkCmdCopyBuffer(commandBuffer, stagingBuffer.GetBuffer(), _indexBuffer.GetBuffer(), 1, &copyRegion);
        vkManager->SubmitCommand(commandBuffer);

        stagingBuffer.Destroy(device);
    }

    if (_uniformBuffer.GetBuffer() == VK_NULL_HANDLE)
    {
        _uniformBuffer.CreateBuffer(device, physicalDevice, sizeof(SpriteUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        _uniformBuffer.Map(device, VK_WHOLE_SIZE);
    }

    VkDescriptorPoolSize poolSizes[2]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS)
    {
        LAppPal::PrintLogLn("failed to create descriptor pool!");
    }

    CreateDescriptorSet(device, _pipeline->GetDescriptorSetLayout());
    if (textureId != 0)
    {
        SetDescriptorUpdated(false);
        UpdateDescriptorSet(device, view, sampler);
    }
}

LAppSprite::~LAppSprite()
{}

void LAppSprite::Release(VkDevice device)
{
    _vertexBuffer.Destroy(device);
    _stagingBuffer.Destroy(device);
    _indexBuffer.Destroy(device);
    _uniformBuffer.Destroy(device);
    vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
}

void LAppSprite::CreateDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &_descriptorSet) != VK_SUCCESS)
    {
        LAppPal::PrintLogLn("failed to allocate descriptor sets!");
    }
}

void LAppSprite::UpdateData(VulkanManager* vkManager, int maxWidth, int maxHeight) const
{
    if (maxWidth == 0 || maxHeight == 0)
    {
        return; // この際は描画できず
    }

    SpriteVertex vtx[VertexNum] = {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.5f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.5f, 0.0f, 1.0f},
        {0.5f, 0.5f, 1.0f, 1.0f},
    };

    vtx[0].x = (_rect.left - maxWidth * 0.5f) / (maxWidth * 0.5f);
    vtx[0].y = (_rect.down - maxHeight * 0.5f) / (maxHeight * 0.5f);
    vtx[1].x = (_rect.right - maxWidth * 0.5f) / (maxWidth * 0.5f);
    vtx[1].y = (_rect.down - maxHeight * 0.5f) / (maxHeight * 0.5f);
    vtx[2].x = (_rect.left - maxWidth * 0.5f) / (maxWidth * 0.5f);
    vtx[2].y = (_rect.up - maxHeight * 0.5f) / (maxHeight * 0.5f);
    vtx[3].x = (_rect.right - maxWidth * 0.5f) / (maxWidth * 0.5f);
    vtx[3].y = (_rect.up - maxHeight * 0.5f) / (maxHeight * 0.5f);

    SpriteVertex vertex;
    csmVector<SpriteVertex> vertices;
    for (SpriteVertex vt : vtx)
    {
        vertex.x = vt.x;
        vertex.y = vt.y;
        vertex.u = vt.u;
        vertex.v = vt.v;
        vertices.PushBack(vertex);
    }

    VkCommandBuffer commandBuffer = vkManager->BeginSingleTimeCommands();
    csmUint32 bufferSize = sizeof(SpriteVertex) * vertices.GetSize();

    _stagingBuffer.MemCpy(vertices.GetPtr(), bufferSize);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(commandBuffer, _stagingBuffer.GetBuffer(), _vertexBuffer.GetBuffer(), 1, &copyRegion);

    vkManager->SubmitCommand(commandBuffer);

    //ユニフォームバッファ設定
    SpriteUBO _ubo;
    _ubo.spriteColor[0] = _spriteColor[0];
    _ubo.spriteColor[1] = _spriteColor[1];
    _ubo.spriteColor[2] = _spriteColor[2];
    _ubo.spriteColor[3] = _spriteColor[3];
    _uniformBuffer.MemCpy(&_ubo, sizeof(SpriteUBO));
}

void LAppSprite::UpdateDescriptorSet(VkDevice device, VkImageView view, VkSampler sampler)
{
    if (!isDescriptorUpdated)
    {
        csmVector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.Resize(2);

        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = _uniformBuffer.GetBuffer();
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = VK_WHOLE_SIZE;

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = _descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = view;
        imageInfo.sampler = sampler;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = _descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, descriptorWrites.GetSize(), descriptorWrites.GetPtr(), 0, NULL);
        isDescriptorUpdated = true;
    }
}

void LAppSprite::SetDescriptorUpdated(bool frag)
{
    isDescriptorUpdated = frag;
}

void LAppSprite::Render(VkCommandBuffer commandBuffer, VulkanManager* vkManager, int windowWidth, int windowHeight)
{
    UpdateData(vkManager, windowWidth, windowHeight);
    VkBuffer vertexBuffers[] = {_vertexBuffer.GetBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->GetPipelineLayout(), 0, 1, &_descriptorSet, 0,nullptr);
    vkCmdDrawIndexed(commandBuffer, IndexNum, 1, 0, 0, 0);
}

bool LAppSprite::IsHit(int windowWidth, int windowHeight, float pointX, float pointY) const
{
    if (windowWidth == 0 || windowHeight == 0)
    {
        return false; // この際は描画できず
    }

    float y = pointY;

    return (pointX >= _rect.left && pointX <= _rect.right && y >= _rect.up && y <= _rect.down);
}

void LAppSprite::SetPipeline(LAppSpritePipeline* pipeline)
{
    _pipeline = pipeline;
}
#elif defined(CSM_TARGET_GPU)

LAppSprite::LAppSprite(
    SDL_GPUDevice* device,
    float x, float y, float width, float height,
    Csm::csmUint32 textureId, SDL_GPUGraphicsPipeline* pipeline,
    SDL_GPUTexture* texture, SDL_GPUSampler* sampler)
    : LAppSprite_Common(textureId),
    _rect(),
    _pipeline(pipeline),
    _gpuDevice(device),
    _texture(texture),
    _sampler(sampler)
{
    _rect.left = (x - width * 0.5f);
    _rect.right = (x + width * 0.5f);
    _rect.up = (y - height * 0.5f);
    _rect.down = (y + height * 0.5f);

    _spriteColor[0] = 1.0f;
    _spriteColor[1] = 1.0f;
    _spriteColor[2] = 1.0f;
    _spriteColor[3] = 1.0f;

    // 頂点バッファ作成
    if (_vertexBuffer.GetBuffer() == nullptr)
    {
        csmUint32 bufferSize = sizeof(SpriteVertex) * VertexNum;
        _vertexBuffer.CreateBuffer(device, bufferSize, SDL_GPU_BUFFERUSAGE_VERTEX);
    }

    // インデックスバッファ作成
    if (_indexBuffer.GetBuffer() == nullptr)
    {
        uint16_t idx[IndexNum] = {
            0, 1, 2,
            1, 3, 2
        };

        uint32_t bufferSize = sizeof(uint16_t) * IndexNum;
        _indexBuffer.CreateBuffer(device, bufferSize, SDL_GPU_BUFFERUSAGE_INDEX);

        // データをアップロード
        Live2D::Cubism::Framework::CubismBufferSDL3 transferBuffer;
        transferBuffer.CreateTransferBuffer(device, bufferSize, SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD);
        void* mapped = transferBuffer.MapTransferBuffer(device, false);
        if (mapped)
        {
            memcpy(mapped, idx, bufferSize);
            transferBuffer.UnmapTransferBuffer(device);

            SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(device);
            if (cmdBuf)
            {
                SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);
                if (copyPass)
                {
                    _indexBuffer.UploadToBuffer(copyPass, &transferBuffer, bufferSize);
                    SDL_EndGPUCopyPass(copyPass);
                }
                SDL_SubmitGPUCommandBuffer(cmdBuf);
            }
        }
        transferBuffer.Destroy(device);
    }
}

LAppSprite::~LAppSprite()
{
}

void LAppSprite::Release(SDL_GPUDevice* device)
{
    _vertexBuffer.Destroy(device);
    _indexBuffer.Destroy(device);
    _vertexTransferBuffer.Destroy(device);
}

void LAppSprite::Render(SDL_GPURenderPass* renderPass, SDL_GPUCommandBuffer* commandBuffer, int windowWidth, int windowHeight)
{
    if (windowWidth == 0 || windowHeight == 0 || _pipeline == nullptr)
    {
        return;
    }

    // パイプラインをバインド
    SDL_BindGPUGraphicsPipeline(renderPass, _pipeline);

    // テクスチャとサンプラーをバインド（シェーダーが2サンプラー宣言しているため2スロットバインド）
    SDL_GPUTextureSamplerBinding textureSamplerBindings[2] = {};
    textureSamplerBindings[0].texture = _texture;
    textureSamplerBindings[0].sampler = _sampler;
    // s_texture1 は未使用だが宣言があるため同じテクスチャで埋める
    textureSamplerBindings[1].texture = _texture;
    textureSamplerBindings[1].sampler = _sampler;
    SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplerBindings, 2);

    // ユニフォームデータをプッシュ
    SpriteUBO ubo;
    ubo.spriteColor[0] = _spriteColor[0];
    ubo.spriteColor[1] = _spriteColor[1];
    ubo.spriteColor[2] = _spriteColor[2];
    ubo.spriteColor[3] = _spriteColor[3];
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &ubo, sizeof(SpriteUBO));

    // 頂点バッファをバインド
    SDL_GPUBufferBinding vertexBinding = {};
    vertexBinding.buffer = _vertexBuffer.GetBuffer();
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    // インデックスバッファをバインド
    SDL_GPUBufferBinding indexBinding = {};
    indexBinding.buffer = _indexBuffer.GetBuffer();
    indexBinding.offset = 0;
    SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // 描画
    SDL_DrawGPUIndexedPrimitives(renderPass, IndexNum, 1, 0, 0, 0);
}

void LAppSprite::UploadVertexData(SDL_GPUCopyPass* copyPass, int windowWidth, int windowHeight)
{
    if (windowWidth == 0 || windowHeight == 0)
    {
        return;
    }

    // 頂点データ更新
    SpriteVertex vertices[VertexNum] = {
        { (_rect.left  - windowWidth * 0.5f) / (windowWidth * 0.5f), (windowHeight * 0.5f - _rect.up) / (windowHeight * 0.5f), 0.0f, 0.0f },
        { (_rect.right - windowWidth * 0.5f) / (windowWidth * 0.5f), (windowHeight * 0.5f - _rect.up) / (windowHeight * 0.5f), 1.0f, 0.0f },
        { (_rect.left  - windowWidth * 0.5f) / (windowWidth * 0.5f), (windowHeight * 0.5f - _rect.down) / (windowHeight * 0.5f), 0.0f, 1.0f },
        { (_rect.right - windowWidth * 0.5f) / (windowWidth * 0.5f), (windowHeight * 0.5f - _rect.down) / (windowHeight * 0.5f), 1.0f, 1.0f }
    };

    // 頂点バッファにデータをアップロード (transfer bufferを再利用)
    csmUint32 bufferSize = sizeof(SpriteVertex) * VertexNum;
    if (_vertexTransferBuffer.GetTransferBuffer() == nullptr)
    {
        _vertexTransferBuffer.CreateTransferBuffer(_gpuDevice, bufferSize, SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD);
    }
    void* mapped = _vertexTransferBuffer.MapTransferBuffer(_gpuDevice, true);
    if (mapped)
    {
        memcpy(mapped, vertices, bufferSize);
        _vertexTransferBuffer.UnmapTransferBuffer(_gpuDevice);

        _vertexBuffer.UploadToBuffer(copyPass, &_vertexTransferBuffer, bufferSize);
    }
}

bool LAppSprite::IsHit(int windowWidth, int windowHeight, float pointX, float pointY) const
{
    if (windowWidth == 0 || windowHeight == 0)
    {
        return false;
    }

    float y = pointY;

    return (pointX >= _rect.left && pointX <= _rect.right && y >= _rect.up && y <= _rect.down);
}

void LAppSprite::SetPipeline(SDL_GPUGraphicsPipeline* pipeline)
{
    _pipeline = pipeline;
}

void LAppSprite::UpdateTexture(SDL_GPUTexture* texture, SDL_GPUSampler* sampler)
{
    _texture = texture;
    _sampler = sampler;
}
#endif

void LAppSprite::SetColor(float r, float g, float b, float a)
{
    _spriteColor[0] = r;
    _spriteColor[1] = g;
    _spriteColor[2] = b;
    _spriteColor[3] = a;
}

void LAppSprite::ResetRect(float x, float y, float width, float height)
{
    _rect.left = (x - width * 0.5f);
    _rect.right = (x + width * 0.5f);
#if defined(CSM_TARGET_OPENGL)
    _rect.up = (y + height * 0.5f);
    _rect.down = (y - height * 0.5f);
#elif defined(CSM_TARGET_VULKAN) || defined(CSM_TARGET_GPU)
    _rect.up = (y - height * 0.5f);
    _rect.down = (y + height * 0.5f);
#endif
}

#if defined(CSM_TARGET_OPENGL)
void LAppSprite::SetWindowSize(int width, int height)
{
    _maxWidth = width;
    _maxHeight = height;
}
#endif
