/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <string>
#include <Type/csmVector.hpp>

#if defined(CSM_TARGET_VULKAN)
#include <vulkan/vulkan.h>
#include <Rendering/Vulkan/CubismClass_Vulkan.hpp>
#endif

/**
* @brief テクスチャの管理を行うクラス
*
* 画像読み込み、管理を行うクラス。
*/
class LAppTextureManager
{
public:
    /**
    * @brief 画像情報構造体
    */
    struct TextureInfo
    {
        unsigned int id;        ///< テクスチャID
        int width;              ///< 横幅
        int height;             ///< 高さ
        std::string fileName;   ///< ファイル名
    };

    /**
    * @brief コンストラクタ
    */
    LAppTextureManager();

    /**
    * @brief デストラクタ
    */
    ~LAppTextureManager();

    /**
    * @brief プリマルチプライ処理
    *
    * @param[in] red  画像のRed値
    * @param[in] green  画像のGreen値
    * @param[in] blue  画像のBlue値
    * @param[in] alpha  画像のAlpha値
    *
    * @return プリマルチプライ処理後の色値
    */
    inline unsigned int Premultiply(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
    {
        return static_cast<unsigned>(\
            (red * (alpha + 1) >> 8) | \
            ((green * (alpha + 1) >> 8) << 8) | \
            ((blue * (alpha + 1) >> 8) << 16) | \
            (((alpha)) << 24)   \
            );
    }

    /**
    * @brief 画像読み込み
    *
    * @param[in] fileName 読み込む画像ファイルパス名
    * @return 画像情報。失敗時はNULLを返す
    */
#if defined(CSM_TARGET_OPENGL)
    TextureInfo* CreateTextureFromPngFile(std::string fileName);
#elif defined(CSM_TARGET_VULKAN)
    TextureInfo* CreateTextureFromPngFile(
        std::string fileName, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags imageProperties, Csm::csmFloat32 anisotropy
    );
#endif

    /**
    * @brief 画像の解放
    *
    * 配列に存在するすべての画像を解放する
    */
    void ReleaseTextures();

    /**
    * @brief 画像の解放
    *
    * 指定したテクスチャIDの画像を解放する
    * @param[in] textureId 解放するテクスチャID
    */
    void ReleaseTexture(unsigned int textureId);

    /**
    * @brief 画像の解放
    *
    * 指定した名前の画像を解放する
    * @param[in] fileName 解放する画像ファイルパス名
    */
    void ReleaseTexture(std::string fileName);

    /**
    * @brief テクスチャIDからテクスチャ情報を得る
    *
    * @param[in] textureId  取得したいテクスチャID
    * @return テクスチャが存在していればTextureInfoが返る
    */
    TextureInfo* GetTextureInfoById(unsigned int textureId) const;

#if defined(CSM_TARGET_VULKAN)
    /**
    * @brief テクスチャIDからCubismImageVulkanを得る
    *
    * @param[in] textureId  取得したいテクスチャID
    * @param[out] retTexture  取得したテクスチャを格納する変数
    * @return テクスチャが存在していればtrue
    */
    bool GetTexture(Csm::csmUint32 textureId, Live2D::Cubism::Framework::CubismImageVulkan& retTexture) const;

    /**
    * @brief バッファをイメージにコピーする
    *
    * @param[in] commandBuffer  コマンドバッファ
    * @param[in] buffer         バッファ
    * @param[in] image          イメージ
    * @param[in] width          画像の横幅
    * @param[in] height         画像の縦幅
    */
    void CopyBufferToImage(VkCommandBuffer commandBuffer, const VkBuffer& buffer, VkImage image, uint32_t width, uint32_t height);

    /**
    * @brief ミップマップを作成する
    *
    * @param[in] image          イメージ
    * @param[in] texWidth       画像の幅
    * @param[in] texHeight      画像の高さ
    * @param[in] mipLevels      ミップレベル
    */
    void GenerateMipmaps(Live2D::Cubism::Framework::CubismImageVulkan image, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);
#endif

private:
#if defined(CSM_TARGET_OPENGL)
    Csm::csmVector<TextureInfo*> _textures;
#elif defined(CSM_TARGET_VULKAN)
    Csm::csmVector<Live2D::Cubism::Framework::CubismImageVulkan> _textures;  ///< テクスチャの実体
    Csm::csmVector<TextureInfo*> _texturesInfo;  ///< テクスチャ情報
    Csm::csmUint32 _sequenceId = 0;  ///< イメージのインデックス
    uint32_t _mipLevels = 0;  ///< ミップレベル
#endif
};
