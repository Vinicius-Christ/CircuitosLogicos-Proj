#include <Wire.h>
#include <Servo.h>
#include "SensorLaser.h"
#include "DisplayOled.h"
#include "MusicasBuzzer.h"

#define PINO_SERVO_PAN    2
#define PINO_SERVO_TILT   3
#define PINO_DHT11        4
#define PINO_JOY_X        A1
#define PINO_JOY_Y        A0
#define PINO_PIR          A3
#define PINO_BUZZER       1
#define ENDERECO_LASER    0x29

static const byte TECLADO_LINHAS[4]  PROGMEM = {8, 7, 6, 5};
static const byte TECLADO_COLUNAS[4] PROGMEM = {12, 11, 10, 9};
static const char TECLADO_MAPA[]     PROGMEM = "123A456B789C*0#D";

#define DIST_FUGA        100
#define MOVIMENTOS_FUGA  3
#define SERVO_PAN_MIN    10
#define SERVO_PAN_MAX    170
#define SERVO_TILT_MIN   20
#define SERVO_TILT_MAX   160
#define JOY_ZONA_MORTA   60
#define JOY_VEL_MAX      250
#define JOY_INVERTE_PAN  0
#define JOY_INVERTE_TILT 0

VL53L0X laser;
Servo servoPan;
Servo servoTilt;

char senhaDigitada[4];
byte qtdDigitos = 0;
bool acessoLiberado = false;

byte panGraus = 90, tiltGraus = 90;
int pan16avos = 90 * 16, tilt16avos = 90 * 16;
int joyCentroX = 512, joyCentroY = 512;
unsigned long tUltimoLoop = 0;

bool modoJogo = false;
bool modoMusica = false;

bool emFuga = false;
byte movimentosRestantes = 0;
unsigned long tProximoMovimento = 0;
unsigned long tProximaFuga = 0;

int dhtTemp10 = -1;
byte dhtUmidade = 255;
int laserDistancia = -1;
bool laserOk = false;
byte laserTentativas = 0;
byte laserFalhas = 0;
bool pirPresenca = false;
unsigned long tLaser = 0, tDht = 0;
unsigned long tProximaBuscaLaser = 3000;

byte musicaAtual = 255;
unsigned int musicaNota = 0;
unsigned long tProximaNota = 0;

byte ultPan, ultTilt, ultUmidade, ultPresenca, ultModo;
int ultTemp10, ultDistancia;

static char tecladoVarre() {
  char achada = 0;
  for (byte c = 0; c < 4; c++) {
    byte pinoColuna = pgm_read_byte(&TECLADO_COLUNAS[c]);
    pinMode(pinoColuna, OUTPUT);
    digitalWrite(pinoColuna, LOW);
    for (byte l = 0; l < 4; l++) {
      if (!digitalRead(pgm_read_byte(&TECLADO_LINHAS[l]))) achada = pgm_read_byte(&TECLADO_MAPA[l * 4 + c]);
    }
    digitalWrite(pinoColuna, HIGH);
    pinMode(pinoColuna, INPUT);
  }
  return achada;
}

static char tecladoLeTecla() {
  static unsigned long tVarredura = 0;
  static char anterior = 0;
  static bool jaEntregue = false;
  if (millis() - tVarredura < 10) return 0;
  tVarredura = millis();
  char atual = tecladoVarre();
  char resultado = 0;
  if (atual == 0) jaEntregue = false;
  else if (atual == anterior && !jaEntregue) { jaEntregue = true; resultado = atual; }
  anterior = atual;
  return resultado;
}

static void buzzerBipe() {
  digitalWrite(PINO_BUZZER, HIGH);
  delay(60);
  digitalWrite(PINO_BUZZER, LOW);
}

static void buzzerAbertura() {
  for (byte i = 0; i < sizeof(ABERTURA) / sizeof(Nota); i++) {
    unsigned int hz = pgm_read_word(&ABERTURA[i].hz);
    unsigned int ms = (unsigned int)pgm_read_byte(&ABERTURA[i].un) * 10;
    if (hz) tone(PINO_BUZZER, hz);
    delay(ms - ms / 8);
    noTone(PINO_BUZZER);
    digitalWrite(PINO_BUZZER, LOW);
    delay(ms / 8);
  }
}

static void musicaPara() {
  noTone(PINO_BUZZER);
  digitalWrite(PINO_BUZZER, LOW);
  musicaAtual = 255;
}

static void musicaInicia(byte numero) {
  noTone(PINO_BUZZER);
  musicaAtual = numero;
  musicaNota = 0;
  tProximaNota = 0;
}

