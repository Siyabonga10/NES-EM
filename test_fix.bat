@echo off
cd /d "%~dp0"
if exist main.exe (
    start /B main.exe test-roms/smb.nes 2>smb_test.log
    timeout /t 2 /nobreak >nul
    taskkill /im main.exe /f 2>nul
    type smb_test.log
) else (
    echo main.exe not found
)