# Cubism Native SDL3 Sample

SDL3ベースの統合サンプルです。OpenGLとVulkanの両方のレンダリングバックエンドをサポートしています。

## 必要条件

- CMake 3.16以上
- C++17対応コンパイラ
- OpenGLの場合: OpenGL 3.3以上
- Vulkanの場合: Vulkan SDK

## ビルド方法

### OpenGLバックエンド (デフォルト)

トップフォルダで実行してください

```bash
make prebuild
make build
```

### Vulkanバックエンド

```bash
CMAKEOPT="-DUSE_OPENGL=OFF -DUSE_VULKAN=ON -DUSE_GPU=OFF" make prebuild
make build
```

### SDL3_GPUバックエンド

```bash
CMAKEOPT="-DUSE_OPENGL=OFF -DUSE_VULKAN=OFF -DUSE_GPU=ON" make prebuild
make build
```

## 実行方法

ビルドディレクトリ `build/x64-windows/bin/$(BUILD_TYPE)` に降りて `SDL3Demo.exe` を実行するか、`make run` を実行。

## 設定オプション

| オプション | デフォルト | 説明 |
|------------|-----------|------|
| USE_OPENGL | ON | OpenGLバックエンドを使用 |
| USE_VULKAN | OFF | Vulkanバックエンドを使用 |
| USE_GPU | OFF | SDL3_GPUバックエンドを使用 |

### SDL3_GPU使用時のサブオプション

SDL3_GPUのバックエンドのうちどれを有効にしてビルドするかをスイッチ。
実質的な処理としてはシェーダーバイナリとして何を受け入れるかのスイッチとなり、
SDL3_GPUの内部処理によりD3D12(DXIL)→VULKAN(SPIR-V)→METAL(MSL)の順番にフォールバックするので、
デフォルトのすべてを有効にした状態ではD3D12が使用される(SDL v3.4.0時点での内部処理)。

USE_GPU_BACKEND_PRIVATEはNDA環境で有効にしてください。
サンプル環境では、自動的にほか3種のバックエンドは無効になるようなcmakeにしてあります。

| オプション | デフォルト | 説明 |
|------------|-----------|------|
| USE_GPU_BACKEND_VULKAN | ON | SDL3_GPUのVulkanバックエンドを使用 |
| USE_GPU_BACKEND_D3D12 | ON | SDL3_GPUのD3D12バックエンドを使用 |
| USE_GPU_BACKEND_METALGPU | ON | SDL3_GPUのMetalバックエンドを使用 |
| USE_GPU_BACKEND_PRIVATE | OFF | SDL3_GPUのNDAバックエンドを使用 |

## SDL3_GPUでのPRIVATEシェーダ(NDA環境での専用シェーダ)を使用する方法

**※サンプルではNDA環境での実行確認はできておらず、とりあえず仕込みだけしてあるだけの状態なので注意。**

### サンプルでの使用方法

  - cmakeオプション `USE_GPU_BACKEND_PRIVATE` を ON にする
  - cmake変数 `CSM_PRIVATE_SHADER_SUBDIR` にシェーダを配置するサブディレクトリ名を指定(例: priv/)
    - (PRIVATEシェーダを使用する場合は他シェーダはビルドしないと思うので空文字列とかでも問題は無いが、一応用意してある)
  - cmake変数 `CSM_PRIVATE_SHADER_EXT` にシェーダ拡張子を指定(例: .sb)
  - cmake変数 `CSM_PRIVATE_SHADER_ENTRY` にシェーダーのエントリポイント名を指定(例: main)
  - NDA環境用のシェーダーコードを用意する
    - Framework(レンダラー)用のコードも必要なので注意。NDA環境用はshadercrossでのクロスビルドも及ばないのですべて自前で用意＆ビルドが必要
  - シェーダービルド用のcmakeファイルを任意のファイル名で用意してcmake変数 `CSM_PRIVATE_SHADER_CMAKE` にセットするか `Samples/SDL3/PrivateShaders.cmake` に配置する
    - 前述の通りSample部分だけでなくFramework(レンダラー)用のシェーダービルドも必要なので注意

### Framework(レンダラー)側でのPRIVATEシェーダ対応の内容

シェーダを配置するサブディレクトリ名とシェーダ拡張子とシェーダエントリポイント名をC++のdefine `CSM_SDL3_GPU_PRIVATE_SHADER_SUBDIR` と `CSM_SDL3_GPU_PRIVATE_SHADER_EXTENSION` と `CSM_SDL3_GPU_PRIVATE_SHADER_ENTRYPOINT` として与える。これをサンプルでは cmake変数 `CSM_PRIVATE_SHADER_EXT`  と `CSM_PRIVATE_SHADER_ENTRY` を使って以下のように設定しています。

```c++
    target_compile_definitions(Framework PUBLIC
        CSM_SDL3_GPU_PRIVATE_SHADER_SUBDIR=${CSM_PRIVATE_SHADER_SUBDIR}
        CSM_SDL3_GPU_PRIVATE_SHADER_EXTENSION=${CSM_PRIVATE_SHADER_EXT}
        CSM_SDL3_GPU_PRIVATE_SHADER_ENTRYPOINT=${CSM_PRIVATE_SHADER_ENTRY}
    )
```

### Framework(レンダラ)のPRIVATEシェーダビルド

NDA環境用のシェーダコードは(当然ながら)用意されていません。Framework/src/Rendering/SDL3_GPU/Shaders/ のシェーダーコードを各NDA環境用のシェーダーコードに書き換えたうえで、それをビルドするためのCMakeLists.txtを用意してください。

サンプルではPRIVATEシェーダービルド用のcmakeファイルは任意のファイル名で用意してcmake変数 `CSM_PRIVATE_SHADER_CMAKE` にセットするか `Samples/SDL3/PrivateShaders.cmake` に配置すると自動的に `include()` されるように仕込んであります。

このcmakeファイルでは既存のシェーダーフォーマット用のビルド記述を参考にして、最終的にバイナリ出力ディレクトリの FrameworkShaders/ の下に成果物が配置されるようにしておいてください。

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
