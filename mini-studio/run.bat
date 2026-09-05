@echo off
setlocal EnableExtensions EnableDelayedExpansion
set /a HG_BATCH_PAUSE_DEPTH+=1
set "LUA_DIR=C:\works\dev\harfang\install\lua\hg_lua"
"%LUA_DIR%\lua.exe" "%~dp0main.lua" %*
set "EXITCODE=%ERRORLEVEL%"
if !HG_BATCH_PAUSE_DEPTH! LEQ 1 if not defined HG_BATCH_NO_PAUSE pause
exit /b %EXITCODE%
