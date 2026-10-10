/*
  Radar Pan/Tilt - Proj. Circuito
  Junta os exemplos: Teclado (senha), Joystick + Servo (Pan/Tilt),
  DHT (temperatura/umidade), Distancia (VL53L0X), PIR (presenca) e Oled.
  Pinos iguais aos dos exemplos.

  Uso: digite a senha (1234). '*' apaga o ultimo digito.
       Senha certa  -> ACESSO LIBERADO e o painel aparece.
       Senha errada -> ACESSO NEGADO e volta para digitar.

  Servos (igual ao exemplo Servo da pasta): o servo do pino 2 (Pan) le o A1 e o
  do pino 3 (Tilt) le o A0; angulo = map(analogRead, 0, 1023, 0, 180), delay(15).
  O Tilt e um MG90S de 360 graus (rotacao continua): para ele esse "angulo" e na
  verdade velocidade e sentido (90 = parado), entao o numero do display e o
  comando enviado, nao a posicao real.

  JOGO: aperte A para ligar/desligar. Com o jogo ligado o joystick fica desligado
  e, sempre que algo estiver a 100 mm ou menos do laser, os servos fazem
  movimentos sorteados para "fugir" (o laser fica montado em um dos servos).

  Ligacao do laser VL53L0X: o VIN deve ir no 5 V, a mesma tensao do Uno (assim
  fazem os projetos Adafruit/Pololu/SunFounder com OLED no mesmo I2C). Testado
  na placa: com o modulo em 3,3 V o barramento ficava em ~3 V e o laser nao
  media; com VIN em 5 V o barramento fica em ~4,9 V e ele mede normalmente.
*/

#include <Wire.h>
#include <Servo.h>
#include "SimpleOLED.h"
#include "DHT.h"
#include "VL53L0X.h"
#include "Keypad.h"

SimpleOLED display(0x3C, 128, 64);
DHT dht(4, DHT11);
VL53L0X sensor;
Servo servoPan;                  // pino 2 (no exemplo Servo: servox)
Servo servoTilt;                 // pino 3 (no exemplo Servo: servoy)

const int potpinx = A1;          // joystick -> servo do pino 2 (Pan), como no exemplo Servo
const int potpiny = A0;          // joystick -> servo do pino 3 (Tilt), como no exemplo Servo
const int pinoPIR = A3;          // sensor de presenca
const byte ENDERECO_LASER = 0x29;

