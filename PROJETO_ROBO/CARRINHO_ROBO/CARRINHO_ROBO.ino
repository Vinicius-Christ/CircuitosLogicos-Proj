/*
 * CARRINHO ROBO - desvio inteligente de obstaculos
 * Servo + HC-SR04 + LCD I2C
 *
 * Autores: Vinicius Christ e Guilherme Ladislau
 *
 * Funcionamento:
 *   - Mais de 50 cm livres: anda rapido.
 *   - De 26 a 50 cm: anda devagar.
 *   - 25 cm ou menos: para, da re e o servo faz uma varredura de 60 graus
 *     para cada lado da frente. O robo procura a maior abertura livre,
 *     mira no centro dela e gira o quanto for preciso para ficar de frente
 *     para ela. Sem nenhuma abertura, gira aos poucos procurando saida.
 *   - O LCD mostra o que o robo esta fazendo e a distancia medida.
 *
 * PINAGEM
 *   Ponte H L298N                       Sensor HC-SR04    LCD 16x2 I2C (0x27)
 *   ENA -> D11 (PWM motor A, direito)   TRIG -> D4        SDA -> A4
 *   IN1 -> D10                          ECHO -> D5        SCL -> A5
 *   IN2 -> D9
 *   IN3 -> D8                           Servo (sinal) -> D3
 *   IN4 -> D7
 *   ENB -> D6  (PWM motor B, esquerdo)
 *
 * Bibliotecas: Servo (vem com a Arduino IDE) e "LiquidCrystal I2C"
 * (copia em ../bibliotecas/LiquidCrystal_I2C).
 */

#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

// ---------- Ponte H L298N ----------
#define ENA 11   // PWM motor A (direito)
#define IN1 10
#define IN2 9
#define IN3 8
#define IN4 7
#define ENB 6    // PWM motor B (esquerdo)

// true = inverte o sentido dos dois motores (de acordo com a ligacao dos fios)
#define INVERTER_MOTORES true

// ---------- Sensor ultrassonico HC-SR04 ----------
#define TRIG 4
#define ECHO 5

// ---------- Servo que gira o sensor ----------
#define PINO_SERVO       3
#define SERVO_CENTRO     60    // calibrado: em 60 graus o sensor olha reto
#define VARREDURA_LADO   60    // graus para cada lado do centro (0 a 120)
#define PASSO_VARREDURA  10    // graus entre uma leitura e outra
#define TEMPO_SERVO      150   // ms para andar um passo da varredura
#define TEMPO_SERVO_LONGO 400  // ms para saltos grandes (ir ao inicio, voltar ao centro)

#define SERVO_MIN (SERVO_CENTRO - VARREDURA_LADO)
#define SERVO_MAX (SERVO_CENTRO + VARREDURA_LADO)

// ---------- Velocidades (PWM 0-255), uma para cada motor ----------
// O motor B recebe mais para compensar a diferenca entre os motores
// e o robo andar reto.
#define VEL_RAPIDA_A  80
#define VEL_RAPIDA_B  97
#define VEL_DEVAGAR_A 60
#define VEL_DEVAGAR_B 77
#define VEL_RE_A      60
#define VEL_RE_B      77
#define VEL_GIRO_A    50
#define VEL_GIRO_B    67

// ---------- Distancias (cm) ----------
#define DIST_PARAR   25    // <= 25 cm: para e procura caminho
#define DIST_DEVAGAR 50    // <= 50 cm: anda devagar
#define DIST_LIVRE   60    // a partir disso a direcao e considerada livre
#define DIST_MAX     400

// ---------- Tempos (ms) ----------
#define TEMPO_RE         1500UL
#define TEMPO_GIRO_90    1200UL  // tempo para o robo girar ~90 graus
#define TEMPO_GIRO_MIN   300UL   // giro mais curto (abaixo disso o motor nao sai do lugar)
#define TEMPO_GIRO_PASSO 600UL   // passo curto da busca "sem saida"

// ---------- Varredura ----------
#define MIN_LEITURAS_LIVRES 2      // minimo de leituras seguidas livres para ser abertura
#define TOLERANCIA_FRENTE   10     // abertura a ate 10 graus do centro: segue reto

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoSensor;

