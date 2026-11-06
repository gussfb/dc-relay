# pressure-mca

Medição da pressão de água da usina (PT-307, 0–2 bar, 4–20 mA) com ESP32-C3 + ADS1115, integrada ao Home Assistant via ESPHome. Esta etapa só mede: não há controle.

## Hardware

| Ligação | Destino |
|---|---|
| PT-307 vermelho | +24 V |
| PT-307 preto | ponto de leitura |
| Shunt 8×1 kΩ em paralelo (125 Ω) | ponto de leitura → GND |
| 1 kΩ série | ponto de leitura → ADS1115 A0 |
| 100 nF | A0 → GND |
| ADS1115 VDD / GND | 3V3 / GND do ESP |
| ADS1115 SDA / SCL | GPIO5 / GPIO6 |
| ADS1115 ADDR | GND (0x48) |
| GND do ESP, ADS1115 e negativo dos 24 V | interligados |

Valores nominais: 4 mA = 0,5 V = 0 bar · 12 mA = 1,5 V = 1 bar · 20 mA = 2,5 V = 2 bar.

## Entidades no HA

| Entidade | Uso |
|---|---|
| Pressão (bar) | média móvel de 5 leituras (5 s); vira "desconhecido" no mesmo ciclo em que surge uma falha |
| Coluna d'água (mca) | Pressão × 10,1972 |
| Pressão (sem filtro) | leitura de cada segundo, já calibrada (diagnóstico) |
| Tensão no shunt / Corrente do sensor | medidas físicas brutas, sem calibração (diagnóstico) |
| Estado da medição | texto: OK / abaixo ou acima da faixa nominal / corrente baixa / corrente alta / falha de comunicação |
| Falha de comunicação ADS1115, Falha no laço 4-20 mA, Corrente fora da faixa nominal | binários `problem`, para automações |
| Falhas de leitura I²C | contador desde o boot |
| Conectado, Sinal Wi-Fi, IP, Tempo ligado | estado do dispositivo |
| Calibração zero / escala, botão Calibração neutra | configuração (persistem na flash) |

### Limiares de diagnóstico (corrente do laço)

Não se assume NAMUR NE43. Todos são `substitutions` no topo do YAML.

| Corrente | Estado | Pressão publicada |
|---|---|---|
| < 3,0 mA | Falha no laço (aberto, sem 24 V, desconectado) | desconhecido |
| 3,0–3,8 mA | Fora da faixa nominal | valor calculado (pode ser negativo) |
| 3,8–20,2 mA | OK | valor calculado |
| 20,2–24,0 mA | Fora da faixa nominal | valor calculado (> 2 bar) |
| > 24,0 mA | Falha no laço (curto, fiação) | desconhecido |
| erro de I²C | Falha de comunicação | tudo desconhecido |

- Nominal: 4–20 mA. Banda OK: ±0,2 mA (≈1,25% do span). Ela cobre a precisão do sensor (~0,5% FS ≈ 0,08 mA), o erro residual do shunt depois de medido e o erro de ganho do ADS1115.
- 3,0 mA: um transmissor de 2 fios vivo não opera muito abaixo de ~3,5 mA, porque precisa dessa corrente para a própria eletrônica. Abaixo de 3,0 mA, o mais provável é um laço aberto. Laço aberto daria 0 mA = −0,5 bar e nunca é publicado como pressão.
- 24 mA (3,0 V): fica acima da saturação típica de transmissores e abaixo do teto do ADS1115 alimentado a 3,3 V.
- Entre a banda OK e os limiares de falha, o valor é publicado sem limitar à faixa 0–2 bar e vem sinalizado. Acima de 20 mA a saída do sensor pode não ser linear.

## Ajustes

- **Shunt**: medir com o multímetro, com o sensor desligado, entre o ponto de leitura e o GND. Pôr o valor em `shunt_ohms` e regravar.
- **Calibração** (HA → dispositivo → Configuração): `p = p_nominal × escala + zero`. O padrão é escala 1 e zero 0. Ajustar o zero com o sistema sem pressão. Ajustar a escala comparando com um manômetro de referência perto de 2 bar.

