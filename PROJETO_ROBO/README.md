# PROJETO_ROBO — Carrinho robô com desvio de obstáculos

Arduino Uno + ponte H L298N + sensor ultrassônico HC-SR04 num servo + LCD 16x2 I2C.

## Pastas

| Pasta | O que é |
|---|---|
| `CARRINHO_ROBO_V2/` | **Programa atual do robô** (é o que está gravado no Arduino) |
| `CALIBRA_SERVO/` | Programa de teste para achar o centro do servo (motores desligados) |
| `CARRINHO_ROBO_CORRIGIDO/` | Código com a pinagem corrigida, reconstruído do firmware que estava no Arduino (referência) |
| `BACKUP_ARDUINO_CORRIGIDO/` | Backup do firmware corrigido e como regravá-lo |
| `bibliotecas/LiquidCrystal_I2C/` | Biblioteca do LCD (API `lcd.init()` / `lcd.backlight()`) |

## Pinagem

| Peça | Pino do Arduino |
|---|---|
| L298N ENA (PWM) | D11 |
| L298N IN1 / IN2 | D10 / D9 |
| L298N IN3 / IN4 | D8 / D7 |
| L298N ENB (PWM) | D6 |
| HC-SR04 TRIG / ECHO | D4 / D5 |
| Servo (sinal) | D3 |
| LCD I2C (0x27) SDA / SCL | A4 / A5 |

## Como o robô funciona (V2)

| Distância à frente | Ação |
|---|---|
| Mais de 20 cm | Anda rápido (PWM 60) |
| De 6 a 20 cm | Anda devagar (PWM 40) |
| 5 cm ou menos | Para, dá ré por 1,5 s, o servo olha para a direita e a esquerda, volta ao centro e o robô vira para o lado mais livre |

Se nenhum lado estiver livre, o robô gira para a esquerda em até 6 passos procurando saída.

Todos os ajustes ficam em `#define` no topo do `CARRINHO_ROBO_V2.ino`:
velocidades (`VEL_RAPIDA`, `VEL_DEVAGAR`, `VEL_RE`, `VEL_GIRO`), distâncias,
tempo de ré (`TEMPO_RE`), tempo de giro (`TEMPO_GIRO_90`) e ângulos do servo
(`SERVO_CENTRO` = 60, calibrado com o `CALIBRA_SERVO`).

## Como compilar

1. Copie `bibliotecas/LiquidCrystal_I2C` para `Documentos\Arduino\libraries`.
2. Abra `CARRINHO_ROBO_V2/CARRINHO_ROBO_V2.ino` na Arduino IDE.
3. Placa **Arduino Uno**, escolha a porta e clique em Carregar.

A biblioteca `Servo` já vem com a Arduino IDE.
