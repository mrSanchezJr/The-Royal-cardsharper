@echo off
set RAYLIB_PATH=C:\raylib

if exist TheRoyalCardshaper.exe del /f /q TheRoyalCardshaper.exe

echo Compiling game...
gcc -o TheRoyalCardshaper.exe src\main.c src\game_logic.c src\render.c src\ai.c src\story.c src\save_system.c ^
    -I%RAYLIB_PATH%\include ^
    -L%RAYLIB_PATH%\lib ^
    -lraylib -lgdi32 -lwinmm -lm ^
    -mwindows -Os -s ^
    -static

if not exist upx.exe (
    echo Downloading UPX...
    powershell -Command "Invoke-WebRequest -Uri 'https://github.com/upx/upx/releases/download/v4.2.4/upx-4.2.4-win64.zip' -OutFile 'upx.zip'; Expand-Archive -Path 'upx.zip' -DestinationPath '.'; Move-Item -Path 'upx-4.2.4-win64\upx.exe' -Destination '.'; Remove-Item -Recurse -Force 'upx-4.2.4-win64', 'upx.zip'"
)

echo Compressing with UPX...
upx.exe --best --lzma TheRoyalCardshaper.exe

echo Done!
dir TheRoyalCardshaper.exe
