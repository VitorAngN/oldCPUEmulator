@echo off
echo ========================================================
echo   Compilando Emulador CHIP-8 (API Win32)
echo ========================================================

g++ main.cpp Chip8.cpp -std=c++11 -lgdi32 -O3 -o chip8_emulator.exe

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCESSO] Compilacao finalizada!
    echo O arquivo "chip8_emulator.exe" foi gerado.
    echo.
    echo Para testar, execute: chip8_emulator.exe pong.ch8
    echo ou de dois cliques no arquivo chip8_emulator.exe 
    echo ^(ele tentara carregar pong.ch8 automaticamente se estiver na mesma pasta^)
    echo.
) else (
    echo.
    echo [ERRO] Falha na compilacao. Verifique os erros acima.
)
pause
