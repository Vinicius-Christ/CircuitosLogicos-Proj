/*
 * CALIBRACAO DO CENTRO DO SERVO (sensor ultrassonico)
 *
 * Motores ficam DESLIGADOS. O servo (D3) varre devagar de 60 a 120 graus,
 * de 5 em 5, parando 3 s em cada angulo. O LCD mostra o angulo atual e a
 * distancia que o sensor le naquela posicao.
 * Anote o angulo em que o sensor fica olhando reto para a frente.
 *
 * Pela Serial (9600): mande um numero (0-180) e Enter para travar o servo
 * nesse angulo. Mande "v" para voltar a varrer.
 */

#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#define ENA 11
#define IN1 10
#define IN2 9
#define IN3 8
#define IN4 7
#define ENB 6
#define TRIG 4
#define ECHO 5
#define PINO_SERVO 3

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoSensor;

int angulo = 60;
bool varrendo = true;
unsigned long ultimoPasso = 0;

long lerDistancia() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long duracao = pulseIn(ECHO, HIGH, 25000);
  if (duracao == 0) return 400;
  return duracao * 0.0343 / 2;
}

void mostrar() {
  char l1[17], l2[17];
  snprintf(l1, sizeof(l1), "Angulo: %3d %s", angulo, varrendo ? "VAR" : "FIX");
  snprintf(l2, sizeof(l2), "Dist: %3ld cm    ", lerDistancia());
  lcd.setCursor(0, 0); lcd.print(l1);
  lcd.setCursor(0, 1); lcd.print(l2);
}

void irPara(int a) {
  angulo = constrain(a, 0, 180);
  servoSensor.write(angulo);
  Serial.print("ANGULO=");
  Serial.println(angulo);
}

void setup() {
  int pinos[] = {ENA, IN1, IN2, IN3, IN4, ENB};
  for (int i = 0; i < 6; i++) {
    pinMode(pinos[i], OUTPUT);
    digitalWrite(pinos[i], LOW);   // motores parados
  }
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  servoSensor.attach(PINO_SERVO);
  irPara(angulo);
  Serial.println("CALIBRA_SERVO pronto. Mande um angulo ou 'v'.");
}

void loop() {
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    s.trim();
    if (s == "v") {
      varrendo = true;
      Serial.println("VARRENDO");
    } else if (s.length() > 0) {
      varrendo = false;
      irPara(s.toInt());
    }
  }

  if (varrendo && millis() - ultimoPasso >= 3000) {
    ultimoPasso = millis();
    int prox = angulo + 5;
    if (prox > 120) prox = 60;
    irPara(prox);
  }

  static unsigned long ultimoLCD = 0;
  if (millis() - ultimoLCD >= 300) {
    ultimoLCD = millis();
    mostrar();
  }
}
