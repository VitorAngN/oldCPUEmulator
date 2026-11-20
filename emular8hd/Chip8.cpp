#include "Chip8.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <chrono>

const uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8() : randGen(std::chrono::system_clock::now().time_since_epoch().count()), randByte(0, 255) {
    // Reset da memoria e registradores
    pc = 0x200;
    I = 0;
    sp = 0;
    delayTimer = 0;
    soundTimer = 0;
    drawFlag = false;

    std::memset(memory, 0, sizeof(memory));
    std::memset(display, 0, sizeof(display));
    std::memset(V, 0, sizeof(V));
    std::memset(stack, 0, sizeof(stack));
    std::memset(keypad, 0, sizeof(keypad));

    // Carregar fonte na memoria (no endereço tradicional 0x50)
    for (int i = 0; i < 80; i++) {
        memory[0x50 + i] = fontset[i];
    }
}

bool Chip8::loadROM(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir a ROM " << filename << std::endl;
        return false;
    }

    std::streamoff size = file.tellg();
    if (size <= 0 || size > (4096 - 0x200)) {
        std::cerr << "Erro: ROM vazia ou muito grande!" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&memory[0x200]), size);
    std::cout << "ROM carregada com sucesso! Tamanho: " << size << " bytes." << std::endl;
    return true;
}

bool Chip8::loadProgram(const uint8_t* data, std::size_t size) {
    if (size == 0 || size > (4096 - 0x200)) return false;
    std::memcpy(&memory[0x200], data, size);
    return true;
}

void Chip8::setKeys(const uint8_t* keys) {
    for (int i = 0; i < 16; ++i) {
        keypad[i] = keys[i];
    }
}

void Chip8::updateTimers() {
    if (delayTimer > 0) --delayTimer;
    if (soundTimer > 0) --soundTimer;
}

