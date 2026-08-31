@echo off
setlocal EnableExtensions

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
