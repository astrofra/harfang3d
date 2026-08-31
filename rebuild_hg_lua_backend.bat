@echo off
setlocal EnableExtensions EnableDelayedExpansion
set /a HG_BATCH_PAUSE_DEPTH+=1

call :main %*
set "EXITCODE=%ERRORLEVEL%"
if !HG_BATCH_PAUSE_DEPTH! LEQ 1 if not defined HG_BATCH_NO_PAUSE pause
exit /b %EXITCODE%

:main
set "BACKEND=%~1"
set "CONFIG=%~2"

if "%BACKEND%"=="" (
	echo Usage: %~nx0 ^<bullet^|tau^> [Config]
	exit /b 1
)

if "%CONFIG%"=="" set "CONFIG=Release"

if /I "%BACKEND%"=="bullet" (
	set "BUILD_DIR_NAME=lua-cmake-bullet"
	set "INSTALL_DIR_NAME=bullet"
) else if /I "%BACKEND%"=="tau" (
	set "BUILD_DIR_NAME=lua-cmake-tau"
	set "INSTALL_DIR_NAME=tau"
) else (
	echo Backend invalide: "%BACKEND%"
	echo Valeurs attendues: bullet ou tau
	exit /b 1
)

for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"

set "BUILD_DIR=%WORK_DIR%\build\%BUILD_DIR_NAME%"
set "INSTALL_DIR=%WORK_DIR%\install\%INSTALL_DIR_NAME%"
set "FABGEN_DIR=%WORK_DIR%\FABGen"
set "GENERATOR=Visual Studio 17 2022"
set "PLATFORM=x64"
set "BUILD_TARGETS=hg_lua lua launcher launcher_noconsole assetc audio_xmp recastc legacy_archive"

if /I "%BACKEND%"=="bullet" (
	set "BUILD_TARGETS=%BUILD_TARGETS% bulletc"
)

if not exist "%FABGEN_DIR%\" (
	echo FABGen introuvable: "%FABGEN_DIR%"
	echo Clone FABGen dans ce dossier ou adapte FABGEN_DIR dans ce script.
	exit /b 1
)

if not defined PYTHON_EXE (
	for /f "delims=" %%I in ('py -3 -c "import sys; print(sys.executable)" 2^>nul') do set "PYTHON_EXE=%%I"
)
if not defined PYTHON_EXE (
	for /f "delims=" %%I in ('python -c "import sys; print(sys.executable)" 2^>nul') do set "PYTHON_EXE=%%I"
)
if not defined PYTHON_EXE (
	echo Python 3 introuvable. Definis PYTHON_EXE ou installe Python 3 dans le PATH.
	exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

echo Build dir   : "%BUILD_DIR%"
echo Install dir : "%INSTALL_DIR%\hg_lua"
echo.

echo [1/3] Configuration CMake HG Lua (%BACKEND%, %CONFIG%)...
cmake -S "%REPO_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" -A "%PLATFORM%" ^
	-DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
	-DHG_FABGEN_PATH="%FABGEN_DIR%" ^
	-DPython3_EXECUTABLE="%PYTHON_EXE%" ^
	-DHG_BUILD_HG_LUA=ON ^
	-DHG_BUILD_ASSETC=ON ^
	-DHG_BUILD_ASSIMP_CONVERTER=OFF ^
	-DHG_BUILD_FBX_CONVERTER=OFF ^
	-DHG_BUILD_GLTF_IMPORTER=OFF ^
	-DHG_BUILD_GLTF_EXPORTER=OFF ^
	-DHG_BUILD_LEGACY_ARCHIVE=ON ^
	-DHG_SCENE_PHYSICS_BACKEND=%BACKEND%
if errorlevel 1 exit /b !errorlevel!

echo [2/3] Build targets HG Lua + AssetC (%BACKEND%, %CONFIG%)...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target %BUILD_TARGETS% -- /m
if errorlevel 1 exit /b !errorlevel!

echo [3/3] Install package (%BACKEND%, %CONFIG%)...
cmake --install "%BUILD_DIR%" --config "%CONFIG%"
if errorlevel 1 exit /b !errorlevel!

if not exist "%INSTALL_DIR%\hg_lua\lua.exe" (
	echo Echec: package incomplet, "%INSTALL_DIR%\hg_lua\lua.exe" absent.
	exit /b 1
)

echo.
echo HG Lua + AssetC rebuild ok.
echo Backend: "%BACKEND%"
echo Install: "%INSTALL_DIR%\hg_lua"
exit /b 0
