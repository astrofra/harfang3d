@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "CONFIG=%~1"
set "RUN_PLAYER=0"

if "%CONFIG%"=="" set "CONFIG=Release"
if /I "%~1"=="--run" (
	set "CONFIG=Release"
	set "RUN_PLAYER=1"
)
if /I "%~2"=="--run" set "RUN_PLAYER=1"

for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"

set "BUILD_DIR=%WORK_DIR%\build\xm-player"
set "INSTALL_DIR=%WORK_DIR%\install"
set "PLAYER_DIR=%INSTALL_DIR%\hg_lua"
set "DATA_DIR=%PLAYER_DIR%\data"
set "RESOURCE_DIR=%PLAYER_DIR%\resources\sounds"
set "FABGEN_DIR=%WORK_DIR%\FABGen"
set "GENERATOR=Visual Studio 17 2022"
set "PLATFORM=x64"

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

echo [1/3] Configuration CMake...
cmake -S "%REPO_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" -A "%PLATFORM%" ^
	-DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
	-DHG_FABGEN_PATH="%FABGEN_DIR%" ^
	-DPython3_EXECUTABLE="%PYTHON_EXE%" ^
	-DHG_ENABLE_XMP_AUDIO=ON ^
	-DHG_BUILD_HG_LUA=ON ^
	-DHG_BUILD_CPP_SDK=OFF ^
	-DHG_BUILD_TESTS=OFF ^
	-DHG_BUILD_DOCS=OFF ^
	-DHG_BUILD_STATIC_DOCS=OFF ^
	-DHG_BUILD_HG_PYTHON=OFF ^
	-DHG_BUILD_HG_GO=OFF ^
	-DHG_BUILD_ASSETC=OFF ^
	-DHG_BUILD_ASSIMP_CONVERTER=OFF ^
	-DHG_BUILD_FBX_CONVERTER=OFF ^
	-DHG_BUILD_GLTF_IMPORTER=OFF ^
	-DHG_BUILD_GLTF_EXPORTER=OFF ^
	-DHG_ENABLE_BULLET3_SCENE_PHYSICS=OFF ^
	-DHG_ENABLE_RECAST_DETOUR_API=OFF ^
	-DHG_ENABLE_OPENVR_API=OFF ^
	-DHG_ENABLE_OPENXR_API=OFF ^
	-DHG_ENABLE_SRANIPAL_API=OFF
if errorlevel 1 exit /b !errorlevel!

echo [2/3] Build et install XM player (%CONFIG%)...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target INSTALL
if errorlevel 1 exit /b !errorlevel!

echo [3/3] Preparation du player et des ressources...
if not exist "%DATA_DIR%" mkdir "%DATA_DIR%"
if not exist "%RESOURCE_DIR%" mkdir "%RESOURCE_DIR%"

copy /Y "%REPO_DIR%\tutorials\audio_stream_mod_xm_stereo.lua" "%DATA_DIR%\audio_stream_mod_xm_stereo.lua" >nul
if errorlevel 1 exit /b !errorlevel!

copy /Y "%REPO_DIR%\tutorials\resources\sounds\4-mat's madness.mod" "%RESOURCE_DIR%\4-mat's madness.mod" >nul
if errorlevel 1 exit /b !errorlevel!

copy /Y "%REPO_DIR%\tutorials\resources\sounds\4-mat_madness_license.txt" "%RESOURCE_DIR%\4-mat_madness_license.txt" >nul
if errorlevel 1 exit /b !errorlevel!

>"%DATA_DIR%\launcher.json" (
	echo {
	echo   "entry": "audio_stream_mod_xm_stereo.lua"
	echo }
)

echo.
echo XM player rebuild ok.
echo Install: "%PLAYER_DIR%"
echo.
echo Pour lancer:
echo   cd /d "%PLAYER_DIR%"
echo   launcher.exe

if "%RUN_PLAYER%"=="1" (
	pushd "%PLAYER_DIR%"
	launcher.exe
	set "PLAYER_ERROR=!errorlevel!"
	popd
	exit /b !PLAYER_ERROR!
)
