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

if not defined BUILD_DIR set "BUILD_DIR=%WORK_DIR%\build\js-cmake-bullet"
if not defined INSTALL_DIR set "INSTALL_DIR=%WORK_DIR%\install\js_bullet"
if not defined FABGEN_DIR set "FABGEN_DIR=%WORK_DIR%\FABGen"
if not defined BUILD_JOBS set "BUILD_JOBS=4"

rem Prefer explicit dependency paths, then workspace deps, then the local bootstrap cache.
if not defined HG_QUICKJS_SOURCE_DIR if exist "%WORK_DIR%\deps\quickjs-2026-06-04\VERSION" set "HG_QUICKJS_SOURCE_DIR=%WORK_DIR%\deps\quickjs-2026-06-04"
if not defined HG_QUICKJS_SOURCE_DIR if exist "%WORK_DIR%\.codex_tmp\quickjs\quickjs-2026-06-04\VERSION" set "HG_QUICKJS_SOURCE_DIR=%WORK_DIR%\.codex_tmp\quickjs\quickjs-2026-06-04"
if not defined HG_QUICKJS_ZIG if exist "%WORK_DIR%\deps\zig-x86_64-windows-0.14.1\zig.exe" set "HG_QUICKJS_ZIG=%WORK_DIR%\deps\zig-x86_64-windows-0.14.1\zig.exe"
if not defined HG_QUICKJS_ZIG if exist "%WORK_DIR%\.codex_tmp\quickjs\zig-x86_64-windows-0.14.1\zig.exe" set "HG_QUICKJS_ZIG=%WORK_DIR%\.codex_tmp\quickjs\zig-x86_64-windows-0.14.1\zig.exe"
if not defined HG_QUICKJS_ZIG (
	for /f "delims=" %%I in ('where zig.exe 2^>nul') do if not defined HG_QUICKJS_ZIG set "HG_QUICKJS_ZIG=%%I"
)

if not exist "%FABGEN_DIR%\lang\quickjs.py" (
	echo FABGen avec backend QuickJS introuvable: "%FABGEN_DIR%"
	echo Definis FABGEN_DIR vers le checkout FABGen.
	exit /b 1
)
if not exist "%HG_QUICKJS_SOURCE_DIR%\VERSION" (
	echo QuickJS 2026-06-04 introuvable. Definis HG_QUICKJS_SOURCE_DIR vers ses sources.
	exit /b 1
)
if not exist "%HG_QUICKJS_ZIG%" (
	echo Zig 0.14.1 introuvable. Definis HG_QUICKJS_ZIG vers zig.exe.
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

echo Build dir   : "%BUILD_DIR%"
echo Runtime     : "%INSTALL_DIR%\hgjs\hgjs.exe"
echo QuickJS     : "%HG_QUICKJS_SOURCE_DIR%"
echo Zig         : "%HG_QUICKJS_ZIG%"
echo.

echo [1/3] Configuration CMake HarfangJs (Bullet, %CONFIG%)...
cmake -S "%REPO_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 ^
	-DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
	-DHG_FABGEN_PATH="%FABGEN_DIR%" ^
	-DPython3_EXECUTABLE="%PYTHON_EXE%" ^
	-DHG_QUICKJS_SOURCE_DIR="%HG_QUICKJS_SOURCE_DIR%" ^
	-DHG_QUICKJS_ZIG="%HG_QUICKJS_ZIG%" ^
	-DHG_BUILD_HG_JS=ON ^
	-DHG_BUILD_HG_LUA=OFF ^
	-DHG_BUILD_HG_SQUIRREL=OFF ^
	-DHG_BUILD_HG_PYTHON=OFF ^
	-DHG_BUILD_HG_GO=OFF ^
	-DHG_BUILD_ASSETC=ON ^
	-DHG_BUILD_ASSIMP_CONVERTER=OFF ^
	-DHG_BUILD_FBX_CONVERTER=OFF ^
	-DHG_BUILD_GLTF_IMPORTER=OFF ^
	-DHG_BUILD_GLTF_EXPORTER=OFF ^
	-DHG_BUILD_LEGACY_ARCHIVE=ON ^
	-DHG_ENABLE_RECAST_DETOUR_API=ON ^
	-DHG_ENABLE_XMP_AUDIO=ON ^
	-DHG_SCENE_PHYSICS_BACKEND=bullet
if errorlevel 1 exit /b !errorlevel!

echo [2/3] Build HarfangJs + AssetC (Bullet, %CONFIG%)...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target HarfangJs assetc audio_xmp recastc legacy_archive bulletc --parallel %BUILD_JOBS%
if errorlevel 1 exit /b !errorlevel!

echo [3/3] Installation du runtime et des outils...
for %%C in (quickjs assetc legacy_archive) do (
	cmake --install "%BUILD_DIR%" --config "%CONFIG%" --component %%C
	if errorlevel 1 exit /b !errorlevel!
)

for %%F in (hgjs\hgjs.exe hgjs\lua54.dll hgjs\glfw3.dll hgjs\audio_xmp.dll assetc\assetc.exe) do (
	if not exist "%INSTALL_DIR%\%%F" (
		echo Echec: package incomplet, "%INSTALL_DIR%\%%F" absent.
		exit /b 1
	)
)

echo.
echo HarfangJs + AssetC rebuild ok (Bullet, %CONFIG%).
echo Runtime: "%INSTALL_DIR%\hgjs\hgjs.exe"
exit /b 0
