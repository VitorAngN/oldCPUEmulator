# Emulador CHIP-8 Profissional (C++ & Win32 API)

[![en](https://img.shields.io/badge/lang-en-red.svg)](README-en.md)
[![pt-br](https://img.shields.io/badge/lang-pt--br-green.svg)](README.md)

![C++](https://img.shields.io/badge/Language-C++-00599C?style=for-the-badge&logo=c%2B%2B)
![Win32 API](https://img.shields.io/badge/Graphics-Win32_API-0078D7?style=for-the-badge&logo=windows)
![Status](https://img.shields.io/badge/Status-Completed-success?style=for-the-badge)

Um emulador completo da CPU CHIP-8 (sistema da década de 70) desenvolvido do zero em C++ moderno. 
Este projeto foi construído para demonstrar profundidade técnica em engenharia de software, saindo do nível de scripting procedural para uma arquitetura orientada a objetos (OOP) totalmente desacoplada.

**O diferencial técnico:** A renderização gráfica e o input não utilizam bibliotecas prontas como SDL2 ou SFML. O emulador "conversa" diretamente com o sistema operacional através da **Win32 API nativa** (`<windows.h>`), desenhando pixels diretamente na placa de vídeo, garantindo zero dependências externas para compilação.

<p align="center">
  <img src="rodando.png" alt="Emulador CHIP-8 executando Pong" width="600"/>
  <br/>
  <em>Emulador CHIP-8 executando Pong via Win32 API nativa</em>
</p>

## 🧠 Arquitetura e Engenharia

O projeto foi dividido em dois domínios principais (desacoplamento):

1. **Domínio da CPU (`Chip8.cpp` & `Chip8.hpp`)**: 
   - Focado exclusivamente em emular os barramentos de hardware. Nenhuma linha de código aqui sabe da existência de uma janela ou teclado físico.
   - **Memória RAM:** 4KB (4096 bytes).
   - **Registradores:** 16 registradores de uso geral (V0 a VF) + Registrador de Índice de 16-bits (I).
   - **Timers:** Sistema de interrupção simulado para *Delay* e *Sound* a 60Hz.

2. **Domínio de Interface (`main.cpp`)**:
   - Integração com o SO via Win32 API.
   - Criação da janela via `CreateWindowEx`.
   - Processamento contínuo da fila de mensagens do Windows (`PeekMessage` e `DispatchMessage`).
   - Sincronização de clock: A CPU roda a aproximadamente 300Hz, enquanto o desenho na tela e os timers são sincronizados a 60Hz.

## Como Compilar e Rodar

Não é necessário instalar ferramentas complexas ou o CMake. Basta ter o compilador G++ (MinGW) no seu Windows.

1. Clone o repositório:
```bash
git clone https://github.com/VitorAngN/oldCPUEmulator.git
cd oldCPUEmulator/emular8hd
```

2. Compile usando o script otimizado que já lida com a linkagem da GDI32:
```bash
# Ou simplesmente dê dois cliques no arquivo build.bat pelo Windows Explorer
.\build.bat
```

3. Execute passando a ROM desejada como argumento:
```bash
chip8_emulator.exe pong.ch8
```
*(Se você não passar nenhum argumento, ele tentará carregar `pong.ch8` automaticamente).*

## Autor

Desenvolvido por **João Vitor Angelim Nogueira**.  
Estudante de Engenharia da Computação focado em Engenharia de Software, Backend e DevOps, buscando aprofundar-se desde o "escovador de bits" até arquiteturas distribuídas e infraestrutura robusta.
