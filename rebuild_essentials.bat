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

echo Rebuild des composantes essentielles de HARFANG (%CONFIG%).
echo.

for %%S in (
	rebuild_assetc.bat
	rebuild_hg_ffmpeg.bat
	rebuild_hg_lua_bullet.bat
	rebuild_hg_python_bullet.bat
	rebuild_hg_js_bullet.bat
	rebuild_static_docs.bat
	rebuild_tools.bat
	rebuild_xm_player.bat
) do (
	echo ==== %%S ====
	call "%~dp0%%S" "%CONFIG%"
	if errorlevel 1 (
		set "BUILD_ERROR=!errorlevel!"
		echo Echec: %%S, code !BUILD_ERROR!. Rebuild interrompu.
		exit /b !BUILD_ERROR!
	)
	echo.
)

echo Rebuild des composantes essentielles termine (%CONFIG%).
exit /b 0