unsigned long ultimoLCD = 0;

// Escreve as duas linhas do LCD, completando com espacos ate 16 colunas
void mostrarLCD(const char *linha1, const char *linha2) {
  char l1[17];
  char l2[17];
  snprintf(l1, sizeof(l1), "%-16.16s", linha1);
  snprintf(l2, sizeof(l2), "%-16.16s", linha2);
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

// ---------- Motores ----------
// frenteA / frenteB: true = para frente, false = para tras
void acionarMotores(bool frenteA, bool frenteB, int velA, int velB) {
  if (INVERTER_MOTORES) {
    frenteA = !frenteA;
    frenteB = !frenteB;
  }
  digitalWrite(IN1, frenteA ? HIGH : LOW);   // motor A (direito)
  digitalWrite(IN2, frenteA ? LOW : HIGH);
  digitalWrite(IN3, frenteB ? HIGH : LOW);   // motor B (esquerdo)
  digitalWrite(IN4, frenteB ? LOW : HIGH);
  analogWrite(ENA, constrain(velA, 0, 255));
  analogWrite(ENB, constrain(velB, 0, 255));
}

void pararMotores() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void frente(int velA, int velB) { acionarMotores(true,  true,  velA, velB); }
void darRe(int velA, int velB)  { acionarMotores(false, false, velA, velB); }

// Gira no proprio eixo: esquerdo (B) para frente, direito (A) para tras
void virarDireita()  { acionarMotores(false, true,  VEL_GIRO_A, VEL_GIRO_B); }
// Gira no proprio eixo: direito (A) para frente, esquerdo (B) para tras
void virarEsquerda() { acionarMotores(true,  false, VEL_GIRO_A, VEL_GIRO_B); }

// ---------- Sensor ----------
// Faz 2 leituras, descarta as invalidas e devolve a media em cm.
// Sem nenhuma leitura valida, devolve DIST_MAX (caminho livre).
long lerDistancia() {
  long soma = 0;
  int validas = 0;

  for (int i = 0; i < 2; i++) {
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    unsigned long duracao = pulseIn(ECHO, HIGH, 25000);  // timeout 25 ms
    if (duracao > 0) {
      long d = duracao * 0.0343 / 2;
      if (d == 0) d = 1;
      if (d <= DIST_MAX) {
        soma += d;
        validas++;
      }
    }
    delay(10);
  }

  if (validas == 0) return DIST_MAX;
  return soma / validas;
}

// ---------- Varredura ----------
// Gira o servo de SERVO_MIN a SERVO_MAX medindo a distancia, procura a maior
// sequencia de leituras livres e devolve o angulo do centro dessa abertura.
// Devolve -1 se nao houver nenhuma abertura.
int procurarMelhorDirecao() {
  const int TOTAL = (SERVO_MAX - SERVO_MIN) / PASSO_VARREDURA + 1;
  long distancias[TOTAL];
  char linha[17];

  mostrarLCD("Fazendo", "varredura...");

  // Primeiro salto e grande (do centro ao inicio): espera mais
  servoSensor.write(SERVO_MIN);
  delay(TEMPO_SERVO_LONGO);

  for (int i = 0; i < TOTAL; i++) {
    int angulo = SERVO_MIN + i * PASSO_VARREDURA;
    servoSensor.write(angulo);
    delay(TEMPO_SERVO);
    distancias[i] = lerDistancia();

    snprintf(linha, sizeof(linha), "%3d graus %3ldcm", angulo, distancias[i]);
    mostrarLCD("Analisando rota", linha);
  }

  servoSensor.write(SERVO_CENTRO);
  delay(TEMPO_SERVO_LONGO);

  // Maior sequencia de leituras livres
  int melhorInicio = -1;
  int maiorTamanho = 0;
  int inicioAtual = 0;
  int tamanhoAtual = 0;

  for (int i = 0; i < TOTAL; i++) {
    if (distancias[i] >= DIST_LIVRE) {
      if (tamanhoAtual == 0) inicioAtual = i;
      tamanhoAtual++;
      if (tamanhoAtual > maiorTamanho) {
        maiorTamanho = tamanhoAtual;
        melhorInicio = inicioAtual;
      }
    } else {
      tamanhoAtual = 0;
    }
  }

  if (maiorTamanho < MIN_LEITURAS_LIVRES) return -1;

  int anguloInicio = SERVO_MIN + melhorInicio * PASSO_VARREDURA;
  int anguloFim = anguloInicio + (maiorTamanho - 1) * PASSO_VARREDURA;
  int melhorAngulo = (anguloInicio + anguloFim) / 2;

  snprintf(linha, sizeof(linha), "Angulo: %3d", melhorAngulo);
  mostrarLCD("Melhor abertura", linha);
  delay(500);

  return melhorAngulo;
}

// Gira o robo proporcionalmente aos graus pedidos (90 graus = TEMPO_GIRO_90)
void girarGraus(int graus, bool paraDireita) {
  unsigned long tempo = (unsigned long)graus * TEMPO_GIRO_90 / 90;
  if (tempo < TEMPO_GIRO_MIN) tempo = TEMPO_GIRO_MIN;

  if (paraDireita) virarDireita();
  else virarEsquerda();
  delay(tempo);
  pararMotores();
}

// Obstaculo perto: para, recua, faz a varredura e escolhe a direcao
void procurarCaminho() {
  pararMotores();
  mostrarLCD("Obstaculo", "encontrado!");
  delay(700);

  mostrarLCD("Obstaculo!", "Dando re...");
  darRe(VEL_RE_A, VEL_RE_B);
  delay(TEMPO_RE);
  pararMotores();
  delay(250);

  int melhorAngulo = procurarMelhorDirecao();

  if (melhorAngulo != -1) {
    // Angulo menor que o centro = direita; maior = esquerda
    int desvio = melhorAngulo - SERVO_CENTRO;

    if (abs(desvio) <= TOLERANCIA_FRENTE) {
      mostrarLCD("Melhor caminho", "Frente!");
      frente(VEL_DEVAGAR_A, VEL_DEVAGAR_B);
      delay(500);
      pararMotores();
    } else if (desvio < 0) {
      mostrarLCD("Melhor caminho", "Direita!");
      girarGraus(-desvio, true);
    } else {
      mostrarLCD("Melhor caminho", "Esquerda!");
      girarGraus(desvio, false);
    }
    delay(300);
    return;
  }

  // Nenhuma abertura: gira aos poucos (sensor no centro) procurando saida
  mostrarLCD("Sem saida!", "Procurando...");
  for (int tentativa = 0; tentativa < 6; tentativa++) {
    virarEsquerda();
    delay(TEMPO_GIRO_PASSO);
    pararMotores();
    delay(200);
    if (lerDistancia() >= DIST_LIVRE) {
      mostrarLCD("Rota encontrada!", "Seguindo...");
      delay(400);
      return;
    }
  }
  mostrarLCD("Sem caminho!", "Tentando outra");
  delay(500);
}

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);

  pararMotores();

  servoSensor.attach(PINO_SERVO);
  servoSensor.write(SERVO_CENTRO);

  lcd.init();
  lcd.backlight();
  mostrarLCD("Carrinho Robo", "Inicializando...");
  delay(1500);
}

void loop() {
  char linha[17];
  long distancia = lerDistancia();

  if (distancia <= DIST_PARAR) {
    procurarCaminho();
    return;
  }

  const char *estado;
  if (distancia <= DIST_DEVAGAR) {
    frente(VEL_DEVAGAR_A, VEL_DEVAGAR_B);
    estado = "Devagar...";
  } else {
    frente(VEL_RAPIDA_A, VEL_RAPIDA_B);
    estado = "Andando...";
  }

  // Atualiza o LCD a cada 180 ms sem travar o loop
  unsigned long agora = millis();
  if (agora - ultimoLCD >= 180) {
    ultimoLCD = agora;
    if (distancia >= DIST_MAX) {
      strcpy(linha, "Dist: >400 cm");
    } else {
      snprintf(linha, sizeof(linha), "Dist: %3ld cm", distancia);
    }
    mostrarLCD(estado, linha);
  }
}
