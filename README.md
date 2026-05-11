# CHIP-8 CPU Emulator

<p>
  <img src="https://img.shields.io/badge/Status-Concluído-success?style=flat-square" alt="Status" />
  <img src="https://img.shields.io/badge/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white" alt="C++" />
  <img src="https://img.shields.io/badge/Win32_API-0078D4?style=flat-square&logo=windows&logoColor=white" alt="Win32 API" />
  <img src="https://img.shields.io/badge/Dependências_Externas-Zero-gray?style=flat-square" alt="Zero Deps" />
</p>

Emulador completo da arquitetura CHIP-8 implementado em C++ puro, com renderização gráfica via **Win32 API nativa** e sem nenhuma dependência externa (sem SDL, SFML ou OpenGL).

---

## Motivação

O CHIP-8 é a plataforma ideal para estudar como um processador real funciona: ele é simples o suficiente para ser implementado por completo, mas rico o suficiente para exigir a implementação de todos os componentes fundamentais — memória, registradores, stack de chamadas, timers e I/O.

---

## O que foi implementado

| Componente | Descrição |
|---|---|
| **Ciclo Fetch/Decode/Execute** | Loop principal que lê, decodifica e executa cada opcode de 2 bytes |
| **4KB de RAM simulada** | Memória endereçável de 0x000 a 0xFFF |
| **16 Registradores (V0–VF)** | Registradores de uso geral de 8 bits |
| **Stack de Chamadas (16 níveis)** | Controle de fluxo para chamadas e retornos de subrotinas |
| **Timers a 60Hz** | Timer de delay e timer de som decrementados na frequência correta |
| **Teclado Hexadecimal (0x0–0xF)** | Mapeado para teclado convencional |
| **Display 64×32** | Renderização de sprites via operação XOR, com detecção de colisão |
| **35 Opcodes** | Implementação completa de todos os opcodes do conjunto de instruções CHIP-8 |

---

## Arquitetura Interna

```
oldCPUEmulator/
├── chip8.h / chip8.cpp   # Núcleo do emulador (CPU, memória, timers)
├── main.cpp              # Loop principal e renderização via Win32 API
└── ROMs/                 # ROMs de teste (Pong, Tetris, etc.)
```

O emulador foi separado em duas camadas: o **núcleo (`chip8.cpp`)**, que é completamente independente de plataforma e contém toda a lógica de emulação, e o **host (`main.cpp`)**, que é responsável por renderização e captura de input usando a Win32 API.

---

## Como Compilar e Executar

Você precisa do **MSVC** (Visual Studio) ou **MinGW (g++)** no Windows.

```bash
# Compilar com g++ (MinGW)
g++ -o chip8.exe main.cpp chip8.cpp -lgdi32 -mwindows

# Executar com uma ROM
./chip8.exe ROMs/pong.ch8
```

---

## Referências

- [Cowgod's CHIP-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM) — especificação completa dos opcodes
- [Guide to making a CHIP-8 emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/) — guia de implementação

<img src="https://komarev.com/ghpvc/?username=VitorAngN-oldCPUEmulator" width="1" height="1" alt="" />
