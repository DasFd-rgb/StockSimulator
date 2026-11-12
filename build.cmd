@echo off
setlocal
pushd "%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto missing
set "MSBUILD="
set "VSPATH="
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%I"
if not defined VSPATH goto missing
set "MSBUILD=%VSPATH%\MSBuild\Current\Bin\MSBuild.exe"
set "TOOLSET=v142"
if exist "%VSPATH%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.v142.default.txt" goto build
if exist "%VSPATH%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.v143.default.txt" set "TOOLSET=v143"
:build
if not exist build\test-data mkdir build\test-data
"%MSBUILD%" StockSimulatorV2.sln /m:1 /nr:false /nologo /v:minimal /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=%TOOLSET%
if errorlevel 1 goto error
tests\bin\Debug\test_game.exe build\test-data
if errorlevel 1 goto error
"%MSBUILD%" StockSimulatorV2.sln /m:1 /nr:false /nologo /v:minimal /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=%TOOLSET%
if errorlevel 1 goto error
tests\bin\Release\test_game.exe build\test-data
if errorlevel 1 goto error
echo Build and tests completed with %TOOLSET%. Project default remains v142 for Visual Studio 2019.
popd
exit /b 0
:missing
echo Install Visual Studio 2019 16.8+ with Desktop development with C++, MSVC v142 and Windows 10 SDK.
:error
popd
exit /b 1
