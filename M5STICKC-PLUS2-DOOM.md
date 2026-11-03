# Doom no M5StickC Plus2

Esta branch do fork [jfcbxp/retro-go](https://github.com/jfcbxp/retro-go/tree/feature/m5stickc-plus2-doom) adiciona um alvo para a M5StickC Plus2 e executa o PrBoom na tela ST7789 da placa. O firmware foi testado na placa conectada por USB; a demo de Doom agora atualiza a tela inteira.

## Hardware e arquitetura

- Placa: M5StickC Plus2, ESP32-PICO-V3-02, flash de 8 MB e PSRAM.
- LCD: ST7789, 135 x 240 pixels físicos; orientação lógica horizontal de 240 x 135.
- GPIO4 mantém a alimentação da placa ativa.
- LCD: SCLK 13, MOSI 15, DC 14, CS 5, RST 12, backlight 27.
- Botões usados pelo alvo: GPIO39, GPIO37 e GPIO35.
- O alvo da placa fica em `components/retro-go/targets/m5stickc-plus2/`.
- `prboom-go/main/main.c` inicializa o Retro-Go e inicia `demo1`.
- `components/retro-go/rg_display.c` converte e envia o framebuffer em blocos; cada bloco configura sua própria janela ST7789.
- `prboom-go/components/prboom/d_main.c` mapeia o IWAD da partição `doomwad` na flash.

Os arquivos `.wad`, imagens completas e saídas de build não são publicados. O WAD é dado externo: obtenha e use uma cópia que você tenha direito de usar.

## Requisitos

- Windows, Git e Python 3.
- ESP-IDF 5.1.x configurado para ESP32 (`IDF_TARGET=esp32`).
- Placa conectada por USB. No PC usado para validar, ela apareceu como `COM4`; confirme a porta no Gerenciador de Dispositivos.
- Para executar o jogo, um `doom1.wad` válido. A compilação do aplicativo não inclui nem baixa o WAD.

## Obter esta branch

```powershell
Set-Location "$env:USERPROFILE\Documents"
git clone --branch feature/m5stickc-plus2-doom https://github.com/jfcbxp/retro-go.git m5stickc-plus2-doom
Set-Location "$env:USERPROFILE\Documents\m5stickc-plus2-doom"
```

## Preparar o terminal PowerShell

Abra um PowerShell novo e carregue o ambiente ESP-IDF. Ajuste `IDF_PATH` se o ESP-IDF estiver instalado em outro local:

```powershell
$env:IDF_PATH = 'C:\Espressif\esp-idf'
$env:IDF_TARGET = 'esp32'

# No ambiente usado para validar este projeto, o Python do ESP-IDF precisa
# preceder o alias Python da Microsoft Store no PATH.
$pythonEnv = "$env:USERPROFILE\.espressif\python_env\idf5.1_py3.13_env"
$env:Path = "$pythonEnv\Scripts;$env:Path"

. "$env:IDF_PATH\export.ps1"
idf.py --version
Set-Location "$env:USERPROFILE\Documents\m5stickc-plus2-doom"
```

Se a instalação do ESP-IDF usa outro ambiente Python, substitua `$pythonEnv` pelo diretório `Scripts` correspondente. O arquivo `components/retro-go/targets/m5stickc-plus2/env.py` define o alvo ESP32 para o `rg_tool.py`.

## Compilar o aplicativo

```powershell
python .\rg_tool.py --target m5stickc-plus2 build prboom-go
```

O binário gerado fica em:

```text
prboom-go\build\prboom-go.bin
```

Para fazer uma compilação limpa, opcionalmente execute antes:

```powershell
python .\rg_tool.py --target m5stickc-plus2 clean prboom-go
python .\rg_tool.py --target m5stickc-plus2 build prboom-go
```

## Atualizar uma placa já provisionada

Substitua `COM4` pela porta USB atual:

```powershell
python .\rg_tool.py --target m5stickc-plus2 --port COM4 --baud 460800 flash prboom-go
```

Esse comando atualiza somente a partição do aplicativo. Use-o para iterações normais: ele preserva as partições de dados, incluindo o IWAD `doomwad`.

Para acompanhar o boot e as mensagens do jogo:

```powershell
python .\rg_tool.py --target m5stickc-plus2 --port COM4 monitor prboom-go
```

Interrompa o monitor serial com `Ctrl+C`.

## Provisionar uma placa do zero

O comando `install` gera e grava uma imagem completa, incluindo a tabela de partições. Ele substitui o firmware e os dados abrangidos pela imagem; não o use para uma atualização comum nem em uma placa cujos dados devam ser preservados.

Defina o caminho para o WAD válido que você forneceu:

```powershell
$wad = 'C:\caminho\para\doom1.wad'
if (-not (Test-Path $wad)) { throw "WAD não encontrado: $wad" }
```

Gere e grave a imagem completa:

```powershell
python .\rg_tool.py --target m5stickc-plus2 --port COM4 --baud 460800 --fatsize 1M --wadfile $wad install prboom-go
```

`--wadfile` cria a partição `doomwad`; o tamanho é calculado a partir do arquivo. `--fatsize 1M` reserva a partição `vfs` usada por saves e arquivos do Retro-Go. O arquivo `.img` resultante é local e não deve ser enviado ao GitHub.

## Verificação do boot

No monitor serial, procure mensagens de inicialização do Retro-Go e do PrBoom, incluindo a identificação do IWAD e o início da demo `demo1`. A tela deve continuar atualizando durante a demo.

Se a placa não aparecer:

1. Confirme a porta com `Get-PnpDevice -PresentOnly -Class Ports`.
2. Feche outros monitores seriais antes de gravar ou monitorar.
3. Confirme que o ambiente ESP-IDF foi carregado no mesmo PowerShell usado para executar `rg_tool.py`.
4. Se a atualização do aplicativo falhar, não use `install` como tentativa automática: ele regrava a imagem inteira.

## Estrutura do projeto

| Caminho | Responsabilidade |
|---|---|
| `components/retro-go/targets/m5stickc-plus2/` | Pinos, tela, rotação e configuração ESP-IDF da placa |
| `components/retro-go/drivers/display/ili9341.h` | Transporte SPI, inicialização do painel e backlight |
| `components/retro-go/rg_display.c` | Conversão e atualização do framebuffer por blocos |
| `prboom-go/main/main.c` | Inicialização do app, superfície de vídeo e argumentos de Doom |
| `prboom-go/components/prboom/d_main.c` | Inicialização do PrBoom e leitura do IWAD mapeado na flash |
| `rg_tool.py` | Compilação, montagem da imagem e gravação do app ou da imagem completa |

O `BUILDING.md` na raiz descreve os comandos genéricos do Retro-Go; este guia contém os comandos específicos da M5StickC Plus2.