## Primeira gravação

Placa conferida com o esptool: ESP32-C3 (QFN32) rev v0.4, 4 MB de flash embutida, USB-Serial/JTAG nativo. O logger está em `USB_SERIAL_JTAG`. Se um dia trocarem por uma placa com chip CH340 ou CP210x, mudar para `UART0`.

1. `cp secrets.example.yaml secrets.yaml` e preencher. Gerar a chave da API com `openssl rand -base64 32`.
2. Ligar o C3 pela USB. Ele deve aparecer como `/dev/ttyACM*` (USB nativo) ou `/dev/ttyUSB*` (chip serial).
3. Na pasta `pressure-MCA/`: `../.venv/bin/esphome run pressure-mca.yaml --device /dev/ttyACM0`
4. Se não entrar em modo de gravação: segurar BOOT (GPIO9), apertar e soltar RESET, soltar BOOT e repetir o passo 3. Depois de gravar, apertar RESET.
5. Próximas atualizações por OTA: `../.venv/bin/esphome run pressure-mca.yaml --device pressure-mca.local`

## Página web

`http://pressure-mca.local` (ou o IP). Sem senha: qualquer um na rede pode mudar a calibração ou reiniciar o ESP.

## Integração ao Home Assistant

Em Configurações → Dispositivos e serviços, o ESPHome deve descobrir `pressure-mca` sozinho. Se não descobrir: Adicionar integração → ESPHome → host `pressure-mca.local` ou o IP. Colar a `api_encryption_key` do `secrets.yaml`.

As entidades de 1 s gravam bastante no recorder. Se o banco crescer demais, excluir as de diagnóstico do recorder ou aumentar `read_interval`.

## Teste inicial

1. **Alimentação, sem o sensor**: medir 24 V na fonte. Medir 3,3 V entre VDD e GND do ADS1115. Medir o shunt com tudo desligado: ~125 Ω.
2. **Comunicação**: com o ESP ligado, `esphome logs pressure-mca.yaml`. O scan do I²C deve listar `0x48`. Sem o sensor, o estado esperado é "Corrente baixa: laço aberto…", com Pressão "desconhecido" e Tensão ≈ 0 V. Isso também testa o diagnóstico de laço aberto.
3. **Falha de I²C** (opcional): desconectar o SDA com tudo ligado. O estado deve virar "Falha de comunicação com ADS1115", com todas as medidas em "desconhecido" e o contador subindo. Reconectar. Se o ADS1115 estiver ausente no boot, reiniciar o ESP depois de corrigir.
4. **Com o sensor, sem pressão**: ligar os 24 V e medir com o multímetro a tensão entre o ponto de leitura e o GND. Esperado ≈ 0,50 V (4 mA). Comparar com "Tensão no shunt" no HA: a diferença deve ficar em poucos mV. A diferença causada pelo 1 kΩ série é desprezível (< 0,05%). Conferir "Corrente do sensor" = V ÷ shunt e "Pressão" ≈ 0.
5. **Com pressão**: comparar com um manômetro de referência e anotar a tensão, o HA e o manômetro em 2 ou 3 pontos. Só então ajustar o zero e a escala.

## Cuidados de hardware (não tratados no firmware)

- Se o sensor ou a fiação entrar em curto, os 24 V ficam direto no shunt: ~190 mA e ~4,6 W, cerca de 0,6 W em cada resistor de 1 kΩ (resistores de 1/4 W queimam). O A0 também receberia ~20 mA pelos clamps do ADS1115, acima do máximo absoluto de 10 mA. Mitigações: fusível/PTC de ~50 mA no +24 V; série de 4,7–10 kΩ em vez de 1 kΩ (erro ≈ 0,1–0,3%, absorvido pela calibração de escala); ou diodo Schottky de A0 para 3V3.
- O ADS1115 a 3,3 V aceita no máximo VDD + 0,3 V na entrada. Com ganho ±4,096 V, o limite útil é ~3,3 V (~26 mA).
