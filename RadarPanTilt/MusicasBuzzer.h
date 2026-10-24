// Musicas do buzzer (tudo na flash).
// Cada musica e uma tabela de {frequencia em Hz, duracao em unidades}. A duracao de uma
// unidade, em ms, esta em TEMPOS. Frequencia 0 = pausa.
// As teclas 1 a 5 tocam as musicas abaixo; 6 a 9 vem do arquivo MusicasExtras.h, se existir.

#pragma once

#define NG3 196
#define NC4 262
#define ND4 294
#define NE4 330
#define NF4 349
#define NG4 392
#define NA4 440
#define NB4 494
#define NC5 523
#define ND5 587
#define NE5 659
#define NF5 698
#define NG5 784

struct Nota { uint16_t hz; uint8_t un; };

// 1 - Parabens pra Voce
static const Nota MUS1[] PROGMEM = {
  {NG4,1},{NG4,1},{NA4,2},{NG4,2},{NC5,2},{NB4,4},
  {NG4,1},{NG4,1},{NA4,2},{NG4,2},{ND5,2},{NC5,4},
  {NG4,1},{NG4,1},{NG5,2},{NE5,2},{NC5,2},{NB4,2},{NA4,2},
  {NF5,1},{NF5,1},{NE5,2},{NC5,2},{ND5,2},{NC5,4}
};

// 2 - Ode a Alegria
static const Nota MUS2[] PROGMEM = {
  {NE4,2},{NE4,2},{NF4,2},{NG4,2},{NG4,2},{NF4,2},{NE4,2},{ND4,2},
  {NC4,2},{NC4,2},{ND4,2},{NE4,2},{NE4,3},{ND4,1},{ND4,4},
  {NE4,2},{NE4,2},{NF4,2},{NG4,2},{NG4,2},{NF4,2},{NE4,2},{ND4,2},
  {NC4,2},{NC4,2},{ND4,2},{NE4,2},{ND4,3},{NC4,1},{NC4,4}
};

// 3 - Brilha Brilha Estrelinha
static const Nota MUS3[] PROGMEM = {
  {NC4,2},{NC4,2},{NG4,2},{NG4,2},{NA4,2},{NA4,2},{NG4,4},
  {NF4,2},{NF4,2},{NE4,2},{NE4,2},{ND4,2},{ND4,2},{NC4,4},
  {NG4,2},{NG4,2},{NF4,2},{NF4,2},{NE4,2},{NE4,2},{ND4,4},
  {NG4,2},{NG4,2},{NF4,2},{NF4,2},{NE4,2},{NE4,2},{ND4,4},
  {NC4,2},{NC4,2},{NG4,2},{NG4,2},{NA4,2},{NA4,2},{NG4,4},
  {NF4,2},{NF4,2},{NE4,2},{NE4,2},{ND4,2},{ND4,2},{NC4,4}
};

// 4 - Jingle Bells (refrao)
static const Nota MUS4[] PROGMEM = {
  {NE4,2},{NE4,2},{NE4,4},{NE4,2},{NE4,2},{NE4,4},
  {NE4,2},{NG4,2},{NC4,3},{ND4,1},{NE4,8},
  {NF4,2},{NF4,2},{NF4,3},{NF4,1},{NF4,2},{NE4,2},{NE4,2},{NE4,1},{NE4,1},
  {NE4,2},{ND4,2},{ND4,2},{NE4,2},{ND4,4},{NG4,4}
};

// 5 - Frere Jacques
static const Nota MUS5[] PROGMEM = {
  {NC4,2},{ND4,2},{NE4,2},{NC4,2},{NC4,2},{ND4,2},{NE4,2},{NC4,2},
  {NE4,2},{NF4,2},{NG4,4},{NE4,2},{NF4,2},{NG4,4},
  {NG4,1},{NA4,1},{NG4,1},{NF4,1},{NE4,2},{NC4,2},{NG4,1},{NA4,1},{NG4,1},{NF4,1},{NE4,2},{NC4,2},
  {NC4,2},{NG3,2},{NC4,4},{NC4,2},{NG3,2},{NC4,4}
};

// Som de abertura ao ligar (fanfarra original, estilo videogame). Unidade = 10 ms.
static const Nota ABERTURA[] PROGMEM = {
  {523,9},{659,9},{784,9},{1047,9},{784,9},{1047,18},{0,6},
  {880,9},{1175,9},{1319,9},{1175,9},{1319,9},{1568,12},{0,4},
  {1047,9},{1319,9},{1568,9},{2093,35}
};

// 6 a 9 (opcionais): arquivo gerado por ferramentas/converte_musica_tone.py.
// Define N_EXTRAS (1 a 4), MUS6/TEMPO6 ... MUS9/TEMPO9.
#if defined(__has_include)
#if __has_include("MusicasExtras.h")
#include "MusicasExtras.h"
#else
#define N_EXTRAS 0
#endif
#else
#include "MusicasExtras.h"      // compilador antigo, sem __has_include: o arquivo precisa existir
#endif

#define N_MUSICAS (5 + N_EXTRAS)

static const Nota* const MUSICAS[N_MUSICAS] PROGMEM = {MUS1, MUS2, MUS3, MUS4, MUS5
#if N_EXTRAS >= 1
  , MUS6
#endif
#if N_EXTRAS >= 2
  , MUS7
#endif
#if N_EXTRAS >= 3
  , MUS8
#endif
#if N_EXTRAS >= 4
  , MUS9
#endif
};

static const uint16_t TAMANHOS[N_MUSICAS] PROGMEM = {
  sizeof(MUS1) / sizeof(Nota), sizeof(MUS2) / sizeof(Nota), sizeof(MUS3) / sizeof(Nota),
  sizeof(MUS4) / sizeof(Nota), sizeof(MUS5) / sizeof(Nota)
#if N_EXTRAS >= 1
  , sizeof(MUS6) / sizeof(Nota)
#endif
#if N_EXTRAS >= 2
  , sizeof(MUS7) / sizeof(Nota)
#endif
#if N_EXTRAS >= 3
  , sizeof(MUS8) / sizeof(Nota)
#endif
#if N_EXTRAS >= 4
  , sizeof(MUS9) / sizeof(Nota)
#endif
};

// Duracao de uma unidade, em ms, de cada musica.
static const byte TEMPOS[N_MUSICAS] PROGMEM = {230, 170, 180, 130, 200
#if N_EXTRAS >= 1
  , TEMPO6
#endif
#if N_EXTRAS >= 2
  , TEMPO7
#endif
#if N_EXTRAS >= 3
  , TEMPO8
#endif
#if N_EXTRAS >= 4
  , TEMPO9
#endif
};
