// Interface web do emulador: carrega o núcleo em WebAssembly, desenha a tela,
// lê o teclado e toca o bipe. Toda a emulação acontece dentro do .wasm.
import criarChip8 from './chip8.js';

const JOGOS = [
  { arquivo: 'pong.ch8', nome: 'Pong', dica: 'Jogador da esquerda: 1 sobe, Q desce.', teclas: [0x1, 0x4] },
  { arquivo: 'brix.ch8', nome: 'Brix', dica: 'Q move para a esquerda, E para a direita.', teclas: [0x4, 0x6] },
  { arquivo: 'tetris.ch8', nome: 'Tetris', dica: 'Q gira, W move para a esquerda, E para a direita, A desce.', teclas: [0x4, 0x5, 0x6, 0x7] },
  { arquivo: 'invaders.ch8', nome: 'Space Invaders', dica: 'W começa e atira, Q e E movem a nave.', teclas: [0x4, 0x5, 0x6] },
  { arquivo: 'ufo.ch8', nome: 'UFO', dica: 'Atire com Q (diagonal esquerda), W (para cima) ou E (diagonal direita).', teclas: [0x4, 0x5, 0x6] },
  { arquivo: 'wipeoff.ch8', nome: 'Wipe Off', dica: 'Q move para a esquerda, E para a direita.', teclas: [0x4, 0x6] },
];

// Layout do teclado hexadecimal do COSMAC VIP e a tecla correspondente no PC.
const LAYOUT = [0x1, 0x2, 0x3, 0xC, 0x4, 0x5, 0x6, 0xD, 0x7, 0x8, 0x9, 0xE, 0xA, 0x0, 0xB, 0xF];
const PC = ['1', '2', '3', '4', 'Q', 'W', 'E', 'R', 'A', 'S', 'D', 'F', 'Z', 'X', 'C', 'V'];
const DO_TECLADO = Object.fromEntries(PC.map((t, i) => [t.toLowerCase(), LAYOUT[i]]));

const $ = (id) => document.getElementById(id);
const canvas = $('tela');
const ctx = canvas.getContext('2d');
const imagem = ctx.createImageData(64, 32);
const buffer = document.createElement('canvas');
buffer.width = 64; buffer.height = 32;
const bctx = buffer.getContext('2d');

const wasm = await criarChip8({ locateFile: (f) => f });
const chip8 = {
  carregar: wasm.cwrap('chip8_carregar', 'number', ['number', 'number']),
  quadro: wasm.cwrap('chip8_quadro', null, ['number']),
  tecla: wasm.cwrap('chip8_tecla', null, ['number', 'number']),
  tela: wasm.cwrap('chip8_tela', 'number', []),
  som: wasm.cwrap('chip8_som', 'number', []),
};

let pausado = false;
let ciclosPorQuadro = 10;
let romAtual = null;
let teclasDoJogo = [];

// ---------- tela ----------
function desenhar() {
  const pixels = new Uint8Array(wasm.HEAPU8.buffer, chip8.tela(), 64 * 32);
  const liga = [226, 232, 240], desliga = [11, 14, 20];
  for (let i = 0; i < pixels.length; i++) {
    const c = pixels[i] ? liga : desliga;
    imagem.data.set([c[0], c[1], c[2], 255], i * 4);
  }
  bctx.putImageData(imagem, 0, 0);
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(buffer, 0, 0, canvas.width, canvas.height);
}

// ---------- som (bipe enquanto o sound timer > 0) ----------
let audio = null, osc = null, somLigado = false;
function atualizarSom() {
  const tocar = somLigado && !pausado && chip8.som();
  if (tocar && !osc) {
    audio ??= new AudioContext();
    osc = audio.createOscillator();
    const ganho = audio.createGain();
    osc.type = 'square'; osc.frequency.value = 440; ganho.gain.value = 0.05;
    osc.connect(ganho).connect(audio.destination);
    osc.start();
  } else if (!tocar && osc) {
    osc.stop(); osc = null;
  }
}
$('som').onclick = () => {
  somLigado = !somLigado;
  $('som').textContent = somLigado ? '🔊' : '🔇';
  audio?.resume();
};

