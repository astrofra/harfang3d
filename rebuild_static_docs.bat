@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"

for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"

set "BUILD_DIR=%WORK_DIR%\build\docs-static"
set "INSTALL_DIR=%WORK_DIR%\install\docs-static"
set "FABGEN_DIR=%WORK_DIR%\FABGen"
set "GENERATOR=Visual Studio 17 2022"
set "PLATFORM=x64"
set "OPEN_DOCS=0"

if /I "%~2"=="--open" set "OPEN_DOCS=1"
if /I "%~1"=="--open" (
	set "CONFIG=Release"
	set "OPEN_DOCS=1"
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

echo [1/2] Configuration CMake...
cmake -S "%REPO_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" -A "%PLATFORM%" ^
	-DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
	-DHG_FABGEN_PATH="%FABGEN_DIR%" ^
	-DPython3_EXECUTABLE="%PYTHON_EXE%" ^
	-DHG_BUILD_STATIC_DOCS=ON ^
	-DHG_BUILD_DOCS=OFF ^
	-DHG_BUILD_CPP_SDK=OFF ^
	-DHG_BUILD_TESTS=OFF ^
	-DHG_BUILD_HG_LUA=OFF ^
	-DHG_BUILD_HG_PYTHON=OFF ^
	-DHG_BUILD_HG_GO=OFF ^
	-DHG_BUILD_ASSETC=OFF ^
	-DHG_BUILD_ASSIMP_CONVERTER=OFF ^
	-DHG_BUILD_FBX_CONVERTER=OFF ^
	-DHG_BUILD_GLTF_IMPORTER=OFF ^
	-DHG_BUILD_GLTF_EXPORTER=OFF
if errorlevel 1 exit /b !errorlevel!

echo [2/2] Generation documentation statique (%CONFIG%)...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target harfang_static_docs_install
if errorlevel 1 exit /b !errorlevel!

set "DOC_INDEX=%INSTALL_DIR%\docs\harfang\index.html"

echo.
echo Static docs rebuild ok.
echo Open: "%DOC_INDEX%"

if "%OPEN_DOCS%"=="1" (
	start "" "%DOC_INDEX%"
)
