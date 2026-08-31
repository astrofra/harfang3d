@echo off
setlocal EnableExtensions EnableDelayedExpansion
set /a HG_BATCH_PAUSE_DEPTH+=1

call :main %*
set "EXITCODE=%ERRORLEVEL%"
if !HG_BATCH_PAUSE_DEPTH! LEQ 1 if not defined HG_BATCH_NO_PAUSE pause
exit /b %EXITCODE%

:main
if "%~2"=="" (
	echo Usage: dump-one.bat ^<bullet^|tau^> script.lua
	exit /b 1
)

set "BACKEND=%~1"
set "SCRIPT=%~2"
if /I not "%BACKEND%"=="bullet" if /I not "%BACKEND%"=="tau" (
	echo Unsupported backend: "%BACKEND%"
	exit /b 1
)

for %%I in ("%~dp0.") do set "QA_DIR=%%~fI"
for %%I in ("%QA_DIR%\..") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"
set "HG_LUA_DIR=%WORK_DIR%\install\%BACKEND%\hg_lua"

if not exist "%HG_LUA_DIR%\lua.exe" (
	echo Lua runtime not found: "%HG_LUA_DIR%\lua.exe"
	echo Rebuild/install the %BACKEND% HG Lua target first.
	exit /b 1
)

set "HG_PHYSICS_QA_MODE=dump_matrix"
set "HG_PHYSICS_QA_BACKEND=%BACKEND%"
call "%QA_DIR%\run-one.bat" "%SCRIPT%"
exit /b %ERRORLEVEL%
