# CHIP-8 CPU Emulator (C++)

![C++](https://img.shields.io/badge/Language-C++-00599C?style=for-the-badge&logo=c%2B%2B)
![Status](https://img.shields.io/badge/Status-Completed-success?style=for-the-badge)

A fully functional CHIP-8 CPU emulator developed entirely from scratch in C++. This project was built to deepen my understanding of computer architecture, low-level programming, and how software interacts with hardware components like memory, registers, and the CPU clock.

## Arquitetura e Componentes

O emulador reproduz o comportamento físico de um sistema da década de 70, implementando os seguintes componentes em software:

- **Memória RAM:** 4KB (4096 bytes) de memória simulada.
- **Registradores:** 16 registradores de propósito geral de 8-bits (`V0` a `VF`).
- **Registrador de Índice:** 1 registrador de 16-bits (`I`) para leitura de memória.
- **Program Counter (PC) & Stack:** Controle de fluxo de execução e pilha para suporte a sub-rotinas (16 níveis).
- **Timers:** Implementação de Delay Timer e Sound Timer operando a 60Hz.
- **Display & Graphics:** Matriz de vídeo de 64x32 pixels renderizada no terminal (via XOR sprites).
- **Input:** Mapeamento de teclado hexadecimal (16 teclas) capturado via API do Windows.

## O Ciclo da CPU (Fetch, Decode, Execute)

O coração deste emulador reside no seu laço principal, que obedece estritamente ao ciclo clássico de um processador:

1. **Fetch (Busca):** Lê o *opcode* (instrução de 16 bits) a partir do endereço de memória apontado pelo *Program Counter*.
2. **Decode (Decodificação):** Isola os *nibbles* (blocos de 4 bits) usando operações bit-a-bit (Bitwise AND/Shift) para determinar qual instrução o software deseja executar.
3. **Execute (Execução):** Baseado na instrução decodificada, realiza cálculos matemáticos (soma, subtração, bitwise AND/OR/XOR), manipulações de memória, saltos condicionais ou rotinas de desenho de pixels.

## Como Rodar

1. Clone o repositório:
```bash
git clone https://github.com/VitorAngN/oldCPUEmulator.git
```

2. Entre na pasta do código:
```bash
cd oldCPUEmulator/emular8hd
```

3. Compile o arquivo fonte usando G++ (MinGW no Windows):
```bash
g++ emulador.cpp -o emulador.exe
```

4. Execute o programa (certifique-se de que há uma rom como `pong.ch8` configurada no código):
```bash
./emulador.exe
```

## 👨‍💻 Autor

Desenvolvido por **João Vitor Angelim Nogueira**.  
Estudante de Engenharia da Computação buscando aprofundar-se desde o "escovador de bits" até arquiteturas distribuídas complexas.
