@echo off
rem 草原のステージを作り直す。Blender をウィンドウなしで動かし、generate_stage.py を実行する。
rem Blender の場所が違う PC では、環境変数 BLENDER_EXE に blender.exe のパスを入れてから実行する。
rem 確かめ用の画像を描かないときは: generate.cmd --no-preview
setlocal
if "%BLENDER_EXE%"=="" set "BLENDER_EXE=D:\01_software\17_Blender\blender-5.2.3-windows-x64\blender.exe"
if not exist "%BLENDER_EXE%" (
    echo Blender が見つかりません: %BLENDER_EXE%
    echo 環境変数 BLENDER_EXE に blender.exe のパスを入れてください。
    exit /b 1
)
"%BLENDER_EXE%" --background --factory-startup --python "%~dp0generate_stage.py" -- %*
