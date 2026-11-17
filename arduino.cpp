#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// DISPLAY LCD 16x2 I2C (Sensor Shield v5 -> Barramento I2C)
// SCL -> SCL (A5) | SDA -> SDA (A4) | VCC -> + | GND -> -
// Endereço mais comum: 0x27 (se não aparecer texto, tente 0x3F)
// ============================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ============================================================
// PINAGEM NO SENSOR SHIELD V5 - MÓDULO HW-095 (L298N)
// IMPORTANTE: Remova os jumpers de ENA e ENB no HW-095!
// ============================================================
// Motor Esquerdo
const int PIN_ENA = 11;  // S11 (PWM)
const int PIN_IN1 = 10;  // S10
const int PIN_IN2 = 9;   // S9

// Motor Direito
const int PIN_IN3 = 8;   // S8
const int PIN_IN4 = 7;   // S7
const int PIN_ENB = 6;   // S6 (PWM)

// ============================================================
// PINAGEM NO SENSOR SHIELD V5 - SENSOR HC-SR04
// GND -> G4 | VCC -> V5
// ============================================================
const int PIN_ECHO = 5;  // S5
const int PIN_TRIG = 4;  // S4

// ============================================================
// PARÂMETROS DE CALIBRAÇÃO
// ============================================================
const int DISTANCIA_OBSTACULO = 5;    // Para quando detectar obstáculo a <= 5 cm
const int DISTANCIA_ROTA_LIVRE = 18;  // Distância mínima (cm) para considerar rota livre

const int VELOCIDADE_FRENTE = 165;    // 0 a 255 (moderada para frear antes de bater)
const int VELOCIDADE_RE     = 160;
const int VELOCIDADE_GIRO   = 180;

const unsigned long TEMPO_RE_MS      = 450; // Duração da ré
const unsigned long TEMPO_GIRO_90_MS = 450; // Tempo aproximado para girar ~90 graus
const unsigned long INTERVALO_LCD_MS = 180; // Atualização do LCD enquanto anda

unsigned long ultimoUpdateLCD = 0;

// ============================================================
// PROTÓTIPOS
// ============================================================
long lerDistanciaCm();
void procurarMelhorRota();
void moverFrente(int velocidade);
void moverTras(int velocidade);
void girarDireita(int velocidade);
void girarEsquerda(int velocidade);
void pararMotores();
void exibirNoLCD(const char* linha1, const char* linha2);

void setup() {
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  pararMotores();

  lcd.init();
  lcd.backlight();
  exibirNoLCD("Carrinho Robo", "Inicializando...");
  delay(1500);
}

void loop() {
  long distancia = lerDistanciaCm();

  if (distancia <= DISTANCIA_OBSTACULO) {
    // 1. Para imediatamente ao encontrar obstáculo
    pararMotores();
    exibirNoLCD("Obstaculo", "encontrado!");
    delay(700);

    // 2. Dá uma ré
    exibirNoLCD("Obstaculo!", "Dando re...");
    moverTras(VELOCIDADE_RE);
    delay(TEMPO_RE_MS);
    pararMotores();
    delay(250);

    // 3. Procura ao redor um caminho sem obstáculos e muda de direção
    procurarMelhorRota();
  } else {
    // Caminho livre: anda para frente
    moverFrente(VELOCIDADE_FRENTE);

    unsigned long agora = millis();
    if (agora - ultimoUpdateLCD >= INTERVALO_LCD_MS) {
      ultimoUpdateLCD = agora;
      char linhaDist[17];
      if (distancia >= 400) {
        snprintf(linhaDist, sizeof(linhaDist), "Dist: >400 cm");
      } else {
        snprintf(linhaDist, sizeof(linhaDist), "Dist: %3ld cm", distancia);
      }
      exibirNoLCD("Andando...", linhaDist);
    }
  }
}

