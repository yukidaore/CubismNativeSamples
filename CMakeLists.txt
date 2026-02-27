cmake_minimum_required(VERSION 3.16)

# UTF-8 source
add_compile_options("$<$<AND:$<C_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:C>>:/utf-8>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/utf-8>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/Zc:__cplusplus>")

# Set app name.
set(APP_NAME live2dsample)
set(APP_VERSION 1.0.0)

# Set directory paths.
set(SDK_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(CORE_PATH ${SDK_ROOT_PATH}/Core)
set(FRAMEWORK_PATH ${SDK_ROOT_PATH}/Framework)
set(THIRD_PARTY_PATH ${SDK_ROOT_PATH}/Samples/OpenGL/thirdParty)
set(STB_PATH ${THIRD_PARTY_PATH}/stb)

# Set project.
project(${APP_NAME} VERSION ${APP_VERSION})

# 実行ファイルの出力先をビルドフォルダの直下の bin 以下にする
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Suppress generation of ZERO_CHECK project.
set(CMAKE_SUPPRESS_REGENERATION ON)

# Rendering backend options
option(USE_OPENGL "Use OpenGL backend" ON)
option(USE_VULKAN "Use Vulkan backend" OFF)
option(USE_GPU "Use SDL3 GPU backend" OFF)

# Detect architecture.
if(CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ARCH x86_64)
  set(DX_ARCH x64)
elseif(CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:X86")
  set(ARCH x86)
  set(DX_ARCH x86)
else()
  # Default to x64 for non-Windows platforms
  set(ARCH x86_64)
  set(DX_ARCH x64)
endif()

# Detect compiler.
if(MSVC_VERSION MATCHES 1800)
  # Visual Studio 2013
  set(COMPILER 120)
elseif(MSVC_VERSION MATCHES 1900)
  # Visual Studio 2015
  set(COMPILER 140)
elseif(MSVC_VERSION GREATER_EQUAL 1910 AND MSVC_VERSION LESS 1920)
  # Visual Studio 2017
  set(COMPILER 141)
elseif(MSVC_VERSION GREATER_EQUAL 1920 AND MSVC_VERSION LESS 1930)
  # Visual Studio 2019
  set(COMPILER 142)
elseif(MSVC_VERSION GREATER_EQUAL 1930)
  # Visual Studio 2022
  set(COMPILER 143)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  # GCC/Clang - use 143 as placeholder
  set(COMPILER 143)
elseif(MSVC)
  message(FATAL_ERROR "[${APP_NAME}] Unsupported Visual C++ compiler used.")
else()
  # Default for other compilers
  set(COMPILER 143)
endif()

# Detect core crt.
set(CRT MT)

# Add Cubism Core.
add_library(Live2DCubismCore STATIC IMPORTED)
# Find library path based on platform
if(WIN32)
  set(CORE_LIB_SUFFIX ${CORE_PATH}/lib/windows/${ARCH}/${COMPILER})
  set_target_properties(Live2DCubismCore
    PROPERTIES
      IMPORTED_LOCATION_DEBUG
        ${CORE_LIB_SUFFIX}/Live2DCubismCore_${CRT}d.lib
      IMPORTED_LOCATION_RELEASE
        ${CORE_LIB_SUFFIX}/Live2DCubismCore_${CRT}.lib
      INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include
  )
elseif(APPLE)
  set_target_properties(Live2DCubismCore
    PROPERTIES
      IMPORTED_LOCATION ${CORE_PATH}/lib/macos/libLive2DCubismCore.a
      INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include
  )
elseif(UNIX)
  set_target_properties(Live2DCubismCore
    PROPERTIES
      IMPORTED_LOCATION ${CORE_PATH}/lib/linux/${ARCH}/libLive2DCubismCore.a
      INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include
  )
endif()

# Framework Renderer Select
if(USE_OPENGL)
  set(FRAMEWORK_SOURCE OpenGL)
  if(WIN32)
    set(THIRD_PARTY_PATH ${SDK_ROOT_PATH}/Samples/OpenGL/thirdParty)
    find_package(GLEW REQUIRED)
  endif()
elseif(USE_VULKAN)
  set(FRAMEWORK_SOURCE Vulkan)
  find_package(Vulkan REQUIRED)
elseif(USE_GPU)
  set(FRAMEWORK_SOURCE SDL3_GPU)
else()
  message(FATAL_ERROR "unknown renderer type.")
endif()

# Add Cubism Native Framework.
add_subdirectory(${FRAMEWORK_PATH} ${CMAKE_CURRENT_BINARY_DIR}/Framework)

# Add rendering definition to framework based on platform and backend
if(USE_OPENGL)
  if(WIN32)
    target_compile_definitions(Framework PUBLIC CSM_TARGET_WIN_GL)
    target_link_libraries(Framework PUBLIC Live2DCubismCore GLEW::GLEW)
  elseif(APPLE)
    target_compile_definitions(Framework PUBLIC CSM_TARGET_MAC_GL)
    target_link_libraries(Framework PUBLIC Live2DCubismCore)
  elseif(UNIX)
    target_compile_definitions(Framework PUBLIC CSM_TARGET_LINUX_GL)
    target_link_libraries(Framework PUBLIC Live2DCubismCore)
  endif()
elseif(USE_VULKAN)
  target_include_directories(Framework PUBLIC ${Vulkan_INCLUDE_DIRS})
  target_link_libraries(Framework PUBLIC Vulkan::Vulkan)
elseif(USE_GPU)
  target_include_directories(Framework PUBLIC ${SDL3_INCLUDE_DIRS})
  target_link_libraries(Framework PUBLIC Live2DCubismCore SDL3::SDL3)
else()
endif()

add_subdirectory(Samples/SDL3)

# You can change target that renderer draws by enabling following definition.
#
# * USE_RENDER_TARGET
#   Renderer draws to target of LAppView.
# * USE_MODEL_RENDER_TARGET
#   Renderer draws to target of each LAppModel.
# * default
#   Renderer draws to default main framebuffer.
#
# INFO: USE_RENDER_TARGET has higher priority than USE_MODEL_RENDER_TARGET.
