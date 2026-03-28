#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <windows.h>

uint8_t fontset_padrao[80] = {
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
//Placa mãe
typedef struct{
    //memoria ram
    uint8_t memoriaRam[4096];
    //registradores
    uint8_t V[16];
    //registrador de index
    uint16_t I;
    //contador de programa
    uint16_t PC;//apontar para qual endereço da memória o processador deve ler a próxima instrução
    //tela
    uint8_t display[64*32];
    // A Pilha e o seu apontador 
    uint16_t pilha[16]; 
    uint16_t sp;
    // (Timers) 60 a 0
    uint8_t delayTimer; // Para o tempo do jogo
    uint8_t soundTimer; // Quando for > 0, o emulador deve emitir um som "Biiip!"

    // Um Teclado Hexadecimal (guarda 1 se a tecla estiver pressionada, 0 se não)
    uint8_t teclado[16];

}chip8;
int carregarROM(chip8 *emulador, const char *nome_arquivo) {
    
    //FILE tenta abrir o arquivo em modo de Leitura Binaria ("rb")
    FILE *arquivo_rom = fopen(nome_arquivo, "rb");
    if (arquivo_rom == NULL) {
        printf("Erro: Nao foi possivel abrir o arquivo %s!\n", nome_arquivo);
        return 0; 
    }

    // tamanho do arquivo
    fseek(arquivo_rom, 0, SEEK_END); // pular pro final do arquivo
    long tamanho = ftell(arquivo_rom); // Qual o byte atual
    rewind(arquivo_rom); // volta o arquivo de volta pro começo para podermos ler

    // verificar se o arquivo cabe na memoria do chip-8
    // A RAM total tem 4096. A ROM começa no 512. O espaço livre máximo é 3584.
    if (tamanho > (4096 - 512)) {
        printf("Erro: O arquivo e muito grande para a memoria do CHIP-8!\n");
        fclose(arquivo_rom);
        return 0;
    }

    // Usa o & para passar o endereço exato da gaveta 0x200
    fread(&emulador->memoriaRam[0x200], 1, tamanho, arquivo_rom);
    //finalizando o arquivo
    fclose(arquivo_rom);
    
    printf("ROM '%s' carregada com sucesso! (%ld bytes)\n", nome_arquivo, tamanho);
    return 1;
}


void ligarEmulador(chip8 *emulador){
    for(int i = 0; i < 4096; i++){
        emulador->memoriaRam[i] = 0;
    }
    for(int i = 0; i < 64*32; i++){
        emulador->display[i] = 0;
    }
    for(int i = 0; i < 16; i++){
        emulador->V[i] = 0;
    }
    emulador->I = 0;
    emulador->PC = 0x200;//o programa começa a ser carregado a partir do endereço 0x200
    //inicilizar placa de video zerando todos as memorias
    for (int i = 0; i < 80; i++) {
    emulador->memoriaRam[i] = fontset_padrao[i];
    }
    for(int i = 0; i < 16; i++){
        emulador->teclado[i] = 0;
    } 
    for(int i = 0; i < 16; i++){
        emulador->pilha[i] = 0;
    }
    emulador->sp = 0;
    emulador->delayTimer = 0;
    emulador->soundTimer = 0;
    printf("Emulador ligado e inicializado!\n");

}
void desenharTelaTerminal(chip8 *emulador) {
    printf("\033[H"); // Garante que o cursor está no topo
    printf("=== EMULADOR CHIP-8 ===================================\n");

    for (int y = 0; y < 32; y++) {
        char linha[65]; // 64 pixels + '\0'
        for (int x = 0; x < 64; x++) {
            // Se o pixel estiver aceso, usamos um caractere sólido (ASCII 219 é um bloco)
            // Se não tiver o caractere especial, use '#' ou 'O'
            linha[x] = (emulador->display[x + (y * 64)] == 1) ? '#' : ' ';
        }
        linha[64] = '\0'; // Finaliza a string da linha
        printf("%s\n", linha); // Imprime a linha inteira de uma vez
    }
    printf("===========================================================\n");
}
void cicloEmulador(chip8 *emulador) {
    //ler a metade e deslocar para a esquerda, depois ler a outra metade e faz um OR bit a bit para juntar as duas partes
    uint16_t opcode = (emulador->memoriaRam[emulador->PC] << 8) | emulador->memoriaRam[emulador->PC + 1];
    uint16_t familia = (opcode & 0xF000) >> 12;
    emulador->PC += 2; //incrementar o contador de programa para a próxima instrução
    //apenas para degub.
    //printf("\tTESTE Instrucao lida da memoria: 0x%XTESTE\n", opcode);
    

    // "Decode"
    uint8_t X = (opcode & 0x0F00) >> 8;  // Pega o 2º dígito
    uint8_t Y = (opcode & 0x00F0) >> 4;  // Pega o 3º dígito
    uint8_t N = (opcode & 0x000F);       // Pega o 4º dígito (não precisa empurrar)
    uint8_t NN = (opcode & 0x00FF);      // Pega os 2 últimos dígitos
    uint16_t NNN = (opcode & 0x0FFF);    // Pega os 3 últimos dígitos

    //executar
    //existem alguns printf q são para debugar e mostrar o que cada instrução faz, mas a maioria só mostra que tipo de instrução foi executada.
    switch (opcode & 0xF000) {
        
        case 0xA000:
            // alterar registrador x para o valor NNN.
            emulador->I = NNN;
            //printf("Executou instrucao A: Define I como 0x%X\n", NNN);
            break;

        case 0x1000:
            // Pule o PC para o endereco NNN
            emulador->PC = NNN;
            //printf("Executou instrucao : Jump para 0x%X\n", NNN);
            break;
        case 0x6000:
            // Coloque o valor NN dentro do registrador V[X]
            emulador->V[X] = NN;
            //printf("Executou instrucao : V[%d] recebe 0x%X\n", X, NN);
            break; 
        case 0x7000:
            //Some o valor NN no registrador V[X]
            emulador->V[X] += NN;
            //printf("Executou instrucao : V[%d] soma 0x%X\n", X, NN);
            break;
        case 0xD000: {
            // Pega as posições iniciais (usa o % para garantir que o desenho de a volta na tela se passar do limite)
            uint8_t x = emulador->V[X] % 64; 
            uint8_t y = emulador->V[Y] % 32; 
            uint8_t altura = N; // O N diz quantas linhas o desenho tem

            // Zera a flag de colisão antes de começar a desenhar
            emulador->V[0xF] = 0; 

            // Loop para cada linha do desenho (Eixo Y)
            for (int linha = 0; linha < altura; linha++) {
                
                // Vai na memória e pega 1 Byte (8 pixels de largura) que formam a linha do desenho
                uint8_t pixelByte = emulador->memoriaRam[emulador->I + linha];

                // Loop para cada um dos 8 pixels dessa linha (Eixo X)
                for (int coluna = 0; coluna < 8; coluna++) {
                    
                    // A mascara (0x80 >> coluna) escaneia bit por bit da esquerda para a direita
                    // Se o bit for 1, significa que o jogo quer acender esse pixel
                    if ((pixelByte & (0x80 >> coluna)) != 0) {
                        
                        // Transforma a coordenada 2D (x,y) no índice do nosso vetor 1D da tela
                        int posTela = (x + coluna) + ((y + linha) * 64);

                        // Só desenha se não tiver passado do tamanho máximo do nosso vetor de tela
                        if (posTela < (64 * 32)) {
                            
                            // SE o pixel na tela já for 1, colidiu! 
                            if (emulador->display[posTela] == 1) {
                                emulador->V[0xF] = 1; // Avisa o jogo que bateu
                            }
                            
                            // Inverte o pixel na tela (0 vira 1, 1 vira 0)
                            emulador->display[posTela] ^= 1;
                        }
                    }
                }
            }
            //printf("Executou instrucao: Desenhou na tela!\n");
            printf("\033[H"); // Limpa o terminal do Windows para dar sensação de que tem "frames"
            desenharTelaTerminal(emulador);
            break;
        }
        case 0x3000:
            if (emulador->V[X] == NN) {
                emulador->PC += 2; // Pula a próxima instrução!
            }
            //printf("Executou instrucao: Skip se V[%d] == 0x%X\n", X, NN);
            break;
        case 0x4000:
            if (emulador->V[X] != NN) {
                emulador->PC += 2; // Pula a proxima instrução!
            }
           // printf("Executou instrucao : Skip se V[%d] != 0x%X\n", X, NN);
            break;
        case 0x5000:
            if (emulador->V[X] == emulador->V[Y]) {
                emulador->PC += 2;
            }
           // printf("Executou instrucao : Skip se V[%d] == V[%d]\n", X, Y);
            break;
        case 0x8000:
            // Analisa o ultimo (N) para saber qual é a conta
            switch (opcode & 0x000F) {
                case 0x0: // 8XY0: Atribuição (Vx = Vy)
                    emulador->V[X] = emulador->V[Y];
                    break;
                case 0x1: // 8XY1: OR Bit a bit (Vx = Vx | Vy)
                    emulador->V[X] |= emulador->V[Y];
                    break;
                case 0x2: // 8XY2: AND Bit a bit (Vx = Vx & Vy)
                    emulador->V[X] &= emulador->V[Y];
                    break;
                case 0x3: // 8XY3: XOR Bit a bit (Vx = Vx ^ Vy)
                    emulador->V[X] ^= emulador->V[Y];
                    break;
                case 0x4: { // 8XY4: SOMA com verificação de limite (Carry)
                    // Se a soma passar de 255 (o limite de 8 bits), ativamos a flag V[F]
                    uint16_t soma = emulador->V[X] + emulador->V[Y];
                    emulador->V[X] = soma & 0xFF; // Guarda só os 8 bits
                    emulador->V[0xF] = (soma > 255) ? 1 : 0;
                    break;
                }
                case 0x5: // 8XY5: SUBTRAÇÃO (Vx = Vx - Vy)
                    // Se X for maior que Y, não vai dar número negativo, então V[F] = 1
                    emulador->V[0xF] = (emulador->V[X] >= emulador->V[Y]) ? 1 : 0;
                    emulador->V[X] -= emulador->V[Y];
                    break;
                case 0x6: // 8XY6: SHIFT RIGHT (Desloca os bits de Vx para a direita)
                    emulador->V[0xF] = emulador->V[X] & 0x1; // Salva o bit que vai cair fora
                    emulador->V[X] >>= 1;
                    break;
                case 0x7: // 8XY7: SUBTRAÇÃO INVERTIDA (Vx = Vy - Vx)
                    emulador->V[0xF] = (emulador->V[Y] >= emulador->V[X]) ? 1 : 0;
                    emulador->V[X] = emulador->V[Y] - emulador->V[X];
                    break;
                case 0xE: // 8XYE: SHIFT LEFT (Desloca os bits de Vx para a esquerda)
                    emulador->V[0xF] = (emulador->V[X] & 0x80) >> 7; // Salva o bit mais alto
                    emulador->V[X] <<= 1;
                    break;
            }
            //printf("Executou instrucao : Operacao matematica!\n");
            break;
        
        case 0x9000:
            // 9XY0: Skip se Vx != Vy
            if (emulador->V[X] != emulador->V[Y]) {
                emulador->PC += 2;
            }
            //printf("Executou instrucao : Skip se V[%d] != V[%d]\n", X, Y);
            break;

        case 0xC000:
            // CXNN: Numero Aleatório (Gera um numero randomico e faz um AND com NN)
            emulador->V[X] = (rand() % 256) & NN;
            //printf("Executou instrucao : V[%d] recebe Aleatorio & 0x%X\n", X, NN);
            break;
        case 0x0000:
            if (opcode == 0x00E0) {
                // 00E0: Limpa o ecrã inteiro (zera o display)
                for (int i = 0; i < 64 * 32; i++) {
                    emulador->display[i] = 0;
                }
                printf("Executou 00E0: Ecrã limpo!\n");
            } 
            else if (opcode == 0x00EE) {
                // 00EE: Retorno de Subrotina (como o 'return' do C)
                emulador->sp--; // Volta um passo na pilha
                emulador->PC = emulador->pilha[emulador->sp]; // Recupera de onde parou
                //printf("Executou 00EE: Retornou da funcao para 0x%X\n", emulador->PC);
            }
            break;

        case 0x2000:
            // 2NNN: Chama uma Subrotina 
            // Guarda o PC atual na pilha para saber para onde voltar depois
            emulador->pilha[emulador->sp] = emulador->PC;
            emulador->sp++;
            emulador->PC = NNN; // Salta para a função
           // printf("Executou 2000: Chamou subrotina em 0x%X\n", NNN);
            break;
        case 0xE000:
            if (NN == 0x9E) {
                // EX9E: Salta a próxima instrução SE a tecla guardada em V[X] estiver pressionada
                if (emulador->teclado[emulador->V[X]] == 1) {
                    emulador->PC += 2;
                }
            } 
            else if (NN == 0xA1) {
                // EXA1: Salta a próxima instrução SE a tecla NÃO estiver pressionada
                if (emulador->teclado[emulador->V[X]] == 0) {
                    emulador->PC += 2;
                }
            }
            break;
        case 0xF000:
            switch (NN) {
                case 0x07: // Fx07: Lê o cronómetro do jogo para V[X]
                    emulador->V[X] = emulador->delayTimer;
                    break;
                case 0x15: // Fx15: Define o cronómetro do jogo com o valor de V[X]
                    emulador->delayTimer = emulador->V[X];
                    break;
                case 0x18: // Fx18: Define o cronómetro de som com o valor de V[X]
                    emulador->soundTimer = emulador->V[X];
                    break;
                case 0x1E: // Fx1E: Soma V[X] ao registo de Índice I
                    emulador->I += emulador->V[X];
                    break;
                case 0x29: // Fx29: Aponta o I para o sprite da Fonte (números de 0 a F)
                    // Como cada número tem 5 linhas de altura, multiplicamos por 5
                    emulador->I = emulador->V[X] * 5;
                    break;
                case 0x33: // Fx33: BCD (Binary-Coded Decimal)
                    // Pega num número (ex: 323) e separa em centenas (3), dezenas (2) e unidades (3)
                    emulador->memoriaRam[emulador->I]     = emulador->V[X] / 100;
                    emulador->memoriaRam[emulador->I + 1] = (emulador->V[X] / 10) % 10;
                    emulador->memoriaRam[emulador->I + 2] = emulador->V[X] % 10;
                    break;
                case 0x55: // Fx55: Guarda os registos V[0] até V[X] na Memória RAM
                    for (int i = 0; i <= X; i++) {
                        emulador->memoriaRam[emulador->I + i] = emulador->V[i];
                    }
                    break;
                case 0x65: // Fx65: Lê a Memória RAM e preenche os registos V[0] até V[X]
                    for (int i = 0; i <= X; i++) {
                        emulador->V[i] = emulador->memoriaRam[emulador->I + i];
                    }
                    break;
            }
            break;
        

        default:
            printf("Opcode desconhecido ou nao implementado: 0x%X\n", opcode);
            break;
    }
    // pequeno relogio para que os timers diminuem a 60Hz se forem maiores que zero
    if (emulador->delayTimer > 0) {
        emulador->delayTimer--;
    }
    if (emulador->soundTimer > 0) {
        emulador->soundTimer--;
        if (emulador->soundTimer == 0) {
            // Emulando o bip de hardware com um caractere especial o (Bell)
            printf("\a"); 
        }
    }
}
void atualizarTeclado(chip8 *emulador) {
    // Mapeamento classico: a tecla '1' do PC é o 0x1 do CHIP-8, 'Q' é o 0x4, etc.
    emulador->teclado[0x1] = (GetAsyncKeyState('1') & 0x8000) ? 1 : 0;
    emulador->teclado[0x2] = (GetAsyncKeyState('2') & 0x8000) ? 1 : 0;
    emulador->teclado[0x3] = (GetAsyncKeyState('3') & 0x8000) ? 1 : 0;
    emulador->teclado[0xC] = (GetAsyncKeyState('4') & 0x8000) ? 1 : 0;

    emulador->teclado[0x4] = (GetAsyncKeyState('Q') & 0x8000) ? 1 : 0;
    emulador->teclado[0x5] = (GetAsyncKeyState('W') & 0x8000) ? 1 : 0;
    emulador->teclado[0x6] = (GetAsyncKeyState('E') & 0x8000) ? 1 : 0;
    emulador->teclado[0xD] = (GetAsyncKeyState('R') & 0x8000) ? 1 : 0;

    emulador->teclado[0x7] = (GetAsyncKeyState('A') & 0x8000) ? 1 : 0;
    emulador->teclado[0x8] = (GetAsyncKeyState('S') & 0x8000) ? 1 : 0;
    emulador->teclado[0x9] = (GetAsyncKeyState('9') & 0x8000) ? 1 : 0; 
    emulador->teclado[0xE] = (GetAsyncKeyState('F') & 0x8000) ? 1 : 0;

    emulador->teclado[0xA] = (GetAsyncKeyState('Z') & 0x8000) ? 1 : 0;
    emulador->teclado[0x0] = (GetAsyncKeyState('X') & 0x8000) ? 1 : 0;
    emulador->teclado[0xB] = (GetAsyncKeyState('C') & 0x8000) ? 1 : 0;
    emulador->teclado[0xF] = (GetAsyncKeyState('V') & 0x8000) ? 1 : 0;
}


int main(){
    chip8 *emulador = (chip8 *)malloc(sizeof(chip8));
    srand(time(NULL)); // sementes aleatorias baseadas no tempo atual.
    if(emulador == NULL){
        printf("Erro ao alocar memória para o emulador\n");
        return 1;
    }
    ligarEmulador(emulador);


    if (carregarROM(emulador, "Tetris.ch8") == 0) {
            // Se erro, limpa a memoria e finaliza o programa.
            free(emulador);
            return 1; 
    }
while (1) {
        atualizarTeclado(emulador);
        cicloEmulador(emulador);
        Sleep(2); // Freiar a execução para não consumir 100% da CPU (ajustavel)
    }






    printf("Emulador desligando...\n");
    free(emulador);
    printf("Emulador desligado!\n");

    return 0;
}