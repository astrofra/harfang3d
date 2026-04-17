@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"

for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"

set "BUILD_DIR=%WORK_DIR%\build\ffmpeg-plugin"
set "INSTALL_DIR=%WORK_DIR%\install\ffmpeg-plugin"
set "FABGEN_DIR=%WORK_DIR%\FABGen"
set "GENERATOR=Visual Studio 17 2022"
set "PLATFORM=x64"

if not "%~2"=="" set "FFMPEG_ROOT=%~2"
if not defined FFMPEG_ROOT if exist "%WORK_DIR%\ffmpeg\" set "FFMPEG_ROOT=%WORK_DIR%\ffmpeg"

if not exist "%FABGEN_DIR%\" (
	echo FABGen introuvable: "%FABGEN_DIR%"
	echo Clone FABGen dans ce dossier ou adapte FABGEN_DIR dans ce script.
	exit /b 1
)

if not defined FFMPEG_ROOT (
	echo FFmpeg SDK introuvable.
	echo Definis FFMPEG_ROOT ou passe son chemin en deuxieme argument:
	echo   rebuild_hg_ffmpeg.bat %CONFIG% C:\path\to\ffmpeg
	exit /b 1
)

if not exist "%FFMPEG_ROOT%\include\libavformat\avformat.h" (
	echo Headers FFmpeg introuvables dans "%FFMPEG_ROOT%\include".
	echo Verifie FFMPEG_ROOT: "%FFMPEG_ROOT%"
	exit /b 1
)

if not exist "%FFMPEG_ROOT%\lib\avformat.lib" (
	echo Librairies FFmpeg introuvables dans "%FFMPEG_ROOT%\lib".
	echo Verifie FFMPEG_ROOT: "%FFMPEG_ROOT%"
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
if not exist "%INSTALL_DIR%\%CONFIG%" mkdir "%INSTALL_DIR%\%CONFIG%"

echo [1/3] Configuration CMake...
cmake -S "%REPO_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" -A "%PLATFORM%" ^
	-DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
	-DHG_FABGEN_PATH="%FABGEN_DIR%" ^
	-DPython3_EXECUTABLE="%PYTHON_EXE%" ^
	-DFFMPEG_ROOT="%FFMPEG_ROOT%" ^
	-DHG_BUILD_FFMPEG_PLUGIN=ON ^
	-DHG_BUILD_CPP_SDK=OFF ^
	-DHG_BUILD_TESTS=OFF ^
	-DHG_BUILD_DOCS=OFF ^
	-DHG_BUILD_STATIC_DOCS=OFF ^
	-DHG_BUILD_HG_LUA=OFF ^
	-DHG_BUILD_HG_PYTHON=OFF ^
	-DHG_BUILD_HG_GO=OFF ^
	-DHG_BUILD_ASSETC=OFF ^
	-DHG_BUILD_ASSIMP_CONVERTER=OFF ^
	-DHG_BUILD_FBX_CONVERTER=OFF ^
	-DHG_BUILD_GLTF_IMPORTER=OFF ^
	-DHG_BUILD_GLTF_EXPORTER=OFF
if errorlevel 1 exit /b !errorlevel!

echo [2/3] Build hg_ffmpeg (%CONFIG%)...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target hg_ffmpeg
if errorlevel 1 exit /b !errorlevel!

set "PLUGIN_DLL=%BUILD_DIR%\plugins\ffmpeg\%CONFIG%\hg_ffmpeg.dll"
if not exist "%PLUGIN_DLL%" (
	echo DLL du plugin introuvable: "%PLUGIN_DLL%"
	exit /b 1
)

echo [3/3] Copie des DLL...
copy /Y "%PLUGIN_DLL%" "%INSTALL_DIR%\%CONFIG%\" >nul
if errorlevel 1 exit /b !errorlevel!

if exist "%FFMPEG_ROOT%\bin\*.dll" (
	copy /Y "%FFMPEG_ROOT%\bin\*.dll" "%INSTALL_DIR%\%CONFIG%\" >nul
	if errorlevel 1 exit /b !errorlevel!
)

echo.
echo FFmpeg plugin rebuild ok.
echo Plugin: "%INSTALL_DIR%\%CONFIG%\hg_ffmpeg.dll"
echo DLLs: "%INSTALL_DIR%\%CONFIG%"
