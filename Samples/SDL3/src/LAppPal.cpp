/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include <cstdio>
#include <stdarg.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <SDL3/SDL.h>

#if defined(_WIN32)
#include <Windows.h>
#endif

using namespace Csm;

namespace {
const csmBool DebugLogToFileEnable = false;

void WriteDebugLogToFile(const csmChar* text)
{
    if (!DebugLogToFileEnable)
    {
        return;
    }

    static std::ofstream logFile("SDL3Demo_trace.log", std::ios::out | std::ios::app);
    if (!logFile.is_open())
    {
        return;
    }
    logFile << text;
    logFile.flush();
}
}

double LAppPal::s_currentFrame = 0.0;
double LAppPal::s_lastFrame = 0.0;
double LAppPal::s_deltaTime = 0.0;

csmByte* LAppPal::LoadFileAsBytes(const std::string filePath, csmSizeInt* outSize)
{
    const char* path = filePath.c_str();

    // file open
    std::ifstream file;
    file.open(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        if (LAppDefine::DebugLogEnable)
        {
            PrintLogLn("file open failed : %s", path);
        }
        return NULL;
    }

    // get file size
    file.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    // read file
    csmByte* buf = new csmByte[size];
    file.read(reinterpret_cast<char*>(buf), size);
    file.close();

    *outSize = static_cast<csmSizeInt>(size);
    return buf;
}

void LAppPal::ReleaseBytes(csmByte* byteData)
{
    delete[] byteData;
}

csmFloat32 LAppPal::GetDeltaTime()
{
    return static_cast<csmFloat32>(s_deltaTime);
}

void LAppPal::UpdateTime()
{
    s_currentFrame = static_cast<double>(SDL_GetTicks()) / 1000.0;
    s_deltaTime = s_currentFrame - s_lastFrame;
    s_lastFrame = s_currentFrame;
}

void LAppPal::PrintLog(const csmChar* format, ...)
{
    va_list args;
    csmChar buf[256];
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
#if defined(_WIN32)
    OutputDebugStringA(buf);
#endif
    std::cerr << buf;
    WriteDebugLogToFile(buf);
    va_end(args);
}

void LAppPal::PrintLogLn(const csmChar* format, ...)
{
    va_list args;
    csmChar buf[256];
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
#if defined(_WIN32)
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
#endif
    std::cerr << buf << std::endl;
    WriteDebugLogToFile(buf);
    WriteDebugLogToFile("\n");
}

void LAppPal::PrintMessage(const csmChar* message)
{
#if defined(_WIN32)
    OutputDebugStringA(message);
#endif
    std::cerr << message;
    WriteDebugLogToFile(message);
}

void LAppPal::PrintMessageLn(const csmChar* message)
{
    PrintLog("%s\n", message);
}

void LAppPal::ConvertMultiByteToWide(const char* multiByte, wchar_t* wide, int wideSize)
{
#if defined(_WIN32)
    MultiByteToWideChar(CP_UTF8, 0, multiByte, -1, wide, wideSize);
#else
    // Simple conversion for non-Windows platforms
    mbstowcs(wide, multiByte, wideSize);
#endif
}

void LAppPal::ConvertWideToMultiByte(const wchar_t* wide, char* multiByte, int multiByteSize)
{
#if defined(_WIN32)
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, multiByte, multiByteSize, NULL, NULL);
#else
    // Simple conversion for non-Windows platforms
    wcstombs(multiByte, wide, multiByteSize);
#endif
}
