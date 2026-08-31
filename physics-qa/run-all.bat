@echo off
setlocal EnableExtensions EnableDelayedExpansion
set /a HG_BATCH_PAUSE_DEPTH+=1

call :main %*
set "EXITCODE=%ERRORLEVEL%"
if !HG_BATCH_PAUSE_DEPTH! LEQ 1 if not defined HG_BATCH_NO_PAUSE pause
exit /b %EXITCODE%

:main
pushd "%~dp0"
for %%F in (*.lua) do (
	echo.
	echo ==== Running %%~nxF ====
	call "%~dp0run-one.bat" "%%~nxF" || (
		popd
		exit /b 1
	)
)
popd

exit /b 0
