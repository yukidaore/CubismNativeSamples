set(shader_files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VertSprite.vert.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FragSprite.frag.hlsl
)

if(NOT DEFINED SHADERCROSS_COMMAND)
  if(DEFINED ENV{SDL_SHADERCROSS_PATH})
    set(SHADERCROSS_COMMAND "$ENV{SDL_SHADERCROSS_PATH}/shadercross")
  else()
    message(FATAL_ERROR "SDL_SHADERCROSS_PATH environment variable is not set and SHADERCROSS_COMMAND is not defined.")
  endif()
endif()

set(output_base_dir ${CMAKE_CURRENT_BINARY_DIR}/compiledShaders)
set(spv_dir ${output_base_dir}/spv)
set(dxil_dir ${output_base_dir}/dxil)
set(msl_dir ${output_base_dir}/msl)
set(shader_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)

foreach(shader ${shader_files})
    get_filename_component(file_name ${shader} NAME)
    get_filename_component(full_path ${shader} ABSOLUTE)
    string(REGEX REPLACE "\\.vert\\.hlsl$|\\.frag\\.hlsl$" "" base_name ${file_name})
    set_source_files_properties(${shader} PROPERTIES HEADER_FILE_ONLY TRUE)

    if(file_name MATCHES "\\.vert\\.hlsl$")
        set(SHADER_STAGE "vertex")
    else()
        set(SHADER_STAGE "fragment")
    endif()

    set(spv_file ${spv_dir}/${base_name}.spv)
    set(dxil_file ${dxil_dir}/${base_name}.dxil)
    set(msl_file ${msl_dir}/${base_name}.msl)

    add_custom_command(
        OUTPUT ${spv_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${spv_dir}
        COMMAND ${SHADERCROSS_COMMAND} ${full_path} -s HLSL -d SPIRV -t ${SHADER_STAGE} -e main -I ${shader_include_dir} -o ${spv_file}
        DEPENDS ${full_path} ${SHADERCROSS_TARGET_DEPENDS}
    )

    add_custom_command(
        OUTPUT ${dxil_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dxil_dir}
        COMMAND ${SHADERCROSS_COMMAND} ${full_path} -s HLSL -d DXIL -t ${SHADER_STAGE} -e main -I ${shader_include_dir} -o ${dxil_file}
        DEPENDS ${full_path} ${SHADERCROSS_TARGET_DEPENDS}
    )

    add_custom_command(
        OUTPUT ${msl_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${msl_dir}
        COMMAND ${SHADERCROSS_COMMAND} ${full_path} -s HLSL -d MSL -t ${SHADER_STAGE} -e main -I ${shader_include_dir} -o ${msl_file}
        DEPENDS ${full_path} ${SHADERCROSS_TARGET_DEPENDS}
    )

    list(APPEND compiled_shaders_app_spv ${spv_file})
    list(APPEND compiled_shaders_app_dxil ${dxil_file})
    list(APPEND compiled_shaders_app_msl ${msl_file})
endforeach()

source_group("shaders" FILES ${shader_files})

# フォーマット別のカスタムターゲット
add_custom_target(SampleShaders_SPIRV
    DEPENDS ${compiled_shaders_app_spv}
    SOURCES ${shader_files}
)

add_custom_target(SampleShaders_DXIL
    DEPENDS ${compiled_shaders_app_dxil}
    SOURCES ${shader_files}
)

add_custom_target(SampleShaders_MSL
    DEPENDS ${compiled_shaders_app_msl}
    SOURCES ${shader_files}
)

# 全フォーマットをまとめてビルドするための便利ターゲット
add_custom_target(SampleShaders
    DEPENDS SampleShaders_SPIRV SampleShaders_DXIL SampleShaders_MSL
)
