@echo off
setlocal
pushd "%~dp0"
if not exist bin\Release\stock_presentation.exe goto missing
bin\Release\stock_presentation.exe
set "game_result=%errorlevel%"
if not "%game_result%"=="0" (
echo Exit code: %game_result%
pause
)
popd
exit /b %game_result%
:missing
echo Build the V2 solution first.
pause
popd
exit /b 1
