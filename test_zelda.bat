@echo off
cd /d "%~dp0"
if exist main.exe (
    echo Testing Zelda...
    start /B main.exe test-roms/lz.nes
    timeout /t 5 /nobreak >nul
    taskkill /im main.exe /f 2>nul
    echo Test complete.
) else (
    echo main.exe not found
)