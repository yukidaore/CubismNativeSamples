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

#
# TODO 現状、vulkanのglslcで.spvをプリビルドする構成になっているので使用していない
#
# # SPIRV-Tools options
# set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
# set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
# set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "" FORCE)
# set(SPIRV_TOOLS_BUILD_STATIC ON CACHE BOOL "" FORCE)

# FetchContent_Declare(
# 	SPIRV-Headers
# 	GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
# 	GIT_TAG vulkan-sdk-1.4.335.0
# )
# FetchContent_MakeAvailable(SPIRV-Headers)

# FetchContent_Declare(
# 	SPIRV-Tools
# 	GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
# 	GIT_TAG vulkan-sdk-1.4.335.0
# )
# FetchContent_MakeAvailable(SPIRV-Tools)

# # glslang options
# set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
# set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
# set(ENABLE_HLSL ON CACHE BOOL "" FORCE)
# set(ENABLE_OPT ON CACHE BOOL "" FORCE)
# set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
# set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# FetchContent_Declare(
# 	glslang
# 	GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
# 	GIT_TAG 16.2.0
# )
# FetchContent_MakeAvailable(glslang)

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
    # Vulkan uses pre-compiled shaders with standard Vulkan binding layout
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
elseif(USE_GPU)
    # SDL3 GPU uses SPIR-V with SDL3-specific descriptor set layout
    # (vertex: set 0=samplers, set 1=UBO; fragment: set 2=samplers, set 3=UBO)
    set(SAMPLE_SHADER_PATH ${SDK_ROOT_PATH}/Samples/SDL3/Shaders)
    set(FRAMEWORK_SHADER_PATH ${FRAMEWORK_PATH}/src/Rendering/SDL3_GPU/Shaders)
    add_subdirectory(${SAMPLE_SHADER_PATH} ${CMAKE_CURRENT_BINARY_DIR}/SDL3SampleShaders)
    add_subdirectory(${FRAMEWORK_SHADER_PATH} ${CMAKE_CURRENT_BINARY_DIR}/SDL3FrameworkShaders)
    add_dependencies(Framework SampleShaders)
    add_dependencies(Framework FrameworkShaders)

    # copy compiled shader binaries
    add_custom_command(
      TARGET ${SDL3_SAMPLE_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/SDL3SampleShaders/compiledShaders $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/SampleShaders
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/SDL3FrameworkShaders/compiledShaders $<TARGET_FILE_DIR:${SDL3_SAMPLE_NAME}>/FrameworkShaders
    )
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
