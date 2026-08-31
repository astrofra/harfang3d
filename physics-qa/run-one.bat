@echo off
setlocal EnableExtensions

if "%~1"=="" (
	echo Usage: run-one.bat script.lua
	exit /b 1
)

for %%I in ("%~dp0.") do set "QA_DIR=%%~fI"
for %%I in ("%QA_DIR%\..") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"

if not defined HG_LUA_DIR set "HG_LUA_DIR=%WORK_DIR%\install\lua\hg_lua"

set "LUA_EXE=%HG_LUA_DIR%\lua.exe"
set "SCRIPT=%~1"

if not exist "%LUA_EXE%" (
	echo Lua runtime not found: "%LUA_EXE%"
	echo Rebuild/install HG Lua first, or define HG_LUA_DIR.
	exit /b 1
)

if not exist "%QA_DIR%\%SCRIPT%" (
	echo Script not found: "%QA_DIR%\%SCRIPT%"
	exit /b 1
)

pushd "%QA_DIR%"
"%LUA_EXE%" "%SCRIPT%"
set "EXITCODE=%ERRORLEVEL%"
popd

exit /b %EXITCODE%
