# Radar Pan/Tilt

Trabalho de Circuitos Lógicos de Vinicius Christ e Guilherme Ladislau.

Radar com dois servos (Pan e Tilt) controlados por joystick. O programa pede uma senha no teclado, mostra os dados num display OLED (ângulos, temperatura, umidade, distância do laser e presença) e ainda tem um jogo de fuga e um modo música no buzzer. Roda num Arduino Uno (ATmega328P).

## Pastas

```
Arduino/
  RadarPanTilt/             código para gravar no Arduino de verdade
SimulIDE/
  RadarPanTilt_SimulIDE/    mesmo programa ajustado para o simulador
  RadarPanTilt_SimulIDE.hex programa já compilado para o simulador
  Circuito_RadarPanTilt.sim1 circuito do SimulIDE
Ferramentas/
  converter_musicas.py      gera o MusicasExtras.h
```

As duas versões do código são iguais, com duas diferenças:

- **Display:** o Arduino usa a janela de colunas/páginas do SSD1306 (comandos 0x21 e 0x22). O SimulIDE não move o cursor com esses comandos, então a versão dele usa o modo por páginas, com comandos de 1 byte.
- **Sensor de temperatura:** o circuito real tem um DHT11. O SimulIDE só tem o DHT22, então a versão dele também lê o formato do DHT22.

Uma versão não funciona direito no lugar da outra.

## Como usar

1. Digite a senha **1234**. A tecla `*` apaga o último dígito. Senha errada mostra `ACESSO NEGADO`.
2. Com o acesso liberado o painel aparece e o joystick passa a mexer nos servos. O joystick controla a velocidade: quanto mais inclinado, mais rápido. No centro o servo para onde está.
3. Tecla `A` liga e desliga o jogo. Com o jogo ligado o joystick fica desligado e, se algo chegar a 100 mm ou menos do laser, os servos fogem em três movimentos (Pan, Tilt, Pan).
4. Tecla `D` liga e desliga o modo música. Com ele ligado, as teclas de 1 a 9 escolhem a música e `*` para.
5. Cada tecla dá um bipe curto. Ao ligar, o buzzer toca uma fanfarra.

| Tecla | Música |
|---|---|
| 1 | Parabéns pra Você |
| 2 | Ode à Alegria |
| 3 | Brilha Brilha Estrelinha |
| 4 | Jingle Bells |
| 5 | Frère Jacques |
| 6 | Megalovania |
| 7 | All Star |
| 8 | Star Wars, Marcha Imperial |
| 9 | Star Wars, Tema |

## Ligações (Arduino Uno)

| Peça | Pino |
|---|---|
| Servo Pan (SG90) | D2 |
| Servo Tilt (SG90) | D3 |
| DHT11 | D4 |
| Joystick eixo X (Pan) | A1 |
| Joystick eixo Y (Tilt) | A0 |
| Sensor de presença PIR HC-SR501 | A3 |
| Buzzer ativo de 5 V | D1 e GND |
| Teclado 4x4, linhas | D8, D7, D6, D5 |
| Teclado 4x4, colunas | D12, D11, D10, D9 |
| Display OLED 0,96" (I2C) | A4 (SDA) e A5 (SCL) |
| Laser VL53L0X (I2C) | A4 (SDA) e A5 (SCL) |

Teclado: `1 2 3 A` / `4 5 6 B` / `7 8 9 C` / `* 0 # D`.

Cuidados:

- O VIN do laser VL53L0X vai no 5 V. Ele não mede bem a menos de uns 3 a 4 cm.
- O buzzer fica no pino 1, que é o TX da serial. Por isso o programa não usa Serial, e o fio do buzzer precisa ser tirado do pino 1 na hora de gravar o Arduino. Depois é só ligar de volta.
- Os servos só são ligados depois da senha.

## Gravar no Arduino

1. Abra `Arduino/RadarPanTilt/RadarPanTilt.ino` na Arduino IDE (núcleo Arduino AVR Boards).
2. Placa Arduino Uno, escolha a porta e clique em Carregar.

Não precisa instalar biblioteca. O display, o DHT11 e o teclado têm código próprio, e o laser vem junto (`SensorLaser`).

## Rodar no SimulIDE

1. Abra `SimulIDE/Circuito_RadarPanTilt.sim1`.
2. Clique com o botão direito no Uno, escolha Load firmware e selecione `SimulIDE/RadarPanTilt_SimulIDE.hex`.
3. Dê o play.

No circuito não existe o laser, então a distância aparece como `SEM SENSOR` e o jogo não dispara. Os servos estão com pulso de 544 a 2400 µs, igual à biblioteca Servo, e o display com 400 kHz.

Para compilar o código do simulador de novo, abra `SimulIDE/RadarPanTilt_SimulIDE/RadarPanTilt_SimulIDE.ino` e exporte o `.hex`.

## Ajustes

No começo do arquivo `.ino`:

| Nome | O que muda |
|---|---|
| `DIST_FUGA` | distância (mm) que faz os servos fugirem |
| `MOVIMENTOS_FUGA` | quantos movimentos tem cada fuga |
| `SERVO_PAN_MIN`, `SERVO_PAN_MAX` | limites do Pan, em graus |
| `SERVO_TILT_MIN`, `SERVO_TILT_MAX` | limites do Tilt, em graus |
| `JOY_ZONA_MORTA` | folga no centro do joystick |
| `JOY_VEL_MAX` | velocidade máxima dos servos |
| `JOY_INVERTE_PAN`, `JOY_INVERTE_TILT` | 1 inverte o sentido do servo |

Se um servo fizer barulho no fim do curso, aproxime o limite dele do meio. A senha está na função `loop()`.

## Músicas extras

As músicas 6 a 9 estão no arquivo `MusicasExtras.h` de cada pasta de código. Para trocar por outras, até 4:

```
python Ferramentas/converter_musicas.py musica1.ino musica2.ino
```

O script lê sketches de buzzer no formato `tone(pino, frequência, duração)` com `delay(ms)`, ou com vetores `melody1[]` e `beats1[]`, e gera o `MusicasExtras.h` nas duas pastas. Depois é só gravar de novo.

## Créditos

- `SensorLaser.h` e `SensorLaser.cpp` são a biblioteca VL53L0X da Pololu.
- `DisplayOled.h` usa a fonte e a inicialização da biblioteca SimpleOLED (Jobit Joseph, licença MIT, cabeçalho mantido no arquivo).
- Músicas 6 a 9: notas convertidas de sketches de terceiros. Megalovania vem do repositório [Arduino-Music-Codes](https://github.com/AnonymousAlly/Arduino-Music-Codes) e as duas de Star Wars da coleção [Arduino-Buzzer-Tone-Codes](https://github.com/AbhishekGhosh/Arduino-Buzzer-Tone-Codes) (licença BSD de 2 cláusulas). A fonte do All Star não foi registrada. As músicas pertencem aos autores originais.
