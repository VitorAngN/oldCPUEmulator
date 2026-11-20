// Teste rápido do build WebAssembly: carrega o Pong, roda 2 s de emulação e
// confere se algo foi desenhado na tela. Roda no CI antes de publicar o site.
import { readFileSync } from 'node:fs';
import criarChip8 from '../../_site/chip8.js';

const wasm = await criarChip8();
const rom = readFileSync(new URL('../../web/roms/pong.ch8', import.meta.url));
const ptr = wasm._malloc(rom.length);
wasm.HEAPU8.set(rom, ptr);
if (wasm._chip8_carregar(ptr, rom.length) !== 1) throw new Error('falhou ao carregar a ROM');
wasm._free(ptr);
for (let i = 0; i < 120; i++) wasm._chip8_quadro(10);
const tela = wasm.HEAPU8.subarray(wasm._chip8_tela(), wasm._chip8_tela() + 64 * 32);
const acesos = tela.reduce((s, p) => s + p, 0);
if (acesos < 10) throw new Error(`tela quase vazia (${acesos} pixels)`);
console.log(`ok: ${acesos} pixels acesos depois de 120 quadros`);