static void musicaAtualiza(unsigned long agora) {
  if (musicaAtual == 255 || (long)(agora - tProximaNota) < 0) return;
  if (musicaNota >= pgm_read_word(&TAMANHOS[musicaAtual])) { musicaPara(); return; }
  const Nota* notas = (const Nota*)pgm_read_word(&MUSICAS[musicaAtual]);
  unsigned int hz = pgm_read_word(&notas[musicaNota].hz);
  unsigned int ms = (unsigned int)pgm_read_byte(&notas[musicaNota].un) * pgm_read_byte(&TEMPOS[musicaAtual]);
  if (hz) tone(PINO_BUZZER, hz, ms - ms / 8);
  tProximaNota = agora + ms;
  musicaNota++;
}

static bool dhtEsperaNivel(byte nivel, unsigned int limiteUs) {
  unsigned long t = micros();
  while (((PIND >> PD4) & 1) != nivel) {
    if (micros() - t > limiteUs) return false;
  }
  return true;
}

static bool dhtLe() {
  byte dados[5] = {0, 0, 0, 0, 0};
  pinMode(PINO_DHT11, OUTPUT);
  digitalWrite(PINO_DHT11, LOW);
  delay(20);
  pinMode(PINO_DHT11, INPUT_PULLUP);
  if (!dhtEsperaNivel(1, 100) || !dhtEsperaNivel(0, 120) ||
      !dhtEsperaNivel(1, 120) || !dhtEsperaNivel(0, 120)) {
    return false;
  }
  for (byte i = 0; i < 40; i++) {
    if (!dhtEsperaNivel(1, 100)) return false;
    unsigned long t = micros();
    if (!dhtEsperaNivel(0, 120)) return false;
    dados[i >> 3] <<= 1;
    if (micros() - t > 45) dados[i >> 3] |= 1;
  }
  if ((byte)(dados[0] + dados[1] + dados[2] + dados[3]) != dados[4]) return false;
  dhtUmidade = dados[0];
  dhtTemp10 = dados[2] * 10 + (dados[3] & 0x0F);
  return true;
}

static void laserProcura() {
  Wire.beginTransmission(ENDERECO_LASER);
  if (Wire.endTransmission() != 0) return;

  Wire.setClock(100000);
  laser.setTimeout(500);
  laserOk = laser.init();
  if (laserOk) {
    laser.startContinuous();
    laser.setTimeout(100);
    laserFalhas = 0;
  } else {
    laserTentativas++;
  }
  Wire.setClock(400000);
}

static void laserAtualiza(unsigned long agora) {
  Wire.setClock(100000);
  uint16_t mm = laser.readRangeContinuousMillimeters();
  Wire.setClock(400000);
  if (laser.timeoutOccurred() || mm == 65535) {
    if (++laserFalhas >= 3) {
      laserOk = false;
      laserDistancia = -1;
      laserTentativas++;
      tProximaBuscaLaser = agora + 10000;
    }
  } else {
    laserFalhas = 0;
    laserDistancia = (mm >= 8190) ? -1 : mm;
  }
}

static void oledTexto(byte coluna, byte pagina, const __FlashStringHelper* texto) {
  oledIni(coluna, pagina);
  oledEscP(texto);
  oledFim();
}

static void telaInicial() {
  oledLimpa();
  oledTexto(0, 0, F("Grupo:"));
  oledTexto(0, 1, F("Proj. Circuito"));
  oledTexto(0, 3, F("Integrantes:"));
  oledTexto(0, 5, F("Vinicius Christ"));
  oledTexto(0, 6, F("Guilherme Ladislau"));
}

static void telaDesenhaSenha() {
  oledIni(43, 4);
  for (byte i = 0; i < 4; i++) {
    oledChar(i < qtdDigitos ? '*' : '_');
    if (i < 3) oledChar(' ');
  }
  oledFim();
}

static void telaSenha() {
  oledLimpa();
  oledTexto(15, 1, F("Digite a senha:"));
  telaDesenhaSenha();
}

static void telaMensagem(const __FlashStringHelper* texto) {
  oledLimpa();
  oledTexto(15, 3, texto);
}

static void painelDesenhaFixo() {
  oledLimpa();
  oledTexto(0, 0, F("Pan : "));   oledTexto(54, 0, F(" graus"));
  oledTexto(0, 1, F("Tilt: "));   oledTexto(54, 1, F(" graus"));
  oledTexto(0, 2, F("Temp: "));   oledTexto(60, 2, F(" C"));
  oledTexto(0, 3, F("Umid: "));   oledTexto(54, 3, F(" %"));
  oledTexto(0, 4, F("Dist: "));
  oledTexto(0, 5, F("Pres: "));
  ultPan = ultTilt = 255;  ultUmidade = 254;  ultPresenca = 2;  ultModo = 99;
  ultTemp10 = -9999;  ultDistancia = -9999;
}

