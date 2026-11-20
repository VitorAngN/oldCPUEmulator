// Testes do núcleo da CPU CHIP-8 (sem dependência de janela/Win32).
// Cada teste monta um pequeno programa em memória, executa N ciclos e
// verifica registradores, memória e tela.
#include "../emular8hd/Chip8.hpp"

#include <cstdio>
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

namespace {

int falhas = 0;
int verificacoes = 0;

void check(bool cond, const std::string& msg, const char* file, int line) {
    ++verificacoes;
    if (!cond) {
        ++falhas;
        std::printf("  FALHOU (%s:%d): %s\n", file, line, msg.c_str());
    }
}
#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)

// Monta um Chip8 com o programa (lista de opcodes de 16 bits) e roda `ciclos`.
Chip8 rodar(std::initializer_list<uint16_t> opcodes, int ciclos) {
    std::vector<uint8_t> bytes;
    for (uint16_t op : opcodes) {
        bytes.push_back(static_cast<uint8_t>(op >> 8));
        bytes.push_back(static_cast<uint8_t>(op & 0xFF));
    }
    Chip8 c;
    c.loadProgram(bytes.data(), bytes.size());
    for (int i = 0; i < ciclos; ++i) c.emulateCycle();
    return c;
}

int pixelsAcesos(const Chip8& c) {
    int total = 0;
    for (int i = 0; i < Chip8::kWidth * Chip8::kHeight; ++i) total += c.getDisplay()[i];
    return total;
}

struct Teste {
    const char* nome;
    std::function<void()> corpo;
};

const std::vector<Teste> testes = {
    {"6XNN/7XNN carregam e somam (sem carry)", [] {
         Chip8 c = rodar({0x6A10, 0x7A05, 0x6BFF, 0x7B02}, 4);
         CHECK(c.getV(0xA) == 0x15);
         CHECK(c.getV(0xB) == 0x01);  // estoura e volta sem mexer em VF
         CHECK(c.getV(0xF) == 0);
     }},
    {"8XY4 soma com carry em VF", [] {
         Chip8 c = rodar({0x60F0, 0x6120, 0x8014}, 3);
         CHECK(c.getV(0) == 0x10);
         CHECK(c.getV(0xF) == 1);
     }},
    {"8XY4 com X = F mantém a flag em VF", [] {
         // Bug corrigido: a flag era gravada antes do resultado e se perdia.
         Chip8 c = rodar({0x6FF0, 0x6120, 0x8F14}, 3);
         CHECK(c.getV(0xF) == 1);
     }},
    {"8XY5 subtração com flag de 'sem empréstimo'", [] {
         Chip8 c = rodar({0x6005, 0x6103, 0x8015}, 3);
         CHECK(c.getV(0) == 2);
         CHECK(c.getV(0xF) == 1);
         Chip8 d = rodar({0x6003, 0x6105, 0x8015}, 3);
         CHECK(d.getV(0) == 0xFE);
         CHECK(d.getV(0xF) == 0);
     }},
    {"8XY6/8XYE deslocamentos guardam o bit perdido em VF", [] {
         Chip8 c = rodar({0x6005, 0x8006}, 2);
         CHECK(c.getV(0) == 2);
         CHECK(c.getV(0xF) == 1);
         Chip8 d = rodar({0x6081, 0x800E}, 2);
         CHECK(d.getV(0) == 0x02);
         CHECK(d.getV(0xF) == 1);
     }},
    {"3XNN pula a próxima instrução quando igual", [] {
         Chip8 c = rodar({0x6007, 0x3007, 0x6199, 0x6211}, 3);
         CHECK(c.getV(1) == 0);     // instrução pulada
         CHECK(c.getV(2) == 0x11);
     }},
    {"1NNN salta para o endereço", [] {
         Chip8 c = rodar({0x1208}, 1);
         CHECK(c.getPC() == 0x208);
     }},
    {"2NNN/00EE chamam e retornam de sub-rotina", [] {
         // 0x200: CALL 0x206 | 0x202: LD V1,1 | 0x204: JP 0x204 | 0x206: LD V0,7 | 0x208: RET
         Chip8 c = rodar({0x2206, 0x6101, 0x1204, 0x6007, 0x00EE}, 4);
         CHECK(c.getV(0) == 7);
         CHECK(c.getV(1) == 1);
         CHECK(c.getPC() == 0x204);
     }},
    {"00EE com pilha vazia não corrompe a memória", [] {
         Chip8 c = rodar({0x00EE, 0x6042}, 2);
         CHECK(c.getV(0) == 0x42);
     }},
    {"FX33 grava o valor em BCD", [] {
         Chip8 c = rodar({0x60FE, 0xA300, 0xF033}, 3);  // 254
         CHECK(c.peek(0x300) == 2);
         CHECK(c.peek(0x301) == 5);
         CHECK(c.peek(0x302) == 4);
     }},
    {"FX55/FX65 salvam e carregam registradores", [] {
         Chip8 c = rodar({0x6011, 0x6122, 0x6233, 0xA400, 0xF255, 0x6000, 0x6100, 0xF265}, 8);
         CHECK(c.peek(0x400) == 0x11 && c.peek(0x402) == 0x33);
         CHECK(c.getV(0) == 0x11 && c.getV(1) == 0x22 && c.getV(2) == 0x33);
     }},
    {"FX29 aponta I para o caractere da fonte", [] {
         Chip8 c = rodar({0x600A, 0xF029}, 2);
         CHECK(c.getI() == 0x50 + 0xA * 5);
     }},
    {"DXYN desenha, detecta colisão e apaga com XOR", [] {
         // Desenha o '0' da fonte duas vezes no mesmo lugar.
         Chip8 c = rodar({0x6000, 0xF029, 0xD005}, 3);
         CHECK(pixelsAcesos(c) == 14);
         CHECK(c.getV(0xF) == 0);
         CHECK(c.drawFlag);
         c.emulateCycle();  // executa 0x0000 (nop) só para provar que a flag não some
         CHECK(c.drawFlag);
         Chip8 d = rodar({0x6000, 0xF029, 0xD005, 0xD005}, 4);
         CHECK(pixelsAcesos(d) == 0);
         CHECK(d.getV(0xF) == 1);
     }},
    {"DXYN recorta na borda direita (não vaza para a linha seguinte)", [] {
         // Sprite de 8 pixels (0xFF) em x = 60: só 4 pixels cabem.
         Chip8 c = rodar({0xA20A, 0x613C, 0x6200, 0xD121, 0x1208, 0xFF00}, 4);
         CHECK(pixelsAcesos(c) == 4);
         CHECK(c.getDisplay()[64] == 0);  // início da linha 1 continua apagado
     }},
    {"FX15/FX07 escrevem e leem o delay timer", [] {
         Chip8 c = rodar({0x6005, 0xF015}, 2);
         c.updateTimers();
         CHECK(c.getDelayTimer() == 4);
     }},
    {"FX0A espera uma tecla", [] {
         Chip8 c = rodar({0xF30A}, 3);
         CHECK(c.getPC() == 0x200);  // continua parado na instrução
         uint8_t teclas[16] = {};
         teclas[0x7] = 1;
         c.setKeys(teclas);
         c.emulateCycle();
         CHECK(c.getV(3) == 0x7);
         CHECK(c.getPC() == 0x202);
     }},
};

}  // namespace

int main() {
    for (const Teste& t : testes) {
        int antes = falhas;
        t.corpo();
        std::printf("%s %s\n", falhas == antes ? "[ok]   " : "[FALHA]", t.nome);
    }
    std::printf("\n%zu testes, %d verificações, %d falha(s)\n", testes.size(), verificacoes, falhas);
    return falhas == 0 ? 0 : 1;
}
