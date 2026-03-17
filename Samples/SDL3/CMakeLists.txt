# SDL3 Sample CMakeLists.txt
# This is called as a subdirectory from the root CMakeLists.txt

set(SDL3_SAMPLE_NAME SDL3Demo)

# Set directory paths
set(RES_PATH ${SDK_ROOT_PATH}/Samples/Resources)
set(COMMON_SRC_PATH ${SDK_ROOT_PATH}/Samples/Common)

# --------------------------------------------------------------------
# SDL3
# --------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.0
    GIT_SHALLOW TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# Source files
set(SDL3_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppDefine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppDefine.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppPal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppPal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppDelegate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppDelegate.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppView.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppLive2DManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppLive2DManager.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppModel.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppTextureManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppTextureManager.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppSprite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppSprite.hpp
)

# OpenGL specific source files
if(USE_OPENGL)
    list(APPEND SDL3_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppSpriteShader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppSpriteShader.hpp
    )
    find_package(OpenGL REQUIRED)
endif()

# Vulkan specific source files
if(USE_VULKAN)
    list(APPEND SDL3_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VulkanManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VulkanManager.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SwapchainManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SwapchainManager.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppSpritePipeline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppSpritePipeline.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppModelSpritePipeline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LAppModelSpritePipeline.hpp
    )
endif()

# Common source files
set(SDL3_COMMON_SOURCE_FILES
    ${COMMON_SRC_PATH}/LAppAllocator_Common.cpp
    ${COMMON_SRC_PATH}/LAppAllocator_Common.hpp
    ${COMMON_SRC_PATH}/LAppView_Common.cpp
    ${COMMON_SRC_PATH}/LAppView_Common.hpp
    ${COMMON_SRC_PATH}/LAppModel_Common.cpp
    ${COMMON_SRC_PATH}/LAppModel_Common.hpp
    ${COMMON_SRC_PATH}/LAppSprite_Common.cpp
    ${COMMON_SRC_PATH}/LAppSprite_Common.hpp
    ${COMMON_SRC_PATH}/LAppTextureManager_Common.cpp
    ${COMMON_SRC_PATH}/LAppTextureManager_Common.hpp
    ${COMMON_SRC_PATH}/LAppWavFileHandler_Common.cpp
    ${COMMON_SRC_PATH}/LAppWavFileHandler_Common.hpp
    ${COMMON_SRC_PATH}/TouchManager_Common.cpp
    ${COMMON_SRC_PATH}/TouchManager_Common.hpp
    ${COMMON_SRC_PATH}/MouseActionManager_Common.cpp
    ${COMMON_SRC_PATH}/MouseActionManager_Common.hpp
    ${COMMON_SRC_PATH}/CubismSampleViewMatrix_Common.cpp
    ${COMMON_SRC_PATH}/CubismSampleViewMatrix_Common.hpp
)

# Make executable app.
add_executable(${SDL3_SAMPLE_NAME}
    ${SDL3_SOURCE_FILES}
    ${SDL3_COMMON_SOURCE_FILES}
)

# Link libraries to app.
target_link_libraries(${SDL3_SAMPLE_NAME} PRIVATE
    Framework
    SDL3::SDL3-static
)

if(USE_OPENGL)
    target_compile_definitions(${SDL3_SAMPLE_NAME} PRIVATE CSM_TARGET_OPENGL)
elseif(USE_VULKAN)
    target_compile_definitions(${SDL3_SAMPLE_NAME} PRIVATE CSM_TARGET_VULKAN)
elseif(USE_GPU)
    target_compile_definitions(${SDL3_SAMPLE_NAME} PRIVATE CSM_TARGET_GPU)
    # GPU バックエンド選択マクロ
    if(USE_GPU_BACKEND_VULKAN)
        target_compile_definitions(${SDL3_SAMPLE_NAME} PRIVATE CSM_TARGET_GPU_VULKAN)
    endif()
    if(USE_GPU_BACKEND_D3D12)
        target_compile_definitions(${SDL3_SAMPLE_NAME} PRIVATE CSM_TARGET_GPU_D3D12)
    endif()
    if(USE_GPU_BACKEND_METAL)
        target_compile_definitions(${SDL3_SAMPLE_NAME} PRIVATE CSM_TARGET_GPU_METAL)
    endif()
endif()

# MSVC specific settings - use static runtime
if(MSVC)
    # Add Windows system libraries
    target_link_libraries(${SDL3_SAMPLE_NAME} PRIVATE
        winmm.lib
        setupapi.lib
        version.lib
        imm32.lib
    )
endif()