static void painelAtualiza() {
  if (panGraus != ultPan) {
    ultPan = panGraus;
    oledIni(36, 0);  oledEscNum(panGraus, 3);  oledFim();
  }
  if (tiltGraus != ultTilt) {
    ultTilt = tiltGraus;
    oledIni(36, 1);  oledEscNum(tiltGraus, 3);  oledFim();
  }
  if (dhtTemp10 != ultTemp10) {
    ultTemp10 = dhtTemp10;
    oledIni(36, 2);
    if (dhtTemp10 < 0) {
      oledEscP(F("--  "));
    } else {
      oledEscNum(dhtTemp10 / 10, 2);
      oledChar('.');
      oledChar('0' + dhtTemp10 % 10);
    }
    oledFim();
  }
  if (dhtUmidade != ultUmidade) {
    ultUmidade = dhtUmidade;
    oledIni(36, 3);
    if (dhtUmidade == 255) oledEscP(F("-- "));
    else oledEscNum(dhtUmidade, 3);
    oledFim();
  }

  int codigoDistancia = laserOk ? laserDistancia : -2;
  if (codigoDistancia != ultDistancia) {
    ultDistancia = codigoDistancia;
    oledIni(36, 4);
    byte n;
    if (codigoDistancia == -2) {
      n = oledEscP(F("SEM SENSOR"));
    } else if (codigoDistancia < 0) {
      n = oledEscP(F("fora de alcance"));
    } else {
      n = oledEscNum(codigoDistancia, 0);
      n += oledEscP(F(" mm"));
    }
    while (n++ < 15) oledChar(' ');
    oledFim();
  }

  byte presenca = pirPresenca;
  if (presenca != ultPresenca) {
    ultPresenca = presenca;
    oledIni(36, 5);
    oledEscP(presenca ? F("SIM") : F("NAO"));
    oledFim();
  }

  byte modo = modoJogo ? (emFuga ? 2 : 1) : (modoMusica ? (musicaAtual == 255 ? 3 : 4 + musicaAtual) : 0);
  if (modo != ultModo) {
    ultModo = modo;
    oledIni(0, 6);
    byte n;
    if (modo == 0)      n = oledEscP(F("A=jogo  D=musica"));
    else if (modo == 1) n = oledEscP(F("JOGO ON  (A sai)"));
    else if (modo == 2) n = oledEscP(F("FUGA!  (A sai)"));
    else if (modo == 3) {
      n = oledEscP(F("MUSICA: 1-"));
      oledChar('0' + N_MUSICAS);
      n += 1 + oledEscP(F("  *=para"));
    }
    else {
      n = oledEscP(F("TOCANDO "));
      oledChar('1' + (modo - 4));
      n += 1 + oledEscP(F("  *=para"));
    }
    while (n++ < 21) oledChar(' ');
    oledFim();
  }
}

static void fugaMovimento(unsigned long agora) {
  byte novo;
  if ((MOVIMENTOS_FUGA - movimentosRestantes) % 2 == 0) {
    do { novo = random(SERVO_PAN_MIN, SERVO_PAN_MAX + 1); } while (abs(novo - panGraus) < 40);
    panGraus = novo;
    pan16avos = panGraus * 16;
    servoPan.write(panGraus);
  } else {
    do { novo = random(SERVO_TILT_MIN, SERVO_TILT_MAX + 1); } while (abs(novo - tiltGraus) < 40);
    tiltGraus = novo;
    tilt16avos = tiltGraus * 16;
    servoTilt.write(tiltGraus);
  }

  tProximoMovimento = agora + random(250, 401);
  movimentosRestantes--;
}

static void fugaInicia(unsigned long agora) {
  movimentosRestantes = MOVIMENTOS_FUGA;
  emFuga = true;
  fugaMovimento(agora);
}

static void joyMoveEixo(int &angulo16avos, int leitura, int centro, bool inverte, byte angMin, byte angMax, unsigned long dt) {
  int desvio = leitura - centro;
  int modulo = abs(desvio);
  if (modulo <= JOY_ZONA_MORTA) return;
  long passo = (long)(modulo - JOY_ZONA_MORTA) * dt * (JOY_VEL_MAX * 16L) / (1000L * (512 - JOY_ZONA_MORTA));
  if ((desvio < 0) != inverte) passo = -passo;
  angulo16avos = constrain(angulo16avos + passo, angMin * 16, angMax * 16);
}

