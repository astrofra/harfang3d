@echo off
setlocal EnableExtensions EnableDelayedExpansion
set /a HG_BATCH_PAUSE_DEPTH+=1

call :main %*
set "EXITCODE=%ERRORLEVEL%"
if !HG_BATCH_PAUSE_DEPTH! LEQ 1 if not defined HG_BATCH_NO_PAUSE pause
exit /b %EXITCODE%

:main
for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "WORK_DIR=%%~fI"

set "INSTALL_DIR=%WORK_DIR%\install"
set "ZIP_SCRIPT=%REPO_DIR%\zip_install.py"

if not exist "%INSTALL_DIR%\" (
	echo Dossier d'installation introuvable: "%INSTALL_DIR%"
	exit /b 1
)

if not exist "%ZIP_SCRIPT%" (
	echo Script Python introuvable: "%ZIP_SCRIPT%"
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

"%PYTHON_EXE%" "%ZIP_SCRIPT%" "%INSTALL_DIR%"
exit /b !errorlevel!
