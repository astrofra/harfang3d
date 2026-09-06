@echo off
setlocal EnableExtensions EnableDelayedExpansion
set /a HG_BATCH_PAUSE_DEPTH+=1

call :main %*
set "EXITCODE=%ERRORLEVEL%"
if !HG_BATCH_PAUSE_DEPTH! LEQ 1 if not defined HG_BATCH_NO_PAUSE pause
exit /b %EXITCODE%

:main
set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"

for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"

if not defined BUILD_DIR set "BUILD_DIR=%WORK_DIR%\build\tools-cmake"
if not defined INSTALL_DIR set "INSTALL_DIR=%WORK_DIR%\install"
if not defined FABGEN_DIR set "FABGEN_DIR=%WORK_DIR%\FABGen"
if not defined GENERATOR set "GENERATOR=Visual Studio 17 2022"
if not defined PLATFORM set "PLATFORM=x64"
if not defined BUILD_JOBS set "BUILD_JOBS=1"
set "CMAKE_PLATFORM_ARG=-A %PLATFORM%"

if not exist "%FABGEN_DIR%\bind.py" (
	echo FABGen introuvable: "%FABGEN_DIR%"
	echo Clone https://github.com/astrofra/FABGen.git dans ce dossier ou definis FABGEN_DIR.
	exit /b 1
)

if not defined HG_FBX_SDK (
	echo Autodesk FBX SDK introuvable. Definis HG_FBX_SDK avec le chemin de son installation.
	exit /b 1
)
if not exist "%HG_FBX_SDK%\include\fbxsdk.h" (
	echo Installation Autodesk FBX SDK invalide: "%HG_FBX_SDK%"
	echo Le fichier include\fbxsdk.h est absent.
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
if errorlevel 1 exit /b !errorlevel!
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
if errorlevel 1 exit /b !errorlevel!

if exist "%BUILD_DIR%\CMakeCache.txt" (
	set "CACHED_GENERATOR_PLATFORM="
	for /f "tokens=2 delims==" %%I in ('findstr /B /C:"CMAKE_GENERATOR_PLATFORM:INTERNAL=" "%BUILD_DIR%\CMakeCache.txt"') do set "CACHED_GENERATOR_PLATFORM=%%I"
	if "!CACHED_GENERATOR_PLATFORM!"=="" (
		set "CMAKE_PLATFORM_ARG="
	) else (
		set "CMAKE_PLATFORM_ARG=-A !CACHED_GENERATOR_PLATFORM!"
	)
)

echo Build dir   : "%BUILD_DIR%"
echo Install dir : "%INSTALL_DIR%"
echo Build jobs  : %BUILD_JOBS%
echo.

echo [1/3] Configuration CMake des tools (%CONFIG%)...
cmake -S "%REPO_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" %CMAKE_PLATFORM_ARG% ^
	-DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
	-DHG_FABGEN_PATH="%FABGEN_DIR%" ^
	-DPython3_EXECUTABLE="%PYTHON_EXE%" ^
	-DHG_FBX_SDK="%HG_FBX_SDK%" ^
	-DHG_BUILD_CPP_SDK=OFF ^
	-DHG_BUILD_TESTS=OFF ^
	-DHG_BUILD_DOCS=OFF ^
	-DHG_BUILD_STATIC_DOCS=OFF ^
	-DHG_BUILD_HG_LUA=OFF ^
	-DHG_BUILD_HG_SQUIRREL=OFF ^
	-DHG_BUILD_HG_PYTHON=OFF ^
	-DHG_BUILD_HG_GO=OFF ^
	-DHG_BUILD_ASSETC=OFF ^
	-DHG_BUILD_ASSIMP_CONVERTER=ON ^
	-DHG_BUILD_FBX_CONVERTER=ON ^
	-DHG_BUILD_GLTF_IMPORTER=ON ^
	-DHG_BUILD_GLTF_EXPORTER=ON ^
	-DHG_BUILD_LEGACY_ARCHIVE=ON ^
	-DHG_BUILD_FFMPEG_PLUGIN=OFF ^
	-DHG_ENABLE_XMP_AUDIO=OFF ^
	-DHG_SCENE_PHYSICS_BACKEND=bullet
if errorlevel 1 exit /b !errorlevel!

echo [2/3] Build des tools (%CONFIG%)...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target assimp_converter fbx_converter gltf_importer gltf_exporter legacy_archive -- /m:%BUILD_JOBS%
if errorlevel 1 exit /b !errorlevel!

echo [3/3] Package des tools...
for %%T in (assimp_converter fbx_converter gltf_importer gltf_exporter legacy_archive) do (
	cmake --install "%BUILD_DIR%" --config "%CONFIG%" --component %%T
	if errorlevel 1 exit /b !errorlevel!
)

for %%T in (assimp_converter fbx_converter gltf_importer gltf_exporter legacy_archive) do (
	if not exist "%INSTALL_DIR%\%%T\%%T.exe" (
		echo Echec: package incomplet, "%INSTALL_DIR%\%%T\%%T.exe" absent.
		exit /b 1
	)
)

echo.
echo Tools rebuild ok.
echo Install: "%INSTALL_DIR%\assimp_converter"
echo Install: "%INSTALL_DIR%\fbx_converter"
echo Install: "%INSTALL_DIR%\gltf_importer"
echo Install: "%INSTALL_DIR%\gltf_exporter"
echo Install: "%INSTALL_DIR%\legacy_archive"
exit /b 0
