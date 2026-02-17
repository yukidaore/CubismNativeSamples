/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppSpriteShader.hpp"

#include <fstream>
#include <sstream>

#include "LAppDefine.hpp"
#include "LAppPal.hpp"

using namespace LAppDefine;

LAppSpriteShader::LAppSpriteShader()
    : _programId(0)
{
    _programId = CreateShader();
}

LAppSpriteShader::~LAppSpriteShader()
{
    glDeleteShader(_programId);
}

GLuint LAppSpriteShader::GetShaderId() const
{
    return _programId;
}

GLuint LAppSpriteShader::CreateShader()
{
    // シェーダーのパスの作成
    Csm::csmString vertShaderFile(ShaderPath);
    vertShaderFile += VertShaderName;
    Csm::csmString fragShaderFile(ShaderPath);
    fragShaderFile += FragShaderName;

    // シェーダーのコンパイル
    GLuint vertexShaderId = CompileShader(vertShaderFile, GL_VERTEX_SHADER);
    GLuint fragmentShaderId = CompileShader(fragShaderFile, GL_FRAGMENT_SHADER);

    if (!vertexShaderId || !fragmentShaderId)
    {
        return 0;
    }

    //プログラムオブジェクトの作成
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    // リンク
    glLinkProgram(programId);

    // シェーダーオブジェクトの削除
    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);

    // リンク結果のチェック
    GLint status;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        glDeleteProgram(programId);
        return 0;
    }

    return programId;
}

bool LAppSpriteShader::CheckShader(GLuint shaderId)
{
    GLint status;
    GLint logLength;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar* log = reinterpret_cast<GLchar*>(CSM_MALLOC(logLength));
        glGetShaderInfoLog(shaderId, logLength, &logLength, log);
        LAppPal::PrintLogLn("Shader compile log: %s", log);
        CSM_FREE(log);
    }

    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        glDeleteShader(shaderId);
        return false;
    }

    return true;
}

GLuint LAppSpriteShader::CompileShader(Csm::csmString filename, GLenum shaderType)
{
    //ファイル読み込み
    std::ifstream file;
    file.open(filename.GetRawString(), std::ios::binary);
    if (!file.is_open())
    {
        LAppPal::PrintLogLn("Cannot open the shader file \"%s\" .", filename.GetRawString());
        return 0;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    //一旦 stringオブジェクトに変換。 stringオブジェクトは一時的なものなので、これを渡すのはNG。正しいのは const char* shaderSourceに変換したもの
    const std::string tmp = buffer.str();
    //c_strで返されるものは一時的なデータなので変数(配列)に移して渡す
    GLint size = static_cast<GLint>(tmp.length());

    const char* shaderSource = tmp.c_str();

    //シェーダーオブジェクトの作成
    GLuint shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &shaderSource, &size);

    //シェーダーのコンパイル
    glCompileShader(shaderId);

    // コンパイル結果のチェック
    if (!CheckShader(shaderId))
    {
        return 0;
    }

    return shaderId;
}
