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
	set "BACKEND=bullet"
	set "BUILD_DIR_NAME=python-cmake-bullet"
	set "INSTALL_DIR_NAME=python-bullet"
) else if /I "%BACKEND%"=="tau" (
	set "BACKEND=tau"
	set "BUILD_DIR_NAME=python-cmake-tau"
	set "INSTALL_DIR_NAME=python-tau"
) else (
	echo Backend invalide: "%BACKEND%"
	echo Valeurs attendues: bullet ou tau
	exit /b 1
)

for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"

if not defined BUILD_DIR set "BUILD_DIR=%WORK_DIR%\build\%BUILD_DIR_NAME%"
if not defined INSTALL_DIR set "INSTALL_DIR=%WORK_DIR%\install\%INSTALL_DIR_NAME%"
if not defined FABGEN_DIR set "FABGEN_DIR=%WORK_DIR%\FABGen"
if not defined GENERATOR set "GENERATOR=Visual Studio 17 2022"
if not defined PLATFORM set "PLATFORM=x64"
set "CMAKE_PLATFORM_ARG=-A %PLATFORM%"

if not exist "%FABGEN_DIR%\bind.py" (
	echo FABGen introuvable: "%FABGEN_DIR%"
	echo Clone https://github.com/astrofra/FABGen.git dans ce dossier ou definis FABGEN_DIR.
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
echo.

echo [1/2] Configuration CMake HG Python (%BACKEND%, %CONFIG%)...
cmake -S "%REPO_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" %CMAKE_PLATFORM_ARG% ^
	-DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
	-DHG_FABGEN_PATH="%FABGEN_DIR%" ^
	-DPython3_EXECUTABLE="%PYTHON_EXE%" ^
	-DHG_BUILD_HG_LUA=OFF ^
	-DHG_BUILD_HG_SQUIRREL=OFF ^
	-DHG_BUILD_HG_PYTHON=ON ^
	-DHG_BUILD_HG_GO=OFF ^
	-DHG_PYTHON_PIP=OFF ^
	-DHG_PYTHON_AUTO_INSTALL_WHEEL=OFF ^
	-DHG_BUILD_ASSETC=ON ^
	-DHG_BUILD_ASSIMP_CONVERTER=OFF ^
	-DHG_BUILD_FBX_CONVERTER=OFF ^
	-DHG_BUILD_GLTF_IMPORTER=OFF ^
	-DHG_BUILD_GLTF_EXPORTER=OFF ^
	-DHG_BUILD_LEGACY_ARCHIVE=ON ^
	-DHG_SCENE_PHYSICS_BACKEND=%BACKEND%
if errorlevel 1 exit /b !errorlevel!

echo [2/2] Build et install HG Python + AssetC (%BACKEND%, %CONFIG%)...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target INSTALL -- /m
if errorlevel 1 exit /b !errorlevel!

if not exist "%INSTALL_DIR%\hg_python\harfang-*-py3-none-*.whl" (
	echo Echec: wheel absente dans "%INSTALL_DIR%\hg_python".
	exit /b 1
)

set "WHEEL_NAME="
for /f "delims=" %%I in ('dir /b /a-d /o-d "%INSTALL_DIR%\hg_python\harfang-*-py3-none-*.whl" 2^>nul') do if not defined WHEEL_NAME set "WHEEL_NAME=%%I"
set "WHEEL_PATH=%INSTALL_DIR%\hg_python\!WHEEL_NAME!"

echo.
echo HG Python + AssetC rebuild ok.
echo Backend: "%BACKEND%"
echo Wheel: "!WHEEL_PATH!"
echo Installation: "%PYTHON_EXE%" -m pip install "!WHEEL_PATH!"
exit /b 0
