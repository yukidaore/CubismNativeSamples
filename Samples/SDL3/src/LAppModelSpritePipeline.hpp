/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "LAppSpritePipeline.hpp"

/**
* @brief スプライト用のシェーダー設定を保持するクラス（モデル用）
 */
class LAppModelSpritePipeline : public LAppSpritePipeline
{
public:
    /**
     * @brief   コンストラクタ
     *
     * @param[in]       device            ->  論理デバイス
     * @param[in]       extent            ->  スワップチェーンの各画像の解像度
     * @param[in]       swapchainFormat   ->  スワップチェーンの画像フォーマット
     */
    LAppModelSpritePipeline(VkDevice device, VkExtent2D extent, VkFormat swapchainFormat);
};
