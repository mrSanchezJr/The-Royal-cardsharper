@echo off
set RAYLIB_PATH=C:\raylib

gcc -o game.exe game.c ^
    -I%RAYLIB_PATH%\include ^
    -L%RAYLIB_PATH%\lib ^
    -lraylib -lgdi32 -lwinmm -lm ^
    -mwindows -Os -s ^
    -static-libgcc

upx --brute game.exe
echo Done!
dir game.exe
pause