@echo off
REM Este script compila el código C++ (image_preprocessor.cpp) usando MSVC (Visual Studio)
REM y luego ejecuta el programa resultante (image_preprocessor.exe).

REM --- CONFIGURACIÓN DE RUTAS ---
REM IMPORTANTE: Ajusta estas rutas segun tu sistema.

REM 1. Ruta al script de entorno de Visual Studio 2022.
REM    Asegurate que esta ruta es la correcta para tu instalacion de VS.
set VSCMD_START_DIR=C:\Users\deivi\OneDrive\Desktop\Software Advanced
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM 2. Ruta base de tu instalacion de vcpkg (donde 'installed' esta dentro).
REM    AJUSTA ESTA RUTA si vcpkg no esta en esta ubicacion.
set VCPKG_ROOT="C:\Users\deivi\OneDrive\Desktop\mi-software\vcpkg"
set VCPKG_INCLUDE_PATH=%VCPKG_ROOT%\installed\x64-windows\include
set VCPKG_LIB_PATH=%VCPKG_ROOT%\installed\x64-windows\lib

REM --- CIERRE DE PROCESOS ANTERIORES ---
echo.
echo Cerrando instancias anteriores de image_preprocessor.exe (si las hay)...
taskkill /IM image_preprocessor.exe /F >NUL 2>&1
echo.

REM --- COMPILACIÓN ---
echo.
echo Compilando image_preprocessor.cpp...
rem Se añade la bandera /std:c++17 para habilitar las caracteristicas de C++17, como std::filesystem
cl image_preprocessor.cpp /EHsc /std:c++17 ^
    /I %VCPKG_INCLUDE_PATH% ^
    /I "%VCPKG_INCLUDE_PATH%\opencv4" ^
    /link ^
    /LIBPATH:%VCPKG_LIB_PATH% ^
    opencv_core4.lib ^
    opencv_imgcodecs4.lib ^
    opencv_highgui4.lib ^
    opencv_imgproc4.lib ^
    opencv_freetype4.lib ^
    /Fe:image_preprocessor.exe

REM Comprueba si la compilacion fue exitosa
IF %ERRORLEVEL% NEQ 0 (
    echo.
    echo Error durante la compilacion. Presiona cualquier tecla para salir.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo Compilacion exitosa: image_preprocessor.exe generado.
echo.

REM --- EJECUCIÓN ---
echo Ejecutando image_preprocessor.exe...
image_preprocessor.exe
echo.
echo Ejecucion finalizada.

pause