// Teclado (igual ao exemplo)
const byte LINHAS = 4;
const byte COLUNAS = 4;
char mapaTeclas[LINHAS][COLUNAS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte pinosLinhas[LINHAS] = {8, 7, 6, 5};
byte pinosColunas[COLUNAS] = {12, 11, 10, 9};
Keypad teclado = Keypad(makeKeymap(mapaTeclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

// Ajustes
const char SENHA[] = "1234";
const int DIST_FUGA = 100;       // jogo: algo a esta distancia (mm) ou menos faz os servos fugirem
const int TILT_PARADA = 90;      // Tilt e servo de rotacao continua: write(90) = parado

char digitada[5];
byte nDigitos = 0;
bool acesso = false;

int valx, valy;                  // leituras do joystick, como no exemplo Servo
int pan = 90, tilt = 90;         // angulos enviados aos servos e mostrados no display
bool jogo = false;               // tecla A liga/desliga
bool fugindo = false;            // o Tilt esta girando numa fuga
unsigned long fugaFim = 0;       // quando o Tilt para de girar
unsigned long proximaFuga = 0;   // intervalo minimo entre duas fugas
float temperatura = NAN, umidade = NAN;
int distancia = -1;              // mm, -1 = fora de alcance
bool sensorOk = false;           // laser encontrado e iniciado
byte tentativasLaser = 0;
bool presenca = false;
unsigned long tDist = 0, tDHT = 0, tTela = 0;
unsigned long proximaBusca = 3000;   // quando tentar achar o laser de novo
byte falhasLeitura = 0;              // leituras seguidas sem resposta do laser

// Escreve no display um texto guardado na flash (nao gasta RAM).
void txt(const __FlashStringHelper* s) {
  char buf[22];
  strncpy_P(buf, (PGM_P)s, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  display.print(buf);
}

void telaInicial() {
  display.clear();
  display.setCursor(0, 2);   txt(F("Grupo:"));
  display.setCursor(0, 12);  txt(F("Proj. Circuito"));
  display.setCursor(0, 28);  txt(F("Integrantes:"));
  display.setCursor(0, 40);  txt(F("Vinicius Christ"));
  display.setCursor(0, 50);  txt(F("Guilherme"));
  display.display();
}

void telaSenha() {
  display.clear();
  display.setCursor(15, 10);
  txt(F("Digite a senha:"));
  for (byte i = 0; i < 4; i++) {
    display.drawChar(40 + i * 12, 30, i < nDigitos ? '*' : '_');
  }
  display.display();
}

void telaPainel() {
  display.clear();
  display.setCursor(0, 0);   txt(F("Pan : "));  display.print(pan);   txt(F(" graus"));
  display.setCursor(0, 10);  txt(F("Tilt: "));  display.print(tilt);  txt(F(" graus"));

  display.setCursor(0, 20);  txt(F("Temp: "));
  if (isnan(temperatura)) txt(F("--"));
  else display.print(temperatura, 1);
  txt(F(" C"));

  display.setCursor(0, 30);  txt(F("Umid: "));
  if (isnan(umidade)) txt(F("--"));
  else display.print(umidade, 0);
  txt(F(" %"));

  display.setCursor(0, 40);  txt(F("Dist: "));
  if (!sensorOk) {
    txt(F("SEM SENSOR"));
  } else if (distancia >= 0) {
    display.print(distancia);
    txt(F(" mm"));
  } else {
    txt(F("fora de alcance"));
  }

  display.setCursor(0, 50);  txt(F("Pres: "));
  txt(presenca ? F("SIM") : F("NAO"));
  display.setCursor(66, 50);
  if (!jogo) txt(F("A=jogo"));
  else txt(fugindo ? F("FUGA!") : F("JOGO ON"));
  display.display();
}

// Jogo: algo perto do laser -> os servos fazem movimentos sorteados para "fugir".
void fogeDoObjeto(unsigned long agora) {
  int novoPan;
  do {
    novoPan = random(0, 181);
  } while (abs(novoPan - pan) < 45);               // sempre um movimento bem visivel
  pan = novoPan;
  servoPan.write(pan);

  int sentido = random(2) ? 1 : -1;                // Tilt (rotacao continua): sorteia o sentido,
  tilt = TILT_PARADA + sentido * random(25, 61);   // a velocidade...
  servoTilt.write(tilt);
  fugaFim = agora + random(300, 801);              // ...e por quanto tempo ele gira
  fugindo = true;
  proximaFuga = agora + 900;                       // da tempo do laser medir de novo
}

// So inicia o laser se ele responder no I2C (0x29); assim um sensor ausente ou
// ainda "dormindo" nao atrasa nem trava o resto do programa.
void procuraLaser() {
  Wire.beginTransmission(ENDERECO_LASER);
  if (Wire.endTransmission() != 0) return;       // nao respondeu

  Wire.setClock(100000);                         // igual ao exemplo Distancia
  sensor.setTimeout(500);
  sensorOk = sensor.init();
  if (sensorOk) {
    sensor.startContinuous();
    sensor.setTimeout(100);                      // leitura que nao responde nao pode travar o loop
    falhasLeitura = 0;
  } else {
    tentativasLaser++;
  }
  Wire.setClock(400000);
}

void setup() {
  pinMode(pinoPIR, INPUT);

  display.begin();                // tambem inicia o I2C (Wire)
  // Se o I2C travar, o Wire solta o barramento em 25 ms em vez de esperar para
  // sempre. (Precisa do nucleo "Arduino AVR Boards" 1.8.3 ou mais novo. Nao use
  // #ifdef WIRE_HAS_TIMEOUT: essa macro nao e definida no nucleo 1.8.8.)
  Wire.setWireTimeout(25000, true);
  dht.begin();

  telaInicial();
  delay(2500);
  procuraLaser();                 // o laser so atende ~60 ms depois de ligar; aqui ja esta pronto
  telaSenha();
}

void loop() {
  // ---------- Senha ----------
  if (!acesso) {
    char tecla = teclado.getKey();

    if (tecla >= '0' && tecla <= '9' && nDigitos < 4) {
      digitada[nDigitos++] = tecla;
      digitada[nDigitos] = '\0';
      telaSenha();
    } else if (tecla == '*' && nDigitos > 0) {
      nDigitos--;
      telaSenha();
    }

    if (nDigitos == 4) {
      delay(300);                 // deixa o 4o '*' aparecer
      display.clear();
      display.setCursor(15, 28);
      if (strcmp(digitada, SENHA) == 0) {
        acesso = true;
        txt(F("ACESSO LIBERADO"));
      } else {
        txt(F("ACESSO NEGADO"));
      }
      display.display();
      delay(1500);

      nDigitos = 0;
      if (acesso) {
        servoPan.attach(2);       // servos so ligam depois da senha (nos testes,
        servoTilt.attach(3);      // ligar no inicio coincidiu com o I2C travando)
        servoPan.write(pan);
        servoTilt.write(TILT_PARADA);
        tDHT = millis() - 2000;   // le o DHT logo na entrada
      } else {
        telaSenha();              // volta para digitar
      }
    }
    return;
  }

  // ---------- Sistema liberado ----------
  unsigned long agora = millis();

  // Tecla A: liga/desliga o jogo
  char tecla = teclado.getKey();
  if (tecla == 'A') {
    jogo = !jogo;
    fugindo = false;
    if (jogo) {
      randomSeed(micros());       // sorteio diferente a cada jogo
      tilt = TILT_PARADA;
      servoTilt.write(tilt);      // comeca com o Tilt parado
    }
  }

  if (!jogo) {
    // Servos: igual ao exemplo Servo (servox no pino 2 le o A1, servoy no pino 3 le o A0)
    valx = analogRead(potpinx);
    pan = map(valx, 0, 1023, 0, 180);
    servoPan.write(pan);
    valy = analogRead(potpiny);
    tilt = map(valy, 0, 1023, 0, 180);
    servoTilt.write(tilt);
    delay(15);
  } else {
    // Jogo: o joystick fica desligado; os servos so se mexem para fugir.
    if (fugindo && agora >= fugaFim) {
      tilt = TILT_PARADA;
      servoTilt.write(tilt);      // o Tilt para de girar
      fugindo = false;
    }
    if (sensorOk && distancia >= 0 && distancia <= DIST_FUGA && agora >= proximaFuga) {
      fogeDoObjeto(agora);
    }
    delay(15);
  }

  // Presenca
  presenca = (digitalRead(pinoPIR) == HIGH);

  // Laser: se nao esta funcionando, tenta de novo a cada 10 s (no maximo 3 falhas)
  if (!sensorOk && tentativasLaser < 3 && (long)(agora - proximaBusca) >= 0) {
    proximaBusca = agora + 10000;
    procuraLaser();
  }

  // Distancia (a cada 200 ms)
  if (sensorOk && agora - tDist >= 200) {
    tDist = agora;
    Wire.setClock(100000);
    uint16_t d = sensor.readRangeContinuousMillimeters();
    Wire.setClock(400000);
    if (sensor.timeoutOccurred() || d == 65535) {      // o laser nao respondeu
      if (++falhasLeitura >= 3) {                      // 3 falhas seguidas: desiste e tenta de novo depois
        sensorOk = false;
        distancia = -1;
        tentativasLaser++;
        proximaBusca = agora + 10000;
      }
    } else {
      falhasLeitura = 0;
      distancia = (d >= 8190) ? -1 : d;                // 8190 ou mais = nada no alcance
    }
  }

  // Temperatura e umidade (a cada 2 s)
  if (agora - tDHT >= 2000) {
    tDHT = agora;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperatura = t;   // se falhar, mantem o ultimo valor
    if (!isnan(h)) umidade = h;
  }

  // Display (a cada 250 ms)
  if (agora - tTela >= 250) {
    tTela = agora;
    telaPainel();
  }
}
