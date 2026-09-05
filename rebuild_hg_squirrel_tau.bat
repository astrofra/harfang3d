@echo off
setlocal EnableExtensions EnableDelayedExpansion
set /a HG_BATCH_PAUSE_DEPTH+=1

for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"
if not defined BUILD_DIR set "BUILD_DIR=%WORK_DIR%\build\squirrel-cmake"
if not defined INSTALL_DIR set "INSTALL_DIR=%WORK_DIR%\install\squirrel_tau"

call "%~dp0rebuild_hg_squirrel_backend.bat" tau %*
set "EXITCODE=%ERRORLEVEL%"
if !HG_BATCH_PAUSE_DEPTH! LEQ 1 if not defined HG_BATCH_NO_PAUSE pause
exit /b %EXITCODE%
