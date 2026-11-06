# bms-monitor

Monitor de tensão do banco off-grid (inversor + 4S LiFePO4 + BMS comum + carregador) com ESP32 e INA219, em ESPHome. O BMS não é smart, então o ESP mede a tensão no **P+** do BMS e mostra numa página web, com alarmes de tensão baixa e alta.

## Página web

`http://bms-monitor.local` (ou o IP que aparece em "Sistema"). Três grupos:

**Bateria**
- **Tensão P+**: leitura do INA219, média móvel de 5 leituras a cada 2 s.
- **Tensão por célula**: P+ dividido por 4.
- **Carga estimada**: % pela tabela de tensão de repouso (OCV) do LiFePO4. Só é confiável com o banco em repouso; carregando ou com o inversor puxando carga a estimativa fica distorcida, porque LiFePO4 tem curva muito plana.
- **Estado**: por tensão de célula.

  | Célula | Estado |
  |--------|--------|
  | ≥ 3,55 V | carregando (absorção) |
  | 3,375 a 3,55 V | cheia / flutuação |
  | 3,25 a 3,375 V | normal |
  | 3,10 a 3,25 V | baixa |
  | < 3,10 V | CRÍTICA |

- **Mínima / Máxima desde o reset**: extremos desde o boot ou desde o botão "Zerar mín e máx".

**Alarmes**
- **Bateria baixa**: liga quando P+ cai abaixo de "Limite tensão baixa" (padrão 12,4 V, 3,10 V/célula) e desliga só 0,2 V acima, para não piscar.
- **Tensão alta**: liga acima de "Limite tensão alta" (padrão 14,8 V). Acima disso o carregador está ajustado errado; 4S LiFePO4 carrega até 14,6 V no máximo.
- Os dois limites são editáveis na página e persistem no ESP.

**Sistema**: "Sensor OK" (INA219 respondendo nos últimos 30 s), IP, sinal WiFi, reiniciar.

## Hardware

- ESP32 DevKit
- Módulo INA219 (endereço I2C 0x40)

| INA219 | ESP32 |
|--------|-------|
| SDA | GPIO21 |
| SCL | GPIO22 |
| VCC | 3V3 |
| GND | GND |

### Ligação na bateria: só tensão

O INA219 mede a tensão de barramento no pino **VIN-** em relação ao GND. Para medir só tensão:

```
BMS P+  ──► VIN-  (pode juntar VIN+ e VIN- no mesmo ponto)
BMS P-  ──► GND do módulo / GND do ESP
```

**Não passe a corrente do inversor pelo módulo.** O shunt de 0,1 Ω do INA219 aguenta 3,2 A; um inversor puxa dezenas de ampères e queima o shunt. Por isso a corrente não é exibida.

O GND do ESP precisa estar no mesmo potencial do P-. Se o ESP for alimentado por um conversor buck ligado em P+/P-, isso já acontece. Se for por USB de um computador, o GND fica compartilhado pelo cabo; funciona, mas evite ligar o negativo da bateria em dois pontos diferentes.

Limites do INA219: 26 V no barramento. 4S LiFePO4 chega a 14,6 V, folga boa. O fundo de escala no YAML está em 16 V para ganhar resolução.

## Rede

Dois SSIDs em `secrets.yaml`; o ESP escolhe o que estiver disponível. Se não achar nenhum, sobe o AP `bms-monitor` com portal cativo.

## REST

```bash
curl http://bms-monitor.local/sensor/Tens%C3%A3o%20P%2B
curl http://bms-monitor.local/text_sensor/Estado
curl http://bms-monitor.local/binary_sensor/Bateria%20baixa
curl -d "" "http://bms-monitor.local/number/Limite%20tens%C3%A3o%20baixa/set?value=12.0"
curl -N http://bms-monitor.local/events      # SSE com todas as mudanças
```

## Build e gravação

Venv do ESPHome na raiz do repo (`.venv`). De dentro de `bms-monitor/`:

```bash
../.venv/bin/esphome compile bms-monitor.yaml
../.venv/bin/esphome run --device /dev/ttyUSB0 bms-monitor.yaml     # primeira vez, USB
../.venv/bin/esphome run --device bms-monitor.local bms-monitor.yaml # depois, OTA
../.venv/bin/esphome logs --device bms-monitor.local bms-monitor.yaml
```

## Ajustes no YAML

```yaml
substitutions:
  cells: "4"         # células em série (muda célula, % e estado)
  read_every: "2s"   # período de leitura
  avg_window: "5"    # média móvel
```

## secrets.yaml

Fica fora do git.

```yaml
wifi_ssid: "..."
wifi_password: "..."
wifi_ssid_lab: "..."
wifi_password_lab: "..."
```
