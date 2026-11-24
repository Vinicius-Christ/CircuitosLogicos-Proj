/*
 * CARRINHO ROBO - desvio de obstaculos com servo + HC-SR04 + LCD I2C
 *
 * Autores: Vinicius Christ e Guilherme Ladislau
 *
 * Funcionamento:
 *   - Caminho livre (mais de 20 cm): anda rapido (VEL_RAPIDA)
 *   - Algo entre 6 e 20 cm: anda devagar (VEL_DEVAGAR)
 *   - Chegou a 5 cm ou menos: para, da re, o SERVO gira o sensor para a
 *     direita e para a esquerda, volta ao centro e o robo vira para o lado
 *     mais livre. Sem nenhum lado livre, gira aos poucos procurando saida.
 *   - O LCD mostra o que o robo esta fazendo e a distancia medida.
 *
 * PINAGEM
 *   Ponte H L298N              Sensor HC-SR04       LCD 16x2 I2C (0x27)
 *   ENA -> D11 (PWM)           TRIG -> D4           SDA -> A4
 *   IN1 -> D10                 ECHO -> D5           SCL -> A5
 *   IN2 -> D9
 *   IN3 -> D8                  Servo (sinal) -> D3
 *   IN4 -> D7
 *   ENB -> D6  (PWM)
 *
 * Bibliotecas: Servo (vem com a Arduino IDE) e "LiquidCrystal I2C"
 * (copia em ../bibliotecas/LiquidCrystal_I2C).
 */

#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

// ---------- Ponte H L298N ----------
#define ENA 11   // PWM motor A (esquerdo)
#define IN1 10
#define IN2 9
#define IN3 8
#define IN4 7
#define ENB 6    // PWM motor B (direito)

// true = inverte o sentido dos dois motores (de acordo com a ligacao dos fios)
#define INVERTER_MOTORES true

// ---------- Sensor ultrassonico HC-SR04 ----------
#define TRIG 4
#define ECHO 5

// ---------- Servo que gira o sensor ----------
#define PINO_SERVO     3
#define SERVO_CENTRO   60    // calibrado: em 60 graus o sensor olha reto
// Lados a 60 graus do centro (o servo nao passa de 0, entao 60 e o maximo
// simetrico). Troque os dois valores se o servo olhar para o lado errado.
#define SERVO_DIREITA  0
#define SERVO_ESQUERDA 120
#define TEMPO_SERVO    400   // ms para o servo chegar na posicao

// ---------- Velocidades (PWM 0-255) ----------
#define VEL_RAPIDA  60    // caminho livre
#define VEL_DEVAGAR 40    // algo a 20 cm ou menos
#define VEL_RE      VEL_DEVAGAR  // re na mesma velocidade da baixa
#define VEL_GIRO    50    // ao mudar, recalibre TEMPO_GIRO_90 abaixo

// ---------- Distancias (cm) ----------
#define DIST_PARAR   5     // <= 5 cm: para e procura caminho
#define DIST_DEVAGAR 20    // <= 20 cm: diminui a velocidade
#define DIST_LIVRE   19    // a partir disso o lado e considerado livre
#define DIST_MAX     400

// ---------- Tempo de re (ms) ----------
// Quanto maior, mais longe o robo recua antes de procurar caminho
#define TEMPO_RE 1500    // era 450

// ---------- Tempos de giro do robo (ms) ----------
// Com VEL_GIRO 180 o robo virava ~90 graus em 450 ms. Com 50 e estimativa:
// se virar MENOS que 90 graus aumente, se virar MAIS diminua.
#define TEMPO_GIRO_90    1200
#define TEMPO_GIRO_PASSO (TEMPO_GIRO_90 / 2)  // passo curto da busca "sem saida"

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
// sentidoA / sentidoB: true = para frente, false = para tras
void acionarMotores(bool frenteA, bool frenteB, int vel) {
  if (INVERTER_MOTORES) {
    frenteA = !frenteA;
    frenteB = !frenteB;
  }
  digitalWrite(IN1, frenteA ? HIGH : LOW);
  digitalWrite(IN2, frenteA ? LOW : HIGH);
  digitalWrite(IN3, frenteB ? HIGH : LOW);
  digitalWrite(IN4, frenteB ? LOW : HIGH);
  analogWrite(ENA, vel);
  analogWrite(ENB, vel);
}

void pararMotores() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void frente(int vel)  { acionarMotores(true,  true,  vel); }
void re(int vel)      { acionarMotores(false, false, vel); }
void virarDireita()   { acionarMotores(true,  false, VEL_GIRO); }  // A frente, B tras
void virarEsquerda()  { acionarMotores(false, true,  VEL_GIRO); }  // A tras, B frente

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

// Gira o servo para um angulo, espera e mede
long medirNoAngulo(int angulo) {
  servoSensor.write(angulo);
  delay(TEMPO_SERVO);
  return lerDistancia();
}

// Obstaculo a 5 cm: para, recua, olha para os lados com o servo e escolhe
void procurarCaminho() {
  char linha[17];

  pararMotores();
  mostrarLCD("Obstaculo", "encontrado!");
  delay(700);

  mostrarLCD("Obstaculo!", "Dando re...");
  re(VEL_RE);
  delay(TEMPO_RE);
  pararMotores();
  delay(250);

  mostrarLCD("Procurando", "nova rota...");

  long distDireita = medirNoAngulo(SERVO_DIREITA);
  snprintf(linha, sizeof(linha), "Direita: %3ld cm", distDireita);
  mostrarLCD("Procurando rota", linha);
  delay(500);

  long distEsquerda = medirNoAngulo(SERVO_ESQUERDA);
  snprintf(linha, sizeof(linha), "Esquerda:%3ld cm", distEsquerda);
  mostrarLCD("Procurando rota", linha);
  delay(500);

  servoSensor.write(SERVO_CENTRO);
  delay(TEMPO_SERVO);

  if (distEsquerda >= distDireita && distEsquerda >= DIST_LIVRE) {
    mostrarLCD("Mudando direcao", "Indo p/ Esquerda");
    virarEsquerda();
    delay(TEMPO_GIRO_90);
    pararMotores();
    delay(300);
    return;
  }
  if (distDireita > distEsquerda && distDireita >= DIST_LIVRE) {
    mostrarLCD("Mudando direcao", "Indo p/ Direita");
    virarDireita();
    delay(TEMPO_GIRO_90);
    pararMotores();
    delay(300);
    return;
  }

  // Nenhum lado livre: gira o robo aos poucos (sensor no centro)
  mostrarLCD("Sem saida aqui!", "Buscando saida..");
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
  mostrarLCD("Mudando direcao", "Tentando seguir");
  delay(300);
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
    frente(VEL_DEVAGAR);
    estado = "Devagar...";
  } else {
    frente(VEL_RAPIDA);
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