// ============================================================
// VARREDURA AO REDOR PARA ENCONTRAR CAMINHO SEM OBSTÁCULOS
// ============================================================
void procurarMelhorRota() {
  char buf[17];

  exibirNoLCD("Procurando", "nova rota...");
  delay(300);

  // 1) Gira para a direita para checar o lado direito
  girarDireita(VELOCIDADE_GIRO);
  delay(TEMPO_GIRO_90_MS);
  pararMotores();
  delay(200);

  long distDireita = lerDistanciaCm();
  snprintf(buf, sizeof(buf), "Direita: %3ld cm", distDireita);
  exibirNoLCD("Procurando rota", buf);
  delay(500);

  // 2) Gira para a esquerda (passando pelo centro até o lado esquerdo)
  girarEsquerda(VELOCIDADE_GIRO);
  delay(TEMPO_GIRO_90_MS * 2);
  pararMotores();
  delay(200);

  long distEsquerda = lerDistanciaCm();
  snprintf(buf, sizeof(buf), "Esquerda:%3ld cm", distEsquerda);
  exibirNoLCD("Procurando rota", buf);
  delay(500);

  // 3) Decide a melhor direção
  if (distEsquerda >= distDireita && distEsquerda > DISTANCIA_ROTA_LIVRE) {
    // Já está virado para a esquerda
    exibirNoLCD("Mudando direcao", "Indo p/ Esquerda");
    delay(500);
    return;
  }

  if (distDireita > distEsquerda && distDireita > DISTANCIA_ROTA_LIVRE) {
    // Volta para a direita
    exibirNoLCD("Mudando direcao", "Indo p/ Direita");
    girarDireita(VELOCIDADE_GIRO);
    delay(TEMPO_GIRO_90_MS * 2);
    pararMotores();
    delay(300);
    return;
  }

  // 4) Se ambos os lados estiverem bloqueados, gira até achar passagem livre
  exibirNoLCD("Sem saida aqui!", "Buscando saida..");
  for (int tentativa = 0; tentativa < 6; tentativa++) {
    girarEsquerda(VELOCIDADE_GIRO);
    delay(TEMPO_GIRO_90_MS / 2);
    pararMotores();
    delay(200);

    long distAtual = lerDistanciaCm();
    if (distAtual > DISTANCIA_ROTA_LIVRE) {
      exibirNoLCD("Rota encontrada!", "Seguindo...");
      delay(400);
      return;
    }
  }

  exibirNoLCD("Mudando direcao", "Tentando seguir");
  delay(300);
}

// ============================================================
// LEITURA DO HC-SR04 (2 amostras rápidas para resposta ágil)
// ============================================================
long lerDistanciaCm() {
  long soma = 0;
  int leiturasValidas = 0;

  for (int i = 0; i < 2; i++) {
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    unsigned long duracao = pulseIn(PIN_ECHO, HIGH, 25000UL);

    if (duracao > 0) {
      long cm = (long)(duracao * 0.0343 / 2.0);
      if (cm == 0) cm = 1; // Objeto praticamente encostado no sensor
      if (cm <= 400) {
        soma += cm;
        leiturasValidas++;
      }
    }
    delay(10);
  }

  if (leiturasValidas == 0) {
    return 400; // Sem eco: caminho livre
  }

  return soma / leiturasValidas;
}

// ============================================================
// CONTROLE DOS MOTORES (HW-095 / L298N)
// ============================================================
void moverFrente(int velocidade) {
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, HIGH);
  digitalWrite(PIN_IN4, LOW);
  analogWrite(PIN_ENA, velocidade);
  analogWrite(PIN_ENB, velocidade);
}

void moverTras(int velocidade) {
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, HIGH);
  digitalWrite(PIN_IN3, LOW);
  digitalWrite(PIN_IN4, HIGH);
  analogWrite(PIN_ENA, velocidade);
  analogWrite(PIN_ENB, velocidade);
}

void girarDireita(int velocidade) {
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, LOW);
  digitalWrite(PIN_IN4, HIGH);
  analogWrite(PIN_ENA, velocidade);
  analogWrite(PIN_ENB, velocidade);
}

void girarEsquerda(int velocidade) {
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, HIGH);
  digitalWrite(PIN_IN3, HIGH);
  digitalWrite(PIN_IN4, LOW);
  analogWrite(PIN_ENA, velocidade);
  analogWrite(PIN_ENB, velocidade);
}

void pararMotores() {
  analogWrite(PIN_ENA, 0);
  analogWrite(PIN_ENB, 0);
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, LOW);
  digitalWrite(PIN_IN4, LOW);
}

// ============================================================
// ATUALIZAÇÃO DO LCD SEM PISCAR (16 colunas fixas por linha)
// ============================================================
void exibirNoLCD(const char* linha1, const char* linha2) {
  char buf1[17];
  char buf2[17];
  snprintf(buf1, sizeof(buf1), "%-16.16s", linha1);
  snprintf(buf2, sizeof(buf2), "%-16.16s", linha2);

  lcd.setCursor(0, 0);
  lcd.print(buf1);
  lcd.setCursor(0, 1);
  lcd.print(buf2);
}