void Chip8::emulateCycle() {
    // 1. Fetch do Opcode (Lê 2 bytes)
    uint16_t opcode = (memory[pc & 0xFFF] << 8) | memory[(pc + 1) & 0xFFF];
    pc = (pc + 2) & 0xFFF;

    // Variaveis de Decode
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    uint8_t N = (opcode & 0x000F);
    uint8_t NN = (opcode & 0x00FF);
    uint16_t NNN = (opcode & 0x0FFF);

    // drawFlag NÃO é zerado aqui: o loop principal roda vários ciclos por frame
    // e só limpa a flag depois de desenhar. Antes, um desenho no 1º ciclo era
    // "esquecido" pelos ciclos seguintes e o frame não aparecia.

    // 2. Decode & Execute
    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                // Clear display
                std::memset(display, 0, sizeof(display));
                drawFlag = true;
            } else if (opcode == 0x00EE) {
                // Return from subroutine (ignora retorno com pilha vazia)
                if (sp == 0) break;
                --sp;
                pc = stack[sp];
            }
            break;

        case 0x1000:
            // Jump to NNN
            pc = NNN;
            break;

        case 0x2000:
            // Call subroutine at NNN (pilha de 16 níveis: evita escrever fora do array)
            if (sp >= 16) break;
            stack[sp] = pc;
            ++sp;
            pc = NNN;
            break;

        case 0x3000:
            // Skip next if V[X] == NN
            if (V[X] == NN) pc += 2;
            break;

        case 0x4000:
            // Skip next if V[X] != NN
            if (V[X] != NN) pc += 2;
            break;

        case 0x5000:
            // Skip next if V[X] == V[Y]
            if (V[X] == V[Y]) pc += 2;
            break;

        case 0x6000:
            // Set V[X] = NN
            V[X] = NN;
            break;

        case 0x7000:
            // Add NN to V[X] (no carry flag)
            V[X] += NN;
            break;

        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000: V[X] = V[Y]; break;
                case 0x0001: V[X] |= V[Y]; break;
                case 0x0002: V[X] &= V[Y]; break;
                case 0x0003: V[X] ^= V[Y]; break;
                // Nas operações abaixo o resultado é gravado ANTES da flag em VF.
                // Assim, se X == F, VF termina com a flag (comportamento correto).
                case 0x0004: {
                    uint16_t sum = V[X] + V[Y];
                    V[X] = sum & 0xFF;
                    V[0xF] = (sum > 255) ? 1 : 0; // Carry flag
                    break;
                }
                case 0x0005: {
                    uint8_t flag = (V[X] >= V[Y]) ? 1 : 0; // No borrow flag
                    V[X] -= V[Y];
                    V[0xF] = flag;
                    break;
                }
                case 0x0006: {
                    uint8_t flag = V[X] & 0x1;
                    V[X] >>= 1;
                    V[0xF] = flag;
                    break;
                }
                case 0x0007: {
                    uint8_t flag = (V[Y] >= V[X]) ? 1 : 0;
                    V[X] = V[Y] - V[X];
                    V[0xF] = flag;
                    break;
                }
                case 0x000E: {
                    uint8_t flag = (V[X] & 0x80) >> 7;
                    V[X] <<= 1;
                    V[0xF] = flag;
                    break;
                }
            }
            break;

        case 0x9000:
            // Skip next if V[X] != V[Y]
            if (V[X] != V[Y]) pc += 2;
            break;

        case 0xA000:
            // Set I = NNN
            I = NNN;
            break;

        case 0xB000:
            // Jump to NNN + V[0]
            pc = (NNN + V[0]) & 0xFFF;
            break;

        case 0xC000:
            // Set V[X] = random byte & NN
            V[X] = randByte(randGen) & NN;
            break;

        case 0xD000: {
            // Desenha um sprite na tela (Wrap para V[X], V[Y])
            uint8_t x = V[X] % 64;
            uint8_t y = V[Y] % 32;
            uint8_t height = N;

            V[0xF] = 0; // Colision flag reset

            for (unsigned int row = 0; row < height && (y + row) < kHeight; ++row) {
                uint8_t spriteByte = memory[(I + row) & 0xFFF];

                // Recorta na borda direita: antes, os pixels que passavam da
                // coluna 63 "vazavam" para o começo da linha de baixo.
                for (unsigned int col = 0; col < 8 && (x + col) < kWidth; ++col) {
                    if ((spriteByte & (0x80 >> col)) != 0) {
                        int index = (x + col) + ((y + row) * kWidth);
                        if (display[index] == 1) {
                            V[0xF] = 1; // Colidiu
                        }
                        display[index] ^= 1;
                    }
                }
            }
            drawFlag = true;
            break;
        }

        case 0xE000:
            if (NN == 0x9E) {
                if (keypad[V[X] & 0xF] != 0) pc += 2;
            } else if (NN == 0xA1) {
                if (keypad[V[X] & 0xF] == 0) pc += 2;
            }
            break;

        case 0xF000:
            switch (NN) {
                case 0x07:
                    V[X] = delayTimer;
                    break;
                case 0x0A: {
                    // Wait for a key press
                    bool keyPressed = false;
                    for (int i = 0; i < 16; ++i) {
                        if (keypad[i] != 0) {
                            V[X] = i;
                            keyPressed = true;
                            break;
                        }
                    }
                    if (!keyPressed) {
                        pc -= 2; // Repete a instrução
                    }
                    break;
                }
                case 0x15:
                    delayTimer = V[X];
                    break;
                case 0x18:
                    soundTimer = V[X];
                    break;
                case 0x1E:
                    I += V[X];
                    break;
                case 0x29:
                    // I aponta para o caractere font (5 bytes de altura)
                    I = 0x50 + ((V[X] & 0xF) * 5);
                    break;
                case 0x33:
                    // Guarda representação BCD
                    memory[I & 0xFFF] = V[X] / 100;
                    memory[(I + 1) & 0xFFF] = (V[X] / 10) % 10;
                    memory[(I + 2) & 0xFFF] = V[X] % 10;
                    break;
                case 0x55:
                    for (int i = 0; i <= X; ++i) {
                        memory[(I + i) & 0xFFF] = V[i];
                    }
                    break;
                case 0x65:
                    for (int i = 0; i <= X; ++i) {
                        V[i] = memory[(I + i) & 0xFFF];
                    }
                    break;
            }
            break;

        default:
            std::cerr << "Opcode desconhecido ou nao suportado: 0x" << std::hex << opcode << std::endl;
            break;
    }
}
