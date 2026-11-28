# PROJETO_ROBO — Carrinho robô com desvio de obstáculos

**Autores:** Vinicius Christ e Guilherme Ladislau

Arduino Uno + ponte H L298N + sensor ultrassônico HC-SR04 num servo + LCD 16x2 I2C.

## Pastas

| Pasta | O que é |
|---|---|
| `CARRINHO_ROBO/` | **Código do robô** |
| `ROBO_SIMULIDE/` | Simulação do robô no SimulIDE, com roteiro de testes |
| `CALIBRA_SERVO/` | Programa de teste para achar o centro do servo (motores desligados) |
| `bibliotecas/LiquidCrystal_I2C/` | Biblioteca do LCD (API `lcd.init()` / `lcd.backlight()`) |

## Pinagem

| Peça | Pino do Arduino |
|---|---|
| L298N ENA (PWM, motor A = direito) | D11 |
| L298N IN1 / IN2 | D10 / D9 |
| L298N IN3 / IN4 | D8 / D7 |
| L298N ENB (PWM, motor B = esquerdo) | D6 |
| HC-SR04 TRIG / ECHO | D4 / D5 |
| Servo (sinal) | D3 |
| LCD I2C (0x27) SDA / SCL | A4 / A5 |

## Como o robô funciona

| Distância à frente | Ação |
|---|---|
| Mais de 50 cm | Anda rápido |
| De 26 a 50 cm | Anda devagar |
| 25 cm ou menos | Para, dá ré por 1,5 s e faz a varredura |

**Varredura:** o servo gira o sensor de 20° a 160°, de 10 em 10 graus (70° para
cada lado da frente, que é 90°). O robô procura a **maior abertura livre**
(60 cm ou mais em pelo menos 2 leituras seguidas), mira no centro dela e gira
o quanto for preciso para ficar de frente para ela. Se a abertura estiver bem
à frente, segue reto. Sem nenhuma abertura, gira para a esquerda em até 6
passos procurando saída.

**Velocidades** (PWM 0–255), uma para cada motor para o robô andar reto:

| Movimento | Motor A (direito) | Motor B (esquerdo) |
|---|---|---|
| Rápido | 80 | 97 |
| Devagar | 60 | 77 |
| Ré | 60 | 77 |
| Giro | 50 | 67 |

Todos os ajustes ficam em `#define` no topo do `CARRINHO_ROBO.ino`:
velocidades, distâncias, tempo de ré (`TEMPO_RE`), tempo de giro
(`TEMPO_GIRO_90`) e o servo (`SERVO_CENTRO` = 90; `VARREDURA_LADO`;
`PASSO_VARREDURA`). Se o sensor não olhar reto, use o `CALIBRA_SERVO` para
achar o ângulo certo e troque o `SERVO_CENTRO`.

## Como compilar

1. Copie `bibliotecas/LiquidCrystal_I2C` para `Documentos\Arduino\libraries`.
2. Abra `CARRINHO_ROBO/CARRINHO_ROBO.ino` na Arduino IDE.
3. Placa **Arduino Uno**, escolha a porta e clique em Carregar.

A biblioteca `Servo` já vem com a Arduino IDE.
