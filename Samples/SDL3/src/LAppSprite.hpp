/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#if defined(CSM_TARGET_OPENGL)
#include <GL/glew.h>
#elif defined(CSM_TARGET_VULKAN)
#include <Rendering/Vulkan/CubismClass_Vulkan.hpp>
#include "LAppSprite_Common.hpp"
#endif

// 前方宣言
#if defined(CSM_TARGET_VULKAN)
class VulkanManager;
class LAppSpritePipeline;
#endif

/**
* @brief スプライトを実装するクラス
*
* テクスチャID、Rect管理
*/
class LAppSprite
#if defined(CSM_TARGET_VULKAN)
    : public LAppSprite_Common
#endif
{
public:
    /**
    * @brief Rect 構造体
    */
    struct Rect
    {
    public:
        float left;
        float right;
        float up;
        float down;
    };

#if defined(CSM_TARGET_VULKAN)
    /**
     * @brief   頂点情報を保持する構造体
     */
    struct SpriteVertex
    {
        float x, y; // Position
        float u, v; // UVs

        /**
         * @brief   頂点入力のバインド設定を指定する
         */
        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(SpriteVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        /**
         * @brief   頂点入力の構造を指定する
         */
        static void GetAttributeDescriptions(VkVertexInputAttributeDescription attributeDescriptions[2])
        {
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(SpriteVertex, x);
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(SpriteVertex, u);
        }
    };

    /**
     * @brief   スプライト用ユニフォームバッファオブジェクトの中身を保持する構造体
     */
    struct SpriteUBO
    {
        float spriteColor[4];
    };
#endif

#if defined(CSM_TARGET_OPENGL)
    /**
    * @brief コンストラクタ
    *
    * @param[in]       x            x座標
    * @param[in]       y            y座標
    * @param[in]       width        横幅
    * @param[in]       height       高さ
    * @param[in]       textureId    テクスチャID
    * @param[in]       programId    シェーダID
    */
    LAppSprite(float x, float y, float width, float height, GLuint textureId, GLuint programId);

    /**
    * @brief デストラクタ
    */
    ~LAppSprite();

    /**
    * @brief 描画する
    */
    void Render() const;

    /**
    * @brief テクスチャIDを指定して描画する
    */
    void RenderImmidiate(GLuint textureId, const GLfloat uvVertex[8]) const;
#elif defined(CSM_TARGET_VULKAN)
    /**
    * @brief コンストラクタ
    *
    * @param[in]       device                  デバイス
    * @param[in]       physicalDevice          物理デバイス
    * @param[in]       vkManager               Vulkanリソースマネージャ
    * @param[in]       x                       x座標
    * @param[in]       y                       y座標
    * @param[in]       width                   横幅
    * @param[in]       height                  高さ
    * @param[in]       textureId               テクスチャID
    * @param[in]       pipeline                パイプライン
    * @param[in]       view                    テクスチャビュー
    * @param[in]       sampler                 テクスチャサンプラー
    */
    LAppSprite(
        VkDevice device, VkPhysicalDevice physicalDevice, VulkanManager* vkManager,
        float x, float y, float width, float height,
        Csm::csmUint32 textureId, LAppSpritePipeline* pipeline,
        VkImageView view, VkSampler sampler);

    /**
    * @brief デストラクタ
    */
    ~LAppSprite();

    /**
    * @brief リソースを開放する
    *
    * @param[in]       device                  デバイス
    */
    void Release(VkDevice device);

    /**
    * @brief ディスクリプタセットを作成する
    *
    * @param[in]       device                  デバイス
    * @param[in]       descriptorSetLayout     ディスクリプタセットレイアウト
    */
    void CreateDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);

    /**
    * @brief ユニフォームバッファを更新する
    *
    * @param[in]       vkManager           Vulkanリソースマネージャ
    * @param[in]       maxWidth            最大幅
    * @param[in]       maxHeight           最大高さ
    */
    void UpdateData(VulkanManager* vkManager, int maxWidth, int maxHeight) const;

    /**
     * @brief ディスクリプタセットを更新する
     *
     * @param[in]       device                  デバイス
     * @param[in]       view                    テクスチャビュー
     * @param[in]       sampler                 テクスチャサンプラー
     */
    void UpdateDescriptorSet(VkDevice device, VkImageView view, VkSampler sampler);

    /**
     * @brief ディスクリプタセットの更新フラグを設定する
     *
     * @param[in]       frag                  フラグ
     */
    void SetDescriptorUpdated(bool frag);

    /**
    * @brief 描画する
    *
    * @param[in]       commandBuffer          コマンドバッファ
    * @param[in]       vkManager              Vulkanリソースマネージャ
    * @param[in]       windowWidth            ウィンドウ幅
    * @param[in]       windowHeight           ウィンドウ高さ
    */
    void Render(VkCommandBuffer commandBuffer, VulkanManager* vkManager, int windowWidth, int windowHeight);

    /**
    * @brief パイプラインのセット
    *
    * @param[in]       pipeline      パイプラインクラスへのポインタ
    */
    void SetPipeline(LAppSpritePipeline* pipeline);
#endif

    /**
    * @brief 当たり判定
    *
    * @param[in]       pointX    x座標
    * @param[in]       pointY    y座標
    */
#if defined(CSM_TARGET_OPENGL)
    bool IsHit(float pointX, float pointY) const;
#elif defined(CSM_TARGET_VULKAN)
    bool IsHit(int windowWidth, int windowHeight, float pointX, float pointY) const;
#endif


    /**
     * @brief 色設定
     *
     * @param[in]       r (0.0~1.0)
     * @param[in]       g (0.0~1.0)
     * @param[in]       b (0.0~1.0)
     * @param[in]       a (0.0~1.0)
     */
    void SetColor(float r, float g, float b, float a);

    /**
     * @brief サイズ再設定
     *
     * @param[in]       x            x座標
     * @param[in]       y            y座標
     * @param[in]       width        横幅
     * @param[in]       height       高さ
     */
    void ResetRect(float x, float y, float width, float height);


#if defined(CSM_TARGET_OPENGL)
    /**
     * @brief ウインドウサイズ設定
     *
     * @param[in]       width        横幅
     * @param[in]       height       高さ
     */
    void SetWindowSize(int width, int height);

    /**
    * @brief Getter テクスチャID
    * @return テクスチャID
    */
    unsigned int GetTextureId() const { return _textureId; }
#endif

private:
    Rect _rect;             ///< 矩形
    float _spriteColor[4];  ///< 表示カラー

#if defined(CSM_TARGET_OPENGL)
    int _maxWidth;          ///< ウインドウ幅
    int _maxHeight;         ///< ウインドウ高さ
    GLuint _textureId;      ///< テクスチャID
    int _positionLocation;  ///< 位置アトリビュート
    int _uvLocation;        ///< UVアトリビュート
    int _textureLocation;   ///< テクスチャアトリビュート
    int _colorLocation;     ///< カラーアトリビュート
#elif defined(CSM_TARGET_VULKAN)
    static const uint16_t VertexNum = 4;
    static const uint16_t IndexNum = 6;
    Live2D::Cubism::Framework::CubismBufferVulkan _vertexBuffer; ///< 頂点バッファ
    Live2D::Cubism::Framework::CubismBufferVulkan _stagingBuffer; ///< ステージングバッファ
    Live2D::Cubism::Framework::CubismBufferVulkan _indexBuffer; ///< インデックスバッファ
    Live2D::Cubism::Framework::CubismBufferVulkan _uniformBuffer; ///< ユニフォームバッファ
    LAppSpritePipeline* _pipeline; ///< パイプライン
    VkDescriptorPool _descriptorPool; ///< ディスクリプタプール
    VkDescriptorSet _descriptorSet; ///< ディスクリプタセット
    bool isDescriptorUpdated;
#endif
};
