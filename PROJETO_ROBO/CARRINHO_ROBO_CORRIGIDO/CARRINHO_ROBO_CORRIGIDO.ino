/*
 * CARRINHO ROBO - desvio de obstaculos com LCD I2C
 *
 * Codigo RECONSTRUIDO a partir do firmware JA CORRIGIDO que estava gravado
 * no Arduino Uno (lido com avrdude em 17/11/2026, backup em
 * ../BACKUP_ARDUINO_CORRIGIDO/). Os pinos, velocidades, tempos e mensagens
 * foram tirados direto do binario. Os nomes de variaveis e funcoes sao
 * nossos (o binario nao guarda nomes).
 *
 * Biblioteca LCD: "LiquidCrystal I2C" (Frank de Brabander / Marco Schwartz),
 * a que usa lcd.init() e lcd.backlight(). A copia em
 * ../Arduino_Libraries/LiquidCrystal_I2C tem a mesma API.
 *
 * PINAGEM CORRETA (lida do firmware corrigido)
 *   Ponte H L298N              Sensor HC-SR04       LCD 16x2 I2C (0x27)
 *   ENA -> D11 (PWM)           TRIG -> D4           SDA -> A4
 *   IN1 -> D10                 ECHO -> D5           SCL -> A5
 *   IN2 -> D9
 *   IN3 -> D8
 *   IN4 -> D7
 *   ENB -> D6  (PWM)
 *
 * Pela logica do codigo, o motor A (ENA/IN1/IN2) e o da ESQUERDA e o motor B
 * (ENB/IN3/IN4) e o da DIREITA: para virar a direita o codigo gira o motor A
 * para frente e o B para tras.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- Ponte H L298N ----------
#define ENA 11   // PWM motor A (esquerdo)
#define IN1 10
#define IN2 9
#define IN3 8
#define IN4 7
#define ENB 6    // PWM motor B (direito)

// ---------- Sensor ultrassonico HC-SR04 ----------
#define TRIG 4
#define ECHO 5

// ---------- Velocidades (PWM 0-255) ----------
#define VEL_FRENTE 165
#define VEL_RE     160
#define VEL_GIRO   180

// ---------- Distancias (cm) ----------
#define DIST_OBSTACULO 6     // abaixo disso: obstaculo
#define DIST_LIVRE     19    // a partir disso: caminho livre
#define DIST_MAX       400   // alcance maximo considerado

LiquidCrystal_I2C lcd(0x27, 16, 2);

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
void pararMotores() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void frente(int vel) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, vel);
  analogWrite(ENB, vel);
}

void re(int vel) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, vel);
  analogWrite(ENB, vel);
}

// Gira no proprio eixo: motor A para frente, motor B para tras
void virarDireita() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, VEL_GIRO);
  analogWrite(ENB, VEL_GIRO);
}

// Gira no proprio eixo: motor A para tras, motor B para frente
void virarEsquerda() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, VEL_GIRO);
  analogWrite(ENB, VEL_GIRO);
}

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

  lcd.init();
  lcd.backlight();
  mostrarLCD("Carrinho Robo", "Inicializando...");
  delay(1500);
}

void loop() {
  char linha[17];
  long distancia = lerDistancia();

  if (distancia < DIST_OBSTACULO) {
    pararMotores();
    mostrarLCD("Obstaculo", "encontrado!");
    delay(700);

    mostrarLCD("Obstaculo!", "Dando re...");
    re(VEL_RE);
    delay(450);
    pararMotores();
    delay(250);

    mostrarLCD("Procurando", "nova rota...");
    delay(300);

    // Olha para a direita
    virarDireita();
    delay(450);
    pararMotores();
    delay(200);
    long distDireita = lerDistancia();
    snprintf(linha, sizeof(linha), "Direita: %3ld cm", distDireita);
    mostrarLCD("Procurando rota", linha);
    delay(500);

    // Olha para a esquerda (gira o dobro para passar do centro)
    virarEsquerda();
    delay(900);
    pararMotores();
    delay(200);
    long distEsquerda = lerDistancia();
    snprintf(linha, sizeof(linha), "Esquerda:%3ld cm", distEsquerda);
    mostrarLCD("Procurando rota", linha);
    delay(500);

    if (distEsquerda >= distDireita) {
      if (distEsquerda >= DIST_LIVRE) {
        // Ja esta virado para a esquerda: so segue
        mostrarLCD("Mudando direcao", "Indo p/ Esquerda");
        delay(500);
        return;
      }
    } else {
      if (distDireita >= DIST_LIVRE) {
        mostrarLCD("Mudando direcao", "Indo p/ Direita");
        virarDireita();
        delay(900);
        pararMotores();
        delay(300);
        return;
      }
    }

    // Nenhum lado livre: gira aos poucos para a esquerda procurando saida
    mostrarLCD("Sem saida aqui!", "Buscando saida..");
    for (int tentativa = 0; tentativa < 6; tentativa++) {
      virarEsquerda();
      delay(225);
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
    return;
  }

  // Caminho livre
  frente(VEL_FRENTE);

  // Atualiza o LCD a cada 180 ms sem travar o loop
  unsigned long agora = millis();
  if (agora - ultimoLCD >= 180) {
    ultimoLCD = agora;
    if (distancia >= DIST_MAX) {
      strcpy(linha, "Dist: >400 cm");
    } else {
      snprintf(linha, sizeof(linha), "Dist: %3ld cm", distancia);
    }
    mostrarLCD("Andando...", linha);
  }
}
