#include <windows.h>
#include <iostream>
#include <chrono>
#include <string>
#include "Chip8.hpp"

// Configuração de Vídeo
const int VIDEO_WIDTH = 64;
const int VIDEO_HEIGHT = 32;
const int VIDEO_SCALE = 15;

Chip8 chip8;
bool isRunning = true;
uint32_t pixels[VIDEO_WIDTH * VIDEO_HEIGHT];
uint8_t keypad[16] = {};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
        case WM_DESTROY:
            isRunning = false;
            PostQuitMessage(0);
            return 0;

        case WM_KEYDOWN:
        case WM_KEYUP: {
            bool isPressed = (uMsg == WM_KEYDOWN);
            switch (wParam) {
                case '1': keypad[0x1] = isPressed; break;
                case '2': keypad[0x2] = isPressed; break;
                case '3': keypad[0x3] = isPressed; break;
                case '4': keypad[0xC] = isPressed; break;

                case 'Q': keypad[0x4] = isPressed; break;
                case 'W': keypad[0x5] = isPressed; break;
                case 'E': keypad[0x6] = isPressed; break;
                case 'R': keypad[0xD] = isPressed; break;

                case 'A': keypad[0x7] = isPressed; break;
                case 'S': keypad[0x8] = isPressed; break;
                case 'D': keypad[0x9] = isPressed; break;
                case 'F': keypad[0xE] = isPressed; break;

                case 'Z': keypad[0xA] = isPressed; break;
                case 'X': keypad[0x0] = isPressed; break;
                case 'C': keypad[0xB] = isPressed; break;
                case 'V': keypad[0xF] = isPressed; break;
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = VIDEO_WIDTH;
            bmi.bmiHeader.biHeight = -VIDEO_HEIGHT; // Top-down
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            // Transfere o array de pixels do C++ direto para a Placa de Video / Janela Windows
            StretchDIBits(
                hdc,
                0, 0, VIDEO_WIDTH * VIDEO_SCALE, VIDEO_HEIGHT * VIDEO_SCALE,
                0, 0, VIDEO_WIDTH, VIDEO_HEIGHT,
                pixels,
                &bmi,
                DIB_RGB_COLORS,
                SRCCOPY
            );

            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main(int argc, char* argv[]) {
    // Definir ROM Padrão caso o usuário simplesmente dê 2 cliques no EXE
    std::string romPath = "pong.ch8";
    if (argc > 1) {
        romPath = argv[1];
    }

    if (!chip8.loadROM(romPath)) {
        std::string erro = "Falha ao carregar a ROM: " + romPath + "\nTem certeza que o arquivo esta na mesma pasta?";
        MessageBox(NULL, erro.c_str(), "Erro CHIP-8", MB_OK | MB_ICONERROR);
        return 1;
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);
    const char CLASS_NAME[] = "Chip8WindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClass(&wc);

    // Ajustar o tamanho real da janela (levando em conta as bordas do Windows)
    RECT rect = { 0, 0, VIDEO_WIDTH * VIDEO_SCALE, VIDEO_HEIGHT * VIDEO_SCALE };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "Emulador CHIP-8 Profissional (API Win32 Nativa)",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, // Impede o redimensionamento livre
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);

    auto lastTimerUpdate = std::chrono::high_resolution_clock::now();
    auto timerInterval = std::chrono::nanoseconds(1000000000 / 60); // 60Hz = 1 bilhao de nanosegundos / 60

    MSG msg = {};
    while (isRunning) {
        // Tratar eventos de janela sem travar o loop
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        chip8.setKeys(keypad);
        
        // Reduzimos o multiplicador para 4 para ficar numa velocidade ideal (aprox 250-300Hz)
        // O Pong costuma ficar muito rapido se o multiplicador for 10.
        for(int i = 0; i < 4; i++) {
            chip8.emulateCycle();
        }

        auto now = std::chrono::high_resolution_clock::now();
        if (now - lastTimerUpdate >= timerInterval) {
            chip8.updateTimers();
            lastTimerUpdate = now;
        }

        if (chip8.soundTimerOn()) {
            std::cout << "\a";
        }

        if (chip8.drawFlag) {
            chip8.drawFlag = false;
            const uint8_t* display = chip8.getDisplay();
            for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; ++i) {
                // Win32 DIB_RGB_COLORS usa formato 0x00RRGGBB
                pixels[i] = (display[i] == 1) ? 0x00FFFFFF : 0x00000000;
            }
            // Força a janela a ser repintada com os novos pixels
            InvalidateRect(hwnd, NULL, FALSE);
        }

        // Freia o processador do CHIP-8 para rodar a uma velocidade realista
        Sleep(1);
    }

    return 0;
}
