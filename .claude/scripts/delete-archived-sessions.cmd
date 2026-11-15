@echo off
rem Delete archived Claude conversations on this PC and personal OneDrive, and record them so other PCs delete them too.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0claude-sessions.ps1" -Mode delete-archived
pause
