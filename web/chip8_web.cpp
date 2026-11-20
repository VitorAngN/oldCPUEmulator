// Ponte entre o núcleo da CPU (Chip8.cpp) e o navegador.
// Compilado com Emscripten para WebAssembly; a interface fica em site/.
// É o mesmo Chip8.cpp usado na versão Win32 e nos testes.
#include "Chip8.hpp"
#include <emscripten/emscripten.h>

static Chip8 cpu;
static uint8_t teclas[16] = {};

extern "C" {

// Reinicia a CPU e carrega o programa (bytes copiados para a memória do wasm pelo JS).
EMSCRIPTEN_KEEPALIVE int chip8_carregar(const uint8_t* dados, int tamanho) {
    cpu = Chip8();
    for (auto& t : teclas) t = 0;
    return cpu.loadProgram(dados, static_cast<std::size_t>(tamanho)) ? 1 : 0;
}

// Um quadro (1/60 s): roda N ciclos da CPU e atualiza os timers uma vez.
EMSCRIPTEN_KEEPALIVE void chip8_quadro(int ciclos) {
    cpu.setKeys(teclas);
    for (int i = 0; i < ciclos; ++i) cpu.emulateCycle();
    cpu.updateTimers();
}

EMSCRIPTEN_KEEPALIVE void chip8_tecla(int tecla, int pressionada) {
    if (tecla >= 0 && tecla < 16) teclas[tecla] = pressionada ? 1 : 0;
}

// Tela 64x32, um byte por pixel (0 ou 1).
EMSCRIPTEN_KEEPALIVE const uint8_t* chip8_tela() { return cpu.getDisplay(); }

EMSCRIPTEN_KEEPALIVE int chip8_som() { return cpu.soundTimerOn() ? 1 : 0; }

}
