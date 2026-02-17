/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "CubismFramework.hpp"
#include "LAppView_Common.hpp"

#if defined(CSM_TARGET_OPENGL)
#include <Rendering/OpenGL/CubismRenderTarget_OpenGLES2.hpp>
#elif defined(CSM_TARGET_VULKAN)
#include <Rendering/Vulkan/CubismRenderTarget_Vulkan.hpp>
#include <Rendering/Vulkan/CubismRenderer_Vulkan.hpp>
#include <vulkan/vulkan.h>
#endif

class TouchManager_Common;
class LAppSprite;
class LAppModel;
#if defined(CSM_TARGET_OPENGL)
class LAppSpriteShader;
#elif defined(CSM_TARGET_VULKAN)
class LAppSpritePipeline;
class LAppModelSpritePipeline;
#endif

/**
* @brief 描画クラス
*/
class LAppView : public LAppView_Common
{
public:
    /**
     * @brief LAppModelのレンダリング先
     */
    enum SelectTarget
    {
        SelectTarget_None,                ///< デフォルトのフレームバッファにレンダリング
        SelectTarget_ModelFrameBuffer,    ///< LAppModelが各自持つフレームバッファにレンダリング
        SelectTarget_ViewFrameBuffer,     ///< LAppViewの持つフレームバッファにレンダリング
    };

    /**
    * @brief コンストラクタ
    */
    LAppView();

    /**
    * @brief デストラクタ
    */
    ~LAppView();

    /**
    * @brief 初期化する。
    */
    virtual void Initialize(int width, int height) override;

    /**
    * @brief 描画する。
    */
    void Render();

    /**
    * @brief 画像の初期化を行う。
    */
    void InitializeSprite();

#if defined(CSM_TARGET_OPENGL)
    /**
    * @brief スプライト系のサイズ再設定
    */
    void ResizeSprite();
#elif defined(CSM_TARGET_VULKAN)
    /**
    * @brief レンダリングを開始する。
    */
    void BeginRendering(VkCommandBuffer commandBuffer, float r, float g, float b, float a, bool isClear);

    /**
    * @brief レンダリングを終了する際のレイアウトを変更する。
    */
    void ChangeEndLayout(VkCommandBuffer commandBuffer);

    /**
    * @brief レンダリングを終了する。
    */
    void EndRendering(VkCommandBuffer commandBuffer);

    /**
    * @brief ウィンドウサイズ変更の際にスプライトを再作成する
    */
    void ResizeSprite(int width, int height);

    /**
    * @brief オフスクリーンの破棄
    */
    void DestroyRenderTarget();
#endif

    /**
    * @brief タッチされたときに呼ばれる。
    *
    * @param[in] pointX スクリーンX座標
    * @param[in] pointY スクリーンY座標
    */
    void OnTouchesBegan(float pointX, float pointY) const;

    /**
    * @brief タッチしているときにポインタが動いたら呼ばれる。
    *
    * @param[in] pointX スクリーンX座標
    * @param[in] pointY スクリーンY座標
    */
    void OnTouchesMoved(float pointX, float pointY) const;

    /**
    * @brief タッチが終了したら呼ばれる。
    *
    * @param[in] pointX スクリーンX座標
    * @param[in] pointY スクリーンY座標
    */
    void OnTouchesEnded(float pointX, float pointY) const;

    /**
     * @brief モデル1体を描画する直前にコールされる
     */
    void PreModelDraw(LAppModel& refModel);

    /**
     * @brief モデル1体を描画した直後にコールされる
     */
#if defined(CSM_TARGET_OPENGL)
    void PostModelDraw(LAppModel& refModel);
#elif defined(CSM_TARGET_VULKAN)
    void PostModelDraw(LAppModel& refModel, Csm::csmInt32 modelIndex);
#endif

    /**
     * @brief 別レンダリングターゲットにモデルを描画するサンプルで描画時のαを決定する
     */
    float GetSpriteAlpha(int assign) const;

    /**
     * @brief レンダリング先を切り替える
     */
    void SwitchRenderingTarget(SelectTarget targetType);

    /**
     * @brief レンダリング先をデフォルト以外に切り替えた際の背景クリア色設定
     */
    void SetRenderTargetClearColor(float r, float g, float b);

private:
    TouchManager_Common* _touchManager;           ///< タッチマネージャー
    LAppSprite* _back;                            ///< 背景画像
    LAppSprite* _gear;                            ///< ギア画像
    LAppSprite* _power;                           ///< 電源画像
    LAppSprite* _renderSprite;                    ///< レンダリング先を別ターゲットにする方式の場合に使用

#if defined(CSM_TARGET_OPENGL)
    Csm::Rendering::CubismRenderTarget_OpenGLES2 _renderBuffer;
    LAppSpriteShader* _spriteShader;              ///< シェーダー作成委譲クラス
#elif defined(CSM_TARGET_VULKAN)
    Csm::Rendering::CubismRenderTarget_Vulkan _renderBuffer;
    LAppSpritePipeline* _spritePipeline;          ///< スプライト用パイプライン
    LAppModelSpritePipeline* _modelSpritePipeline; ///< モデルスプライト用パイプライン
#endif

    SelectTarget _renderTarget;                   ///< レンダリング先の選択肢
    float _clearColor[4];                         ///< レンダリングターゲットのクリアカラー
};
