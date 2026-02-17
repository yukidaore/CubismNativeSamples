# Cubism Native SDL3 Sample

SDL3ベースの統合サンプルです。OpenGLとVulkanの両方のレンダリングバックエンドをサポートしています。

## 必要条件

- CMake 3.16以上
- C++17対応コンパイラ
- OpenGLの場合: OpenGL 3.3以上
- Vulkanの場合: Vulkan SDK

## ビルド方法

### OpenGLバックエンド (デフォルト)

```bash
cd Samples/SDL3
mkdir build && cd build
cmake ..
cmake --build .
```

### Vulkanバックエンド

```bash
cd Samples/SDL3
mkdir build && cd build
cmake .. -DUSE_OPENGL=OFF -DUSE_VULKAN=ON
cmake --build .
```

## 設定オプション

| オプション | デフォルト | 説明 |
|------------|-----------|------|
| USE_OPENGL | ON | OpenGLバックエンドを使用 |
| USE_VULKAN | OFF | Vulkanバックエンドを使用 |

## ディレクトリ構成

```
SDL3/
├── CMakeLists.txt      # ビルド設定
├── README.md           # このファイル
└── src/
    ├── main.cpp            # エントリーポイント
    ├── LAppDefine.*        # 定数定義
    ├── LAppPal.*           # プラットフォーム抽象化
    ├── LAppDelegate.*      # アプリケーションメインクラス
    ├── LAppView.*          # 描画ビュー
    ├── LAppLive2DManager.* # モデル管理
    ├── LAppModel.*         # モデルクラス
    ├── LAppTextureManager.* # テクスチャ管理
    ├── LAppSprite.*        # スプライト描画
    ├── VulkanManager.*     # Vulkan管理 (Vulkanのみ)
    └── SwapchainManager.*  # スワップチェーン管理 (Vulkanのみ)
```

## 依存ライブラリ

- SDL3: ウィンドウ管理・入力処理 (FetchContentで自動取得)
- GLEW: OpenGL拡張ロード (OpenGLの場合、OpenGL/thirdPartyから参照)
- stb_image: 画像読み込み (OpenGL/thirdPartyから参照)

## 操作方法

- マウス左クリック: モデルの視線追従
- ドラッグ: モデルの移動
- マウスホイール: ズーム
- ESC: 終了

## 注意事項

- OpenGLとVulkanを同時に有効にすることはできません
- Vulkanバックエンドを使用する場合は、Vulkan SDKがインストールされている必要があります
- Windows以外のプラットフォームでのビルドは追加の設定が必要な場合があります
