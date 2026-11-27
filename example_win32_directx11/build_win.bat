@REM Build for Visual Studio compiler. Run vcvars64.bat (or vcvarsall.bat) first.
@set OUT_DIR=Debug
@set OUT_EXE=example_win32_directx11
@set IMGUI=..\third_party\imgui
@set INCLUDES=/I%IMGUI% /I%IMGUI%\backends /I.
@set SOURCES=main.cpp keyauth\auth.cpp %IMGUI%\backends\imgui_impl_dx11.cpp %IMGUI%\backends\imgui_impl_win32.cpp %IMGUI%\imgui*.cpp
@set LIBS=d3d11.lib d3dcompiler.lib dxgi.lib winhttp.lib crypt32.lib advapi32.lib
mkdir %OUT_DIR%
cl /nologo /Zi /MD /EHsc /std:c++17 /utf-8 %INCLUDES% /D UNICODE /D _UNICODE %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
