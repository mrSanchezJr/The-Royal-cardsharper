@echo off
set RAYLIB_PATH=C:\raylib\w64devkit
set XXD="C:\Program Files\Git\usr\bin\xxd.exe"

if exist TheRoyalCardshaper.exe del /f /q TheRoyalCardshaper.exe

echo Generating king sprite headers...
%XXD% -i src\characters\king_idle_state.png       > src\king_idle_state.h
%XXD% -i src\characters\king_angry_state.png      > src\king_angry_state.h
%XXD% -i src\characters\king_overwhelmed_state.png > src\king_overwhelmed_state.h
%XXD% -i src\characters\king_turned_head.png       > src\king_turned_head.h
echo Generating table sprite header...
%XXD% -i src\sprites\table.png > src\table_sprite.h
if errorlevel 1 (
    echo ERROR: Failed to generate sprite headers. Make sure Git for Windows is installed.
    exit /b 1
)

echo Compiling game...
gcc -o TheRoyalCardshaper.exe src\main.c src\game_logic.c src\render.c src\ai.c src\story.c src\save_system.c src\audio.c ^
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
