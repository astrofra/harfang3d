@echo off
setlocal EnableExtensions

for %%I in ("%~dp0.") do set "QA_DIR=%%~fI"
for %%I in ("%QA_DIR%\..") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"

if not defined HG_LUA_DIR set "HG_LUA_DIR=%WORK_DIR%\install\lua\hg_lua"

set "ASSETC_EXE=%HG_LUA_DIR%\harfang\assetc\assetc.exe"

if not exist "%ASSETC_EXE%" (
	echo assetc not found: "%ASSETC_EXE%"
	echo Rebuild/install HG Lua first, or define HG_LUA_DIR.
	exit /b 1
)

pushd "%QA_DIR%"
"%ASSETC_EXE%" assets
set "EXITCODE=%ERRORLEVEL%"
popd

exit /b %EXITCODE%
