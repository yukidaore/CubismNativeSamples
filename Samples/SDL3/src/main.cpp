/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppDelegate.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif

int main(int argc, char* argv[])
{
#if defined(_WIN32)
    // Setting the console character encoding to UTF-8
    UINT preConsoleOutputCP = GetConsoleOutputCP();
    SetConsoleOutputCP(65001);
#endif

    // create the application instance
    if (!LAppDelegate::GetInstance()->Initialize())
    {
#if defined(_WIN32)
        SetConsoleOutputCP(preConsoleOutputCP);
#endif
        return 1;
    }

    LAppDelegate::GetInstance()->Run();

#if defined(_WIN32)
    SetConsoleOutputCP(preConsoleOutputCP);
#endif

    return 0;
}
