@echo off
setlocal
set "LUA_DIR=C:\works\dev\harfang\install\lua\hg_lua"
"%LUA_DIR%\lua.exe" "%~dp0main.lua" %*
pause
