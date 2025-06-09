@echo off
REM Este script compila el codigo C++ (imagenes.cpp) usando MSVC (Visual Studio)
REM y luego, opcionalmente, ejecuta el programa resultante (imagenes.exe).

REM --- CONFIGURACION ---
REM Asegurate de que la ruta a 'vcvarsall.bat' sea correcta para tu instalacion de Visual Studio.
REM Esta ruta puede variar si tienes una version diferente de VS o si la instalaste en otra ubicacion.
REM El argumento 'x64' es crucial para configurar el entorno de compilacion de 64 bits.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
REM ^^^^ ASEGURATE QUE ESTA RUTA ES LA CORRECTA, LA QUE ENCONTRASTE ^^

REM Define las rutas de include y lib para OpenCV y FreeType2 instalados via vcpkg.
REM AJUSTA ESTAS RUTAS SI SON DIFERENTES EN TU SISTEMA O SI CAMBIAS LA UBICACION DE VCPKG.
set VCPKG_INCLUDE_PATH="C:\Users\deivi\OneDrive\Desktop\mi-software\vcpkg\installed\x64-windows\include"
set VCPKG_LIB_PATH="C:\Users\deivi\OneDrive\Desktop\mi-software\vcpkg\installed\x64-windows\lib"

REM --- COMPILACION ---
echo Compilando imagenes.cpp...
rem ** Aquí estamos concatenando las rutas de include con las de VCPKG **
rem ** Añado /std:c++17 para habilitar el soporte de filesystem **
cl imagenes.cpp /EHsc /std:c++17 ^
    /I %VCPKG_INCLUDE_PATH% ^
    /I "%VCPKG_INCLUDE_PATH%\opencv4" ^
    /link /LIBPATH:%VCPKG_LIB_PATH% ^
    opencv_core4.lib opencv_imgcodecs4.lib opencv_highgui4.lib opencv_imgproc4.lib opencv_freetype4.lib ^
    /Fe:imagenes.exe

REM Comprobacion si la compilacion fue exitosa
IF %ERRORLEVEL% NEQ 0 (
    echo.
    echo Error durante la compilacion. Presiona cualquier tecla para salir.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo Compilacion exitosa: imagenes.exe generado.
echo.

REM --- EJECUCION (Opcional, para pruebas) ---
REM Si solo quieres compilar, puedes comentar las siguientes lineas anteponiendo REM.
REM echo Ejecutando imagenes.exe...
REM imagenes.exe
REM echo.
REM echo Ejecucion finalizada.

pause
