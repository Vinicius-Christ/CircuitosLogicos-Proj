# Radar Pan/Tilt

Projeto de Circuitos Lógicos — Vinicius Christ e Guilherme Ladislau.

Um radar com dois servos (Pan e Tilt) controlados por joystick, com senha no teclado, painel em display OLED, sensor de distância a laser, temperatura/umidade, sensor de presença, buzzer, um jogo de fuga e um modo música. Feito para **Arduino Uno (ATmega328P)**.

## O que o projeto faz

1. Mostra os nomes do grupo e pede uma **senha de 4 dígitos**.
2. Com a senha certa (`ACESSO LIBERADO`), o painel mostra: ângulo do Pan e do Tilt, temperatura, umidade, distância medida pelo laser e se há presença.
3. O **joystick** move os servos. Ele controla a *velocidade*: quanto mais inclinado, mais rápido o servo anda. Com o joystick no centro o servo **para e fica onde foi deixado**.
4. O **teclado** liga o jogo (tecla `A`) ou o modo música (tecla `D`).

## Como usar

### Senha
- Digite **1234**. A tecla `*` apaga o último dígito.
- Senha errada: `ACESSO NEGADO` e a tela volta para digitar de novo.
- Cada tecla apertada dá um bipe curto no buzzer.

### Joystick
Solte o joystick ao liberar o acesso: nesse momento o programa mede onde fica o centro dele.

### Jogo de fuga (tecla `A`)
- `A` liga e desliga o jogo. O joystick fica desligado enquanto o jogo está ligado.
- Se qualquer coisa chegar a **100 mm ou menos** do sensor laser, os servos "fogem": três movimentos seguidos para ângulos sorteados, alternando Pan, Tilt, Pan.
- No display aparece `JOGO ON` e, durante a fuga, `FUGA!`.

### Modo música (tecla `D`)
- `D` liga e desliga o modo música (desliga o jogo, e vice-versa).
- Com o modo ligado, as teclas escolhem a música e `*` para a que está tocando:

| Tecla | Música |
|---|---|
| 1 | Parabéns pra Você |
| 2 | Ode à Alegria |
| 3 | Brilha Brilha Estrelinha |
| 4 | Jingle Bells |
| 5 | Frère Jacques |
| 6 a 9 | opcionais, veja abaixo |

### Músicas extras (opcional)
As teclas 6 a 9 só existem (até 4 músicas extras) se houver um arquivo `RadarPanTilt/MusicasExtras.h` (ele não vem no repositório). Para criá-lo a partir de sketches Arduino de música, rode:

```
python ferramentas/converter_musicas.py musica_a.ino [musica_b.ino ...]
```

O script entende dois formatos: `tone(pino, frequência, duração)` com `delay(ms)`, e vetores `melody1[]`/`beats1[]` com o período das notas (cada melodia vira uma música). Ele gera o `RadarPanTilt/MusicasExtras.h`, e as músicas ganham as teclas 6, 7, 8 e 9 na ordem em que foram convertidas; depois é só gravar o programa de novo. Use apenas músicas que você tenha direito de usar.

## Ligações

| Peça | Pino do Arduino |
|---|---|
| Servo Pan (SG90) | D2 |
| Servo Tilt (SG90) | D3 |
| DHT11 (temperatura e umidade) | D4 |
| Joystick, eixo X (Pan) | A1 |
| Joystick, eixo Y (Tilt) | A0 |
| Sensor de presença PIR HC-SR501 | A3 |
| Buzzer ativo de 5 V | D1 (sinal) e GND |
| Teclado 4x4, linhas | D8, D7, D6, D5 |
| Teclado 4x4, colunas | D12, D11, D10, D9 |
| Display OLED 0,96" (I2C) | SDA (A4) e SCL (A5) |
| Laser VL53L0X (I2C, endereço 0x29) | SDA (A4) e SCL (A5) |

Teclado: `1 2 3 A` / `4 5 6 B` / `7 8 9 C` / `* 0 # D`.

Observações:
- O **laser VL53L0X** deve ser alimentado no **VIN com 5 V**. Ele não mede bem a menos de uns 3 a 4 cm.
- O buzzer fica no **pino 1**, que é o TX da serial. Por isso o programa **não usa o Serial** (o monitor serial faria o buzzer apitar).
- Os servos só são ligados depois da senha.

## Como gravar no Arduino

1. Instale a [Arduino IDE](https://www.arduino.cc/en/software) (ou o `arduino-cli`) com o núcleo **Arduino AVR Boards** (1.8.3 ou mais novo).
2. Abra `RadarPanTilt/RadarPanTilt.ino`. Os outros arquivos da pasta (`DisplayOled.h`, `MusicasBuzzer.h`, `SensorLaser.h`, `SensorLaser.cpp`) abrem junto.
3. Placa: **Arduino Uno**. Escolha a porta e clique em **Carregar**.

Não é preciso instalar nenhuma biblioteca extra: o driver do display, a leitura do DHT11 e a do teclado estão no próprio projeto, e `Wire` e `Servo` já vêm com a IDE.

## Estrutura

```
RadarPanTilt/
  RadarPanTilt.ino   programa principal
  DisplayOled.h      display OLED (só texto, sem usar RAM de buffer)
  MusicasBuzzer.h    as 5 músicas do buzzer
  SensorLaser.h/.cpp sensor de distância a laser VL53L0X (biblioteca da Pololu)
  MusicasExtras.h    (opcional, não vem no repositório) músicas 6 a 9
ferramentas/
  converter_musicas.py   gera o MusicasExtras.h
projetoTrabalho.sim1     simulação do circuito
```

## Ajustes

No começo de `RadarPanTilt/RadarPanTilt.ino`, na seção **AJUSTES**:

| Nome | O que muda |
|---|---|
| `DIST_FUGA` | distância (mm) que faz os servos fugirem no jogo |
| `MOVIMENTOS_FUGA` | quantos movimentos tem cada fuga |
| `JOY_ZONA_MORTA` | folga no centro do joystick |
| `JOY_VEL_MAX` | velocidade máxima dos servos (graus por segundo) |
| `JOY_INVERTE_PAN`, `JOY_INVERTE_TILT` | colocar 1 inverte o sentido do servo |

A senha (`1234`) está na função `loop()`, na linha com `memcmp_P`.

## Créditos

- `SensorLaser.h/.cpp`: biblioteca VL53L0X da Pololu, sem alterações.
- `DisplayOled.h`: usa a fonte e a sequência de inicialização da biblioteca SimpleOLED (Jobit Joseph, licença MIT, cabeçalho mantido no arquivo).
