# pc-button

Botão de power remoto para CPU, com ESP32 rodando ESPHome. O relé fica em paralelo com o botão físico do gabinete e é acionado pela rede.

## Botões (página web e Home Assistant)

| Botão | Ação | Duração |
|-------|------|---------|
| Start | Liga a máquina, ou shutdown normal via ACPI se estiver ligada | 400 ms |
| Force Shutdown | Desliga a fonte na marra | 7 s |
| Reiniciar ESP | Reboot do ESP32 | - |

Start e Force Shutdown só funcionam com a chave "Destravar" ligada. Ela desliga sozinha após 30 s ou logo após um acionamento, pra evitar clique acidental. Um pedido durante um pulso em andamento é ignorado. O sensor "Relé acionado" mostra o estado do relé em tempo real.

## Watchdog

Chave "Watchdog" na página web, gravada na flash e desligada por padrão. Com ela ligada o ESP sonda o servidor a cada minuto:

1. Tenta uma conexão TCP em até 5 portas, em ordem (22, 80, 443, 445, 3389 por padrão), com 250 ms de timeout cada. Para na primeira que responder.
2. Qualquer resposta TCP conta como vivo, inclusive "conexão recusada". Só silêncio total (timeout) conta como morto. Isso evita Start numa máquina ligada, já que Start numa máquina ligada vira shutdown ACPI.
3. Três minutos seguidos sem resposta disparam um Start automático.
4. Depois de qualquer acionamento do relé, automático ou manual, fica 5 minutos de carência sem sondar, pra máquina bootar.
5. Depois de 3 Starts automáticos sem o servidor voltar, desiste e mostra "DESISTI" no status. Desligar e religar a chave zera os contadores.
6. Sem WiFi no ESP não conta como falha.

Entidades na página:

| Entidade | Função |
|----------|--------|
| Servidor IP | IP sondado, editável na página, gravado na flash |
| Watchdog | Chave liga/desliga |
| Testar watchdog | Sonda agora e mostra o resultado, sem mexer no relé |
| Watchdog status | Texto com o estado atual |
| Servidor vivo | Resultado da última sonda |

Atenção: um servidor com firewall em DROP e nenhuma das portas aberta aparece como morto. Use o botão "Testar watchdog" antes de ligar a chave. Portas e tempos ficam em `substitutions` no YAML.

## Acesso

- Página web: `http://pc-button.local/` ou pelo IP que aparece no sensor "IP"
- REST, pra scripts:

```bash
curl -X POST -d "" http://pc-button.local/switch/Destravar/turn_on
curl -X POST -d "" http://pc-button.local/button/Start/press
curl -X POST -d "" "http://pc-button.local/button/Force%20Shutdown/press"   # também precisa destravar antes
```

A página web agrupa as entidades em ENERGIA, Watchdog e Sistema.

Se nenhuma rede WiFi for encontrada, o ESP sobe um AP aberto chamado `pc-button` com portal captivo pra configurar.

## Hardware

- ESP32 DevKit
- Módulo relé 5V, alimentado pelo pino VIN do devkit
- 2N2222 NPN acionado pela GPIO23

```
GPIO23 → resistor base → 2N2222 (base)
                         2N2222 (coletor) → IN do módulo relé
                         2N2222 (emissor) → GND
```

Contatos COM e NO do relé vão nos dois pinos PWR_SW da placa-mãe, em paralelo com o botão do gabinete. Polaridade não importa.

GPIO23 sobe em LOW no boot, então o relé não pulsa quando o ESP reinicia ou é gravado.

## Build e gravação

ESPHome fica em um venv na raiz do repo:

```bash
python3 -m venv .venv
.venv/bin/pip install esphome
```

Redes WiFi em `pc-button/secrets.yaml`, fora do git:

```yaml
wifi_ssid: "rede-casa"
wifi_password: "senha"
wifi_ssid_lab: "rede-lab"
wifi_password_lab: "senha"
server_ip: "192.168.1.10"   # valor inicial do campo "Servidor IP"
```

Primeira gravação pela USB, depois OTA pela rede:

```bash
cd pc-button
../.venv/bin/esphome run --device /dev/ttyUSB0 pc-button.yaml   # USB
../.venv/bin/esphome run pc-button.yaml                          # OTA
../.venv/bin/esphome logs pc-button.yaml                         # logs pela rede
```