# Specify include directories.
target_include_directories(${SDL3_SAMPLE_NAME} PRIVATE
    ${STB_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${COMMON_SRC_PATH}
    ${CORE_PATH}/include
)

# Build in multi-process.
if(MSVC)
    target_compile_options(${SDL3_SAMPLE_NAME} PRIVATE /MP /utf-8)
    target_compile_definitions(${SDL3_SAMPLE_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Copy resource directory to build directory.
add_custom_command(
  TARGET ${SDL3_SAMPLE_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_PATH} $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/Resources
)

# Copy shaders
if(USE_OPENGL)
    set(SAMPLE_SHADER_PATH ${SDK_ROOT_PATH}/Samples/OpenGL/Shaders/Standard)
    set(FRAMEWORK_SHADER_PATH ${FRAMEWORK_PATH}/src/Rendering/OpenGL/Shaders/Standard)
    add_custom_command(
      TARGET ${SDL3_SAMPLE_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${SAMPLE_SHADER_PATH} $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/SampleShaders
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${FRAMEWORK_SHADER_PATH} $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/FrameworkShaders
    )
endif()

if(USE_VULKAN)
    set(SAMPLE_SHADER_PATH ${SDK_ROOT_PATH}/Samples/Vulkan/Shaders)
    set(FRAMEWORK_SHADER_PATH ${FRAMEWORK_PATH}/src/Rendering/Vulkan/Shaders)
    add_subdirectory(${SAMPLE_SHADER_PATH} ${CMAKE_CURRENT_BINARY_DIR}/SampleShaders)
    add_subdirectory(${FRAMEWORK_SHADER_PATH} ${CMAKE_CURRENT_BINARY_DIR}/FrameworkShaders)
    add_dependencies(Framework SampleShaders)
    add_dependencies(Framework FrameworkShaders)

    # copy compiled shader binaries
    add_custom_command(
      TARGET ${SDL3_SAMPLE_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/SampleShaders/compiledShaders $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/SampleShaders
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/FrameworkShaders/compiledShaders $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/FrameworkShaders
    )
endif()

if(USE_GPU)
    set(SAMPLE_SHADER_PATH ${SDK_ROOT_PATH}/Samples/SDL3/Shaders)
    set(FRAMEWORK_SHADER_PATH ${FRAMEWORK_PATH}/src/Rendering/SDL3_GPU/Shaders)

    # --- Resolve shadercross executable ---
    set(SHADERCROSS_FOUND FALSE)
    set(SHADERCROSS_TARGET_DEPENDS "")
    if(DEFINED ENV{SDL_SHADERCROSS_PATH})
        find_program(_SHADERCROSS_EXE shadercross PATHS "$ENV{SDL_SHADERCROSS_PATH}" NO_DEFAULT_PATH)
        if(_SHADERCROSS_EXE)
            set(SHADERCROSS_COMMAND "${_SHADERCROSS_EXE}")
            set(SHADERCROSS_FOUND TRUE)
            message(STATUS "Found shadercross: ${SHADERCROSS_COMMAND}")
        endif()
    endif()

    if(NOT SHADERCROSS_FOUND)
        # build own shadercross
        message(STATUS "shadercross not found in SDL_SHADERCROSS_PATH, building from source...")
        FetchContent_Declare(
            sdl_shadercross
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_shadercross.git
            GIT_TAG main
            GIT_SHALLOW TRUE
        )

        # shadercross CLIをstaticビルド
        set(SDLSHADERCROSS_STATIC ON CACHE BOOL "" FORCE)
        set(SDLSHADERCROSS_SHARED OFF CACHE BOOL "" FORCE)
        set(SDLSHADERCROSS_CLI ON CACHE BOOL "" FORCE)
        set(SDLSHADERCROSS_VENDORED ON CACHE BOOL "" FORCE)
        set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL3_DIR "${sdl3_BINARY_DIR}" CACHE PATH "" FORCE)

        # shadercrossのexternalのビルドでパスがかなり長くなり、ソースツリーを
        # 置いたパスによっては"Filename too long" エラーが起きる可能性がある。
        # 回避策としてGitに core.longpaths=true を強制する環境変数を一時設定 (Git 2.31+)
        if(WIN32)
            set(ENV{GIT_CONFIG_COUNT} "1")
            set(ENV{GIT_CONFIG_KEY_0} "core.longpaths")
            set(ENV{GIT_CONFIG_VALUE_0} "true")
            # オブジェクトファイルのパス長が250文字(CMakeデフォルト制限)を超える場合の警告を抑制
            set(CMAKE_OBJECT_PATH_MAX 500)
        endif()

        # 一時的にshadercross関連ビルドを完全にstaticになるようにさしかえ
        set(_prev_build_shared_libs ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(sdl_shadercross)
        set(BUILD_SHARED_LIBS ${_prev_build_shared_libs})

        set(SHADERCROSS_COMMAND "$<TARGET_FILE:shadercross>")
        set(SHADERCROSS_TARGET_DEPENDS "shadercross")
        set(SHADERCROSS_FOUND TRUE)
        message(STATUS "Will build shadercross from source")

        # DirectXShaderCompiler は static ビルドができないため常に共有ライブラリになる。
        # shadercross の実行時に dxcompiler.dll / dxil.dll が必要なので、
        # shadercross のビルド完了後にその実行ファイルと同じディレクトリへコピーする。
        # パスは FetchContent が設定する sdl_shadercross_BINARY_DIR と
        # ビルド構成を表す $<CONFIG> ジェネレータ式で組み立てる。
        #
        # NOTE: add_custom_command(TARGET ...) は対象ターゲットが作成された
        #       ディレクトリ内でしか使えないため、shadercross(FetchContent由来)には使えない。
        #       代わりに add_custom_target + DEPENDS で同等の動作を実現する。
        if(WIN32)
            set(_dxc_bin_dir
                "${sdl_shadercross_BINARY_DIR}/external/DirectXShaderCompiler/$<CONFIG>/bin")
            add_custom_target(shadercross_copy_dlls
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_dxc_bin_dir}/dxcompiler.dll"
                    "${_dxc_bin_dir}/dxil.dll"
                    "$<TARGET_FILE_DIR:shadercross>"
                DEPENDS shadercross
                COMMENT "Copying DirectXShaderCompiler DLLs next to shadercross"
            )
            set(SHADERCROSS_TARGET_DEPENDS "shadercross_copy_dlls")
        endif()
    endif()

    add_subdirectory(${SAMPLE_SHADER_PATH} ${CMAKE_CURRENT_BINARY_DIR}/SDL3SampleShaders)
    add_subdirectory(${FRAMEWORK_SHADER_PATH} ${CMAKE_CURRENT_BINARY_DIR}/SDL3FrameworkShaders)

    # USE_GPU_BACKEND_XXX に応じて必要なシェーダーフォーマットのみビルド・コピー
    if(USE_GPU_BACKEND_VULKAN)
        add_dependencies(Framework FrameworkShaders_SPIRV SampleShaders_SPIRV)
        add_custom_command(TARGET ${SDL3_SAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_BINARY_DIR}/SDL3SampleShaders/compiledShaders/spv
                $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/SampleShaders/spv
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_BINARY_DIR}/SDL3FrameworkShaders/compiledShaders/spv
                $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/FrameworkShaders/spv
        )
    endif()
    if(USE_GPU_BACKEND_D3D12)
        add_dependencies(Framework FrameworkShaders_DXIL SampleShaders_DXIL)
        add_custom_command(TARGET ${SDL3_SAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_BINARY_DIR}/SDL3SampleShaders/compiledShaders/dxil
                $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/SampleShaders/dxil
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_BINARY_DIR}/SDL3FrameworkShaders/compiledShaders/dxil
                $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/FrameworkShaders/dxil
        )
    endif()
    if(USE_GPU_BACKEND_METAL)
        add_dependencies(Framework FrameworkShaders_MSL SampleShaders_MSL)
        add_custom_command(TARGET ${SDL3_SAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_BINARY_DIR}/SDL3SampleShaders/compiledShaders/msl
                $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/SampleShaders/msl
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_BINARY_DIR}/SDL3FrameworkShaders/compiledShaders/msl
                $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/FrameworkShaders/msl
        )
    endif()

    # --- NDA/PRIVATEシェーダービルド用フック ---
    # 利用者は独自のシェーダービルド処理をCMakeファイルとして差し込める。
    # フック内では以下の変数が利用可能:
    #   FRAMEWORK_SHADER_PATH  - Frameworkシェーダーソースディレクトリ
    #   SAMPLE_SHADER_PATH     - Sampleシェーダーソースディレクトリ
    #   SDL3_SAMPLE_NAME       - サンプルアプリのターゲット名
    #   SDK_ROOT_PATH          - SDKルートパス
    #
    # 優先順位:
    #   1. CSM_PRIVATE_SHADER_CMAKE 変数で指定されたファイル
    #   2. ${CMAKE_CURRENT_SOURCE_DIR}/PrivateShaders.cmake (Convention)
    if(DEFINED CSM_PRIVATE_SHADER_CMAKE)
        if(EXISTS "${CSM_PRIVATE_SHADER_CMAKE}")
            message(STATUS "Including private shader build: ${CSM_PRIVATE_SHADER_CMAKE}")
            include("${CSM_PRIVATE_SHADER_CMAKE}")
        else()
            message(WARNING "CSM_PRIVATE_SHADER_CMAKE is set but file not found: ${CSM_PRIVATE_SHADER_CMAKE}")
        endif()
    else()
        set(_private_shader_hook "${CMAKE_CURRENT_SOURCE_DIR}/PrivateShaders.cmake")
        if(EXISTS "${_private_shader_hook}")
            message(STATUS "Including private shader build (convention): ${_private_shader_hook}")
            include("${_private_shader_hook}")
        endif()
    endif()
endif()

# Set project properties.
if(MSVC)
    set_target_properties(${SDL3_SAMPLE_NAME} PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/bin/${SDL3_SAMPLE_NAME}/${CMAKE_CFG_INTDIR}
    )
endif()

message(STATUS "SDL3 Sample Configuration:")
message(STATUS "  OpenGL:   ${USE_OPENGL}")
message(STATUS "  Vulkan:   ${USE_VULKAN}")
message(STATUS "  SDL3_GPU: ${USE_GPU}")
message(STATUS "  - backend: Vulkan:  ${USE_GPU_BACKEND_VULKAN}")
message(STATUS "  - backend: D3D12:   ${USE_GPU_BACKEND_D3D12}")
message(STATUS "  - backend: Metal:   ${USE_GPU_BACKEND_METAL}")
message(STATUS "  - backend: Private: ${USE_GPU_BACKEND_PRIVATE}")
