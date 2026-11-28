# ROBO_SIMULIDE — Simulação do carrinho robô no SimulIDE

**Autores:** Vinicius Christ e Guilherme Ladislau

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

Clique no potenciômetro e mude o valor dele. Na prática, distância ≈ (1000 − Ω) ÷ 2:

| Potenciômetro | Distância lida | Faixa |
|---|---|---|
| 711 Ω | ~143 cm | Caminho livre (anda rápido) |
| 920 Ω | ~40 cm | Devagar |
| 970 Ω | ~14 cm | Obstáculo (≤ 25 cm) |

No SimulIDE o sensor lê a **mesma distância para qualquer ângulo do servo**.
Para simular uma abertura só de um lado, mude o potenciômetro **durante a
varredura**: o LCD mostra o ângulo de cada leitura (`20 graus 143cm`), então
dá para trocar no momento certo. Diminuir a velocidade da simulação (por
exemplo para 20%) ajuda.

A varredura vai de **20° a 160°** de 10 em 10 graus. O centro (frente) é **90°**:
ângulos menores que 90 são a **direita**, maiores são a **esquerda**.

## Roteiro de testes

| # | Teste | Como fazer | O que deve acontecer |
|---|---|---|---|
| 1 | LCD e partida | Ligue a simulação | `Carrinho Robo / Inicializando...` e depois o estado e a distância |
| 2 | Caminho livre | Potenciômetro em 711 Ω | `Andando...`, os dois motores para frente, PWM ~31% em ENA e ~35% em ENB (80 e 88) |
| 3 | Devagar | 920 Ω | `Devagar...`, PWM cai para ~24% e ~27% (60 e 68) |
| 4 | Obstáculo e ré | 970 Ω | `Obstaculo encontrado!`, motores param, depois `Dando re...` com os dois motores no sentido contrário |
| 5 | Varredura com caminho livre na frente | Mude para 711 Ω assim que aparecer `Fazendo varredura...` e deixe até o fim | O LCD mostra cada ângulo de 20° a 160°, depois `Melhor abertura / Angulo: 90` e `Melhor caminho / Frente!` |
| 6 | Abertura à direita | Deixe 711 Ω de 20° a 60° e mude para 970 Ω a partir de 70° | `Angulo: 40` e `Melhor caminho / Direita!`; o robô gira (um motor para frente, o outro para trás). Deixe 711 Ω depois do giro, senão ele mostra `Ajustando giro` e gira mais um pouco |
| 7 | Abertura à esquerda | 970 Ω até 110° e 711 Ω de 120° a 160° | `Angulo: 140` e `Melhor caminho / Esquerda!`; o robô gira para o outro lado (deixe 711 Ω depois do giro) |
| 8 | Sem saída | Deixe 970 Ω a varredura toda | `Sem saida! / Procurando...`, o robô gira para a esquerda aos poucos |
| 9 | Saída encontrada | Durante o teste 8, mude para 711 Ω | `Rota encontrada! / Seguindo...` e o robô volta a andar |

Para ver o sentido dos motores, olhe a cor dos fios do L298: **vermelho é nível
alto** e **azul é nível baixo**.

O giro é **proporcional** ao ângulo da abertura: 90° de desvio = 1200 ms de giro,
40° = ~530 ms. O giro mais curto é 300 ms. Depois do giro o robô mede a frente; se continuar
bloqueada, mostra `Ajustando giro` e gira mais 300 ms (até 4 vezes).

No início de cada movimento os dois PWM sobem por 60 ms para 140 ("tranco" da
partida) e depois caem para a velocidade normal; dá para ver isso no analisador.

## O que a simulação não mostra

Estes pontos só dão para conferir no robô de verdade:

- **Força dos motores:** a ponte H do SimulIDE é ideal; a L298N real perde uns 2 V,
  e com PWM baixo o motor pode não sair do lugar.
- **Ângulo do giro:** se `TEMPO_GIRO_90` (1200 ms) vira mesmo uns 90°.
- **Andar reto:** se a diferença de velocidade entre os motores (A e B) deixa o robô reto.
- **Centro do servo:** se o sensor olha mesmo reto em 90°.

## Atualizar o programa da simulação

Se mudar o `../CARRINHO_ROBO/CARRINHO_ROBO.ino`, gere o `.hex` de novo. Na Arduino IDE, use
**Sketch → Exportar binário compilado** e copie o `.hex` (o que **não** tem
`with_bootloader` no nome) para esta pasta com o nome `CARRINHO_ROBO.hex`.
Depois recarregue o firmware no Arduino da simulação.
