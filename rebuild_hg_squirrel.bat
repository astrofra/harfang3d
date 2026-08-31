@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"

for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"

if not defined BUILD_DIR set "BUILD_DIR=%WORK_DIR%\build\squirrel-cmake"
if not defined INSTALL_DIR set "INSTALL_DIR=%WORK_DIR%\install\squirrel"
if not defined FABGEN_DIR set "FABGEN_DIR=%WORK_DIR%\FABGen"
if not defined SQUIRREL_DIR set "SQUIRREL_DIR=%REPO_DIR%\extern\squirrel"
set "GENERATOR=Visual Studio 17 2022"
set "PLATFORM=x64"
set "CMAKE_PLATFORM_ARG=-A %PLATFORM%"

if not exist "%FABGEN_DIR%\" (
	echo FABGen introuvable: "%FABGEN_DIR%"
	echo Clone FABGen dans ce dossier ou adapte FABGEN_DIR dans ce script.
	exit /b 1
)

if not exist "%SQUIRREL_DIR%\include\squirrel.h" (
	echo Squirrel introuvable: "%SQUIRREL_DIR%"
	echo Vendorise Squirrel dans ce dossier, definis SQUIRREL_DIR, ou adapte ce script.
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

if exist "%BUILD_DIR%\CMakeCache.txt" (
	set "CACHED_GENERATOR_PLATFORM="
	for /f "tokens=2 delims==" %%I in ('findstr /B /C:"CMAKE_GENERATOR_PLATFORM:INTERNAL=" "%BUILD_DIR%\CMakeCache.txt"') do set "CACHED_GENERATOR_PLATFORM=%%I"
	if "!CACHED_GENERATOR_PLATFORM!"=="" (
		set "CMAKE_PLATFORM_ARG="
	) else (
		set "CMAKE_PLATFORM_ARG=-A !CACHED_GENERATOR_PLATFORM!"
	)
)

echo [1/2] Configuration CMake...
cmake -S "%REPO_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" %CMAKE_PLATFORM_ARG% ^
	-DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
	-DHG_FABGEN_PATH="%FABGEN_DIR%" ^
	-DHG_SQUIRREL_PATH="%SQUIRREL_DIR%" ^
	-DPython3_EXECUTABLE="%PYTHON_EXE%" ^
	-DHG_BUILD_HG_LUA=OFF ^
	-DHG_BUILD_HG_SQUIRREL=ON ^
	-DHG_BUILD_ASSETC=ON ^
	-DHG_BUILD_ASSIMP_CONVERTER=OFF ^
	-DHG_BUILD_FBX_CONVERTER=OFF ^
	-DHG_BUILD_GLTF_IMPORTER=OFF ^
	-DHG_BUILD_GLTF_EXPORTER=OFF
if errorlevel 1 exit /b !errorlevel!

echo [2/2] Build et install HG Squirrel + AssetC (%CONFIG%)...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target INSTALL
if errorlevel 1 exit /b !errorlevel!

if not exist "%BUILD_DIR%\tools\assetc\cmake_install.cmake" (
	echo Script d'installation AssetC introuvable: "%BUILD_DIR%\tools\assetc\cmake_install.cmake"
	exit /b 1
)

cmake -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%\hg_squirrel\harfang" -DCMAKE_INSTALL_CONFIG_NAME="%CONFIG%" -DBUILD_TYPE="%CONFIG%" -P "%BUILD_DIR%\tools\assetc\cmake_install.cmake"
if errorlevel 1 exit /b !errorlevel!

echo.
echo HG Squirrel + AssetC rebuild ok.
echo Install: "%INSTALL_DIR%\hg_squirrel"
