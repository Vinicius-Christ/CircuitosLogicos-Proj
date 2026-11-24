# ROBO_SIMULIDE — Simulação do carrinho robô no SimulIDE

Circuito do robô com a **mesma pinagem do robô real** e o programa
`CARRINHO_ROBO` já carregado no Arduino da simulação.

| Arquivo | O que é |
|---|---|
| `ROBO.sim1` | Circuito do SimulIDE |
| `CARRINHO_ROBO.hex` | Programa do robô compilado (o Arduino da simulação carrega este arquivo) |

Os dois arquivos precisam ficar **na mesma pasta**.

Testado no **SimulIDE 1.1.0-SR2**.

## Peças e ligações

| Peça | Pino do Arduino |
|---|---|
| L298 ENA / ENB (PWM) | D11 / D6 |
| L298 IN1 / IN2 / IN3 / IN4 | D10 / D9 / D8 / D7 |
| HC-SR04 TRIG / ECHO | D4 / D5 |
| Servo (sinal) | D3 |
| LCD 16x2 via I2C (PCF8574, 0x27) | A4 (SDA) / A5 (SCL) |

- Motores: `MOTOR_ESQ` (saídas OUT1/OUT2) e `MOTOR_DIR` (OUT3/OUT4).
- O **potenciômetro** ligado ao sensor define a distância que o HC-SR04 "enxerga".
- O **analisador lógico** está ligado em ENA e ENB, para ver o PWM dos motores.

## Como abrir e rodar

1. Abra o SimulIDE e use **Arquivo → Abrir** para abrir `ROBO.sim1`.
2. Se o Arduino aparecer sem programa, clique com o botão direito nele,
   escolha **Carregar firmware** e selecione `CARRINHO_ROBO.hex`.
3. Clique no botão de **ligar a simulação** (o de energia, no topo).
4. O LCD mostra `Carrinho Robo / Inicializando...` por 1,5 s e o robô começa a andar.

## Como mudar a distância

Clique no potenciômetro e mude o valor dele. Valores usados nos testes:

| Potenciômetro | Distância lida | Faixa |
|---|---|---|
| 711 Ω | ~143 cm | Caminho livre |
| 970 Ω | ~14 cm | Devagar |
| 994 Ω | ~3 cm | Obstáculo |

No SimulIDE o sensor lê a **mesma distância para qualquer ângulo do servo**.
Para simular um lado livre e o outro bloqueado, mude o potenciômetro entre a
leitura da direita e a da esquerda (veja os testes 5 e 6). Diminuir a velocidade
da simulação (por exemplo para 20%) ajuda a fazer a troca no momento certo.

## Roteiro de testes

| # | Teste | Como fazer | O que deve acontecer |
|---|---|---|---|
| 1 | LCD e partida | Ligue a simulação | `Carrinho Robo / Inicializando...` e depois o estado e a distância |
| 2 | Caminho livre | Potenciômetro em 711 Ω | `Andando...`, os dois motores para frente, PWM ~24% no analisador (valor 60) |
| 3 | Devagar | 970 Ω | `Devagar...`, PWM cai para ~16% (valor 40) |
| 4 | Obstáculo e ré | 994 Ω | `Obstaculo encontrado!`, motores param, depois `Dando re...` com os dois motores no sentido contrário |
| 5 | Servo e escolha da esquerda | Deixe em 994 Ω enquanto o servo olha para a direita (0°); mude para 711 Ω quando ele for para a esquerda (120°) | O servo volta ao centro (60°) e o LCD mostra `Indo p/ Esquerda`; um motor gira para frente e o outro para trás |
| 6 | Escolha da direita | 711 Ω com o servo na direita, 994 Ω com o servo na esquerda | `Indo p/ Direita` e o robô gira para o outro lado |
| 7 | Sem saída | Deixe 994 Ω nas duas leituras | `Sem saida aqui! / Buscando saida..`, o robô gira para a esquerda aos poucos |
| 8 | Saída encontrada | Durante o teste 7, mude para 711 Ω | `Rota encontrada! / Seguindo...` e o robô volta a andar |

Para ver o sentido dos motores, olhe a cor dos fios do L298: **vermelho é nível
alto** e **azul é nível baixo**.

Resultado do teste feito em 24/11/2026: **todos os 8 testes passaram.**

## O que a simulação não mostra

Estes pontos só dão para conferir no robô de verdade:

- **Força dos motores:** a ponte H do SimulIDE é ideal; a L298N real perde uns 2 V,
  e com PWM baixo (40) o motor pode não sair do lugar.
- **Ângulo do giro:** se `TEMPO_GIRO_90` (1200 ms) vira mesmo uns 90°.
- **Centro do servo:** o centro de 60° foi calibrado no servo real com o
  `CALIBRA_SERVO`. Na simulação o servo pode parecer torto em 60°, e isso é esperado.

## Atualizar o programa da simulação

Se mudar o `../CARRINHO_ROBO/CARRINHO_ROBO.ino`, gere o `.hex` de novo. Na Arduino IDE, use
**Sketch → Exportar binário compilado** e copie o `.hex` (o que **não** tem
`with_bootloader` no nome) para esta pasta com o nome `CARRINHO_ROBO.hex`.
Depois recarregue o firmware no Arduino da simulação.
