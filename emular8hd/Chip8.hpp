#pragma once

#include <cstdint>
#include <string>
#include <random>

class Chip8 {
public:
    Chip8();
    
    // Inicializa o emulador e carrega a ROM
    bool loadROM(const std::string& filename);
    
    // Executa um ciclo da CPU (Fetch, Decode, Execute)
    void emulateCycle();
    
    // Atualiza os timers a 60Hz
    void updateTimers();

    // Acesso aos gráficos e estados para a UI
    const uint8_t* getDisplay() const { return display; }
    
    // Setter do estado do teclado (0-F) a partir da interface SDL
    void setKeys(const uint8_t* keys);

    // Flags para o sistema principal
    bool drawFlag; 
    bool soundTimerOn() const { return soundTimer > 0; }

private:
    uint8_t memory[4096];
    uint8_t V[16];          // Registradores
    uint16_t I;             // Registrador de índice
    uint16_t pc;            // Program Counter
    uint8_t display[64 * 32];
    uint16_t stack[16];
    uint16_t sp;
    uint8_t delayTimer;
    uint8_t soundTimer;
    uint8_t keypad[16];

    // Geração de números aleatórios de forma moderna
    std::mt19937 randGen;
    std::uniform_int_distribution<int> randByte;
};
