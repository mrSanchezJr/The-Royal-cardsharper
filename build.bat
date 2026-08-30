@echo off
set RAYLIB_PATH=C:\raylib\w64devkit
set XXD="C:\Program Files\Git\usr\bin\xxd.exe"

if exist TheRoyalCardshaper.exe del /f /q TheRoyalCardshaper.exe

echo Generating king sprite headers...
%XXD% -i src\characters\king_idle_state.png       > src\king_idle_state.h
%XXD% -i src\characters\king_angry_state.png      > src\king_angry_state.h
%XXD% -i src\characters\king_overwhelmed_state.png > src\king_overwhelmed_state.h
%XXD% -i src\characters\king_turned_head.png       > src\king_turned_head.h
%XXD% -i src\characters\wanderer_hooded.png        > src\wanderer_hooded.h
%XXD% -i src\characters\wanderer_idle.png          > src\wanderer_idle.h
%XXD% -i src\characters\lesb_final.png              > src\lesb_final.h
%XXD% -i src\characters\princess.png                > src\princess.h
echo Generating button sound header...
%XXD% -i src\audio\btn_press.MP3 > src\btn_press.h
echo Generating table sprite header...
%XXD% -i src\sprites\table.png > src\table_sprite.h
if errorlevel 1 (
    echo ERROR: Failed to generate sprite headers. Make sure Git for Windows is installed.
    exit /b 1
)

rem -----------------------------------------------------------------------
rem  Build a minimal raylib without rmodels and unused file formats.
rem  Uses raylib sources from C:\raylib\raylib\src
rem  NOTE: No -DEXTERNAL_CONFIG_FLAGS - config.h provides safe defaults
rem        (keeps PNG/TTF/WAV/MP3 loading enabled).
rem        We only override specific flags to 0 (unused modules/formats).
rem  No UPX needed - fits in 1.44MB natively (no AV false positives)
rem -----------------------------------------------------------------------
echo Building slim raylib (rmodels disabled)...
set RAYLIB_SRC=C:\raylib\raylib\src
set GCC=C:\raylib\w64devkit\bin\gcc.exe
set AR=C:\raylib\w64devkit\bin\ar.exe

rem Common flags for all raylib source files
set SLIM_CFLAGS=-Os -s -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -DSUPPORT_MODULE_RMODELS=0 -DSUPPORT_FILEFORMAT_BMP=0 -DSUPPORT_FILEFORMAT_DDS=0 -DSUPPORT_FILEFORMAT_OBJ=0 -DSUPPORT_FILEFORMAT_MTL=0 -DSUPPORT_FILEFORMAT_IQM=0 -DSUPPORT_FILEFORMAT_GLTF=0 -DSUPPORT_FILEFORMAT_VOX=0 -DSUPPORT_FILEFORMAT_M3D=0 -DSUPPORT_IMAGE_EXPORT=0 -DSUPPORT_IMAGE_GENERATION=0 -DSUPPORT_MESH_GENERATION=0 -DSUPPORT_FILEFORMAT_OGG=0 -DSUPPORT_FILEFORMAT_XM=0 -DSUPPORT_FILEFORMAT_MOD=0 -DSUPPORT_SCREEN_CAPTURE=0 -DSUPPORT_AUTOMATION_EVENTS=0 -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-stack-protector -fomit-frame-pointer -I%RAYLIB_SRC% -I%RAYLIB_SRC%\external\glfw\include

if not exist build mkdir build

%GCC% %SLIM_CFLAGS% -c %RAYLIB_SRC%\rcore.c     -o build\rcore.o
%GCC% %SLIM_CFLAGS% -c %RAYLIB_SRC%\rshapes.c   -o build\rshapes.o
%GCC% %SLIM_CFLAGS% -c %RAYLIB_SRC%\rtextures.c -o build\rtextures.o
%GCC% %SLIM_CFLAGS% -c %RAYLIB_SRC%\rtext.c     -o build\rtext.o
%GCC% %SLIM_CFLAGS% -c %RAYLIB_SRC%\raudio.c    -o build\raudio.o
%GCC% %SLIM_CFLAGS% -c %RAYLIB_SRC%\rglfw.c     -o build\rglfw.o
if errorlevel 1 (
    echo ERROR: Failed to compile slim raylib.
    exit /b 1
)
%AR% rcs build\libraylib_slim.a build\rcore.o build\rshapes.o build\rtextures.o build\rtext.o build\raudio.o build\rglfw.o
if errorlevel 1 (
    echo ERROR: Failed to archive slim raylib.
    exit /b 1
)
echo Slim raylib OK.

echo Compiling game...
gcc -o TheRoyalCardshaper.exe src\main.c src\game_logic.c src\render.c src\ai.c src\story.c src\save_system.c src\audio.c ^
    -I%RAYLIB_SRC% -I%RAYLIB_PATH%\include ^
    -Lbuild -lraylib_slim -lgdi32 -lwinmm -lm -lopengl32 ^
    -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 ^
    -mwindows -Os -s ^
    -fno-unwind-tables -fno-asynchronous-unwind-tables ^
    -fno-stack-protector -fomit-frame-pointer ^
    -static
if errorlevel 1 (
    echo ERROR: Compilation failed.
    exit /b 1
)

echo.
echo Done! No UPX needed - binary fits in 1.44MB natively.
dir TheRoyalCardshaper.exe