// ---------- laço principal ----------
function laco() {
  if (!pausado && romAtual) {
    chip8.quadro(ciclosPorQuadro);
    desenhar();
  }
  atualizarSom();
  requestAnimationFrame(laco);
}

// ---------- carregar ROMs ----------
function carregarBytes(bytes, teclas = teclasDoJogo) {
  teclasDoJogo = teclas;
  const ptr = wasm._malloc(bytes.length);
  wasm.HEAPU8.set(bytes, ptr);
  const ok = chip8.carregar(ptr, bytes.length);
  wasm._free(ptr);
  if (!ok) { alert('Arquivo grande demais para a memória do CHIP-8 (máx. 3,5 KB).'); return; }
  romAtual = bytes;
  pausado = false;
  $('pausar').textContent = 'Pausar';
  marcarTeclas();
}
async function carregarJogo(i) {
  const jogo = JOGOS[i];
  $('jogo-nome').textContent = jogo.nome;
  $('jogo-dica').textContent = jogo.dica;
  const resp = await fetch(`roms/${jogo.arquivo}`);
  carregarBytes(new Uint8Array(await resp.arrayBuffer()), jogo.teclas);
}

$('rom').innerHTML = JOGOS.map((j, i) => `<option value="${i}">${j.nome}</option>`).join('');
$('rom').onchange = (e) => { carregarJogo(Number(e.target.value)); e.target.blur(); };
$('reiniciar').onclick = () => romAtual && carregarBytes(romAtual);
$('pausar').onclick = () => { pausado = !pausado; $('pausar').textContent = pausado ? 'Continuar' : 'Pausar'; };
$('abrir').onclick = () => $('arquivo').click();
$('arquivo').onchange = async (e) => {
  const arq = e.target.files[0];
  if (!arq) return;
  $('jogo-nome').textContent = arq.name;
  $('jogo-dica').textContent = 'ROM aberta do seu computador. Veja no teclado ao lado quais teclas ele usa.';
  carregarBytes(new Uint8Array(await arq.arrayBuffer()), []);
  e.target.value = '';
};
$('velocidade').oninput = (e) => {
  ciclosPorQuadro = Number(e.target.value);
  $('velocidade-txt').textContent = `${ciclosPorQuadro * 60} instruções/s`;
};

// ---------- teclado ----------
const botoes = {};
$('teclado').innerHTML = LAYOUT.map((k, i) =>
  `<button data-k="${k}">${k.toString(16).toUpperCase()}<small>${PC[i]}</small></button>`).join('');
$('teclado').querySelectorAll('button').forEach((b) => {
  const k = Number(b.dataset.k);
  botoes[k] = b;
  const apertar = (v) => (e) => { e.preventDefault(); chip8.tecla(k, v); b.classList.toggle('ativa', !!v); };
  b.addEventListener('pointerdown', apertar(1));
  ['pointerup', 'pointerleave', 'pointercancel'].forEach((ev) => b.addEventListener(ev, apertar(0)));
});
function marcarTeclas() {
  const usadas = new Set(teclasDoJogo);
  Object.entries(botoes).forEach(([k, b]) => b.classList.toggle('usada', usadas.has(Number(k))));
}
function teclaFisica(e, v) {
  const k = DO_TECLADO[e.key.toLowerCase()];
  if (k === undefined || e.ctrlKey || e.metaKey || e.target.tagName === 'INPUT') return;
  e.preventDefault();
  chip8.tecla(k, v);
  botoes[k].classList.toggle('ativa', !!v);
}
addEventListener('keydown', (e) => teclaFisica(e, 1));
addEventListener('keyup', (e) => teclaFisica(e, 0));

await carregarJogo(0);
requestAnimationFrame(laco);
