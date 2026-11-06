# dc-relay

Quatro projetos de ESP32 neste repo:

- **dc-relay** (este arquivo): monitor de bateria com corte automático de carga.
- **[pc-button](pc-button/README.md)**: botão de power remoto de CPU via ESPHome.
- **[bms-monitor](bms-monitor/README.md)**: monitor de tensão do banco off-grid 4S LiFePO4 (INA219) via ESPHome.
- **[pressure-mca](pressure-MCA/README.md)**: pressão de água da usina (PT-307 4–20 mA + ADS1115, ESP32-C3) via ESPHome.

---

Monitor de bateria automotiva com ESP32 para desligamento automático de cargas DC (ex: Starlink Mini).

## O que faz

Monitora a tensão da bateria do carro usando um sensor INA219 e controla um relé para desconectar a carga quando a bateria fica baixa. Útil para evitar que equipamentos como a Starlink Mini drenem a bateria quando o carro está desligado.

### Lógica

- **Tensão < 12.2V** → desliga o relé (protege a bateria)
- **Tensão > 12.6V** → liga o relé (carro ligou, alternador carregando)
- **Entre 12.2V e 12.6V** → mantém o estado anterior (histerese para evitar chaveamento)

### Consumo

O ESP32 usa deep sleep entre leituras (~10μA). O estado do relé é mantido durante o sleep via `gpio_hold`. WiFi e Bluetooth ficam desligados.

| Estado | Intervalo de leitura |
|--------|---------------------|
| Relé ON | 60s |
| Relé OFF | 30s |

## Hardware

- ESP32 DevKit
- INA219 (sensor de corrente/tensão)
- Relé JQC3F 5V acionado via transistor NPN

### Pinagem

| Componente | Pino ESP32 |
|------------|-----------|
| INA219 SDA | GPIO 18 |
| INA219 SCL | GPIO 19 |
| Relé (base NPN) | GPIO 23 |

### Circuito do relé

```
GPIO23 → resistor base → NPN (base)
                          NPN (coletor) → IN do módulo relé
                          NPN (emissor) → GND
```

O VIN+ e VIN- do INA219 ficam em série com a alimentação da bateria.

## Build

Requer [PlatformIO](https://platformio.org/).

```bash
pio run -t upload
pio device monitor
```

## Configuração

Os parâmetros podem ser ajustados no `src/main.cpp`:

```c
#define VOLTAGE_OFF 12.2   // Tensão mínima para desligar o relé
#define VOLTAGE_ON  12.6   // Tensão mínima para religar o relé
#define SLEEP_RELAY_ON_S  60  // Intervalo de check com relé ligado
#define SLEEP_RELAY_OFF_S 30  // Intervalo de check com relé desligado
```
