@echo off
rem Remove the personal OneDrive key and rclone from this PC.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0claude-sessions.ps1" -Mode remove
pause
