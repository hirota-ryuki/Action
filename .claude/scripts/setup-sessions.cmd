@echo off
rem Set up the backup of Claude conversations to your personal OneDrive (once per PC).
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0claude-sessions.ps1" -Mode setup
pause