static void joyCalibra() {
  long somaX = 0, somaY = 0;
  for (byte i = 0; i < 16; i++) {
    somaX += analogRead(PINO_JOY_X);
    somaY += analogRead(PINO_JOY_Y);
    delay(2);
  }
  joyCentroX = somaX / 16;
  joyCentroY = somaY / 16;
  if (abs(joyCentroX - 512) > 150) joyCentroX = 512;
  if (abs(joyCentroY - 512) > 150) joyCentroY = 512;
}

void setup() {
  pinMode(PINO_BUZZER, OUTPUT);
  digitalWrite(PINO_BUZZER, LOW);
  pinMode(PINO_PIR, INPUT);
  pinMode(PINO_DHT11, INPUT_PULLUP);
  for (byte l = 0; l < 4; l++) pinMode(pgm_read_byte(&TECLADO_LINHAS[l]), INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(400000);

  Wire.setWireTimeout(25000, true);
  oledInicia();

  telaInicial();
  buzzerAbertura();
  delay(1000);
  laserProcura();
  telaSenha();
}

void loop() {
  char tecla = tecladoLeTecla();
  if (tecla && musicaAtual == 255) buzzerBipe();

  if (!acessoLiberado) {
    if (tecla >= '0' && tecla <= '9' && qtdDigitos < 4) {
      senhaDigitada[qtdDigitos++] = tecla;
      telaDesenhaSenha();
    } else if (tecla == '*' && qtdDigitos > 0) {
      qtdDigitos--;
      telaDesenhaSenha();
    }

    if (qtdDigitos == 4) {
      delay(300);
      bool senhaCerta = (memcmp_P(senhaDigitada, PSTR("1234"), 4) == 0);
      telaMensagem(senhaCerta ? F("ACESSO LIBERADO") : F("ACESSO NEGADO"));
      delay(1500);

      qtdDigitos = 0;
      if (senhaCerta) {
        acessoLiberado = true;
        servoPan.attach(PINO_SERVO_PAN);
        servoTilt.attach(PINO_SERVO_TILT);
        servoPan.write(panGraus);
        servoTilt.write(tiltGraus);
        joyCalibra();
        tUltimoLoop = millis();
        painelDesenhaFixo();
        tDht = millis() - 2000;
      } else {
        telaSenha();
      }
    }
    return;
  }

  unsigned long agora = millis();

  unsigned long dt = agora - tUltimoLoop;
  tUltimoLoop = agora;
  if (dt > 100) dt = 100;

  if (tecla == 'A') {
    modoJogo = !modoJogo;
    emFuga = false;
    if (modoJogo) {
      modoMusica = false;
      musicaPara();
      randomSeed(micros());
    }
  } else if (tecla == 'D') {
    modoMusica = !modoMusica;
    if (modoMusica) { modoJogo = false; emFuga = false; }
    else musicaPara();
  } else if (modoMusica) {
    if (tecla >= '1' && tecla < '1' + N_MUSICAS) musicaInicia(tecla - '1');
    else if (tecla == '*') musicaPara();
  }
  musicaAtualiza(agora);

  if (!modoJogo) {

    joyMoveEixo(pan16avos, analogRead(PINO_JOY_X), joyCentroX, JOY_INVERTE_PAN, SERVO_PAN_MIN, SERVO_PAN_MAX, dt);
    joyMoveEixo(tilt16avos, analogRead(PINO_JOY_Y), joyCentroY, JOY_INVERTE_TILT, SERVO_TILT_MIN, SERVO_TILT_MAX, dt);
    byte novoPan = pan16avos / 16, novoTilt = tilt16avos / 16;
    if (novoPan != panGraus)   { panGraus = novoPan;   servoPan.write(panGraus); }
    if (novoTilt != tiltGraus) { tiltGraus = novoTilt; servoTilt.write(tiltGraus); }
    delay(15);
  } else {

    if (emFuga && agora >= tProximoMovimento) {
      if (movimentosRestantes > 0) {
        fugaMovimento(agora);
      } else {
        emFuga = false;
        tProximaFuga = agora + 300;
      }
    }
    if (!emFuga && laserOk && laserDistancia >= 0 && laserDistancia <= DIST_FUGA && agora >= tProximaFuga) {
      fugaInicia(agora);
    }
    delay(15);
  }

  pirPresenca = (digitalRead(PINO_PIR) == HIGH);

  if (!laserOk && laserTentativas < 3 && (long)(agora - tProximaBuscaLaser) >= 0) {
    tProximaBuscaLaser = agora + 10000;
    laserProcura();
  }

  if (laserOk && agora - tLaser >= 200) {
    tLaser = agora;
    laserAtualiza(agora);
  }

  if (agora - tDht >= 2000) {
    tDht = agora;
    dhtLe();
  }

  painelAtualiza();
}
