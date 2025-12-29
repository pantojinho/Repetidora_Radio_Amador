# Repetidora de Rádio Amador - ESP32-2432S028 (CYD)

<div align="center">

![ESP32-2432S028](https://img.shields.io/badge/ESP32-2432S028-CYD-blue)
![Arduino](https://img.shields.io/badge/Arduino-IDE-orange)
![License](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Status-Stable-brightgreen)

**Sistema completo de repetidora de rádio amador com interface gráfica**

</div>

---

## 📖 Sobre

Este projeto implementa uma repetidora de rádio amador moderna baseada no microcontrolador **ESP32-WROOM-32** com o display **ESP32-2432S028R** (conhecido como "Cheap Yellow Display" ou CYD). A repetidora possui uma interface visual TFT colorida com touchscreen, sistema de courtesy tones audíveis e indicador de status por LED RGB.

### Principais Características

- 🖥️ **Display TFT 2.8"** ILI9341 (320x240 pixels) com orientação paisagem
- 👆 **Touchscreen resistivo** para seleção de courtesy tones
- 🎵 **33 Courtesy Tones** diferentes selecionáveis
- 🎨 **LED RGB** com indicador visual de status em tempo real:
  - 🟢 **Verde/Amarelo pulsante**: Recebendo sinal (RX)
  - 🔴 **Vermelho fixo**: Transmitindo (TX)
  - 🌈 **Rainbow**: Em espera (Idle)
- 📊 **Display informativo** com estatísticas em tempo real
- 🔊 **Áudio I2S** para reproduction de courtesy tones no speaker onboard
- ⚡ **Otimizações de performance**: Display sem flicker, atualizações parciais
- 📝 **Sistema de logging** em NDJSON para análise offline

---

## 🎯 Funcionalidades

### Controle de Repetidora
- Detecção automática de sinal (COR - Squelch Detection)
- Ativação/desativação de PTT (Push-to-Talk)
- Hang time configurável (600ms)
- Contador de QSOs

### Interface Visual
- Header com callsign customizável
- Status em tempo real (EM ESCUTA / RX ATIVO / TX ATIVO)
- Seleção visual de courtesy tone
- Estatísticas: QSOs, Uptime, Índice do CT
- Barra de progresso durante transmissão
- Touchscreen com debounce para evitar trocas acidentais

### Indicador LED RGB
- Sistema completo de feedback visual
- Transições suaves via PWM (5kHz, 8 bits)
- Efeitos: breathing e rainbow cíclico

---

## 🔧 Hardware Necessário

### Obrigatório
- Placa **ESP32-2432S028R** (Cheap Yellow Display)
- Rádio transceptor com acesso a:
  - Saída de squelch/COR
  - Entrada de PTT
- Speaker 8Ω 1-3W (opcional, para áudio onboard)

### Opcional (Recomendado)
- **Level shifter** ou optocoupler (se rádio usar 5V)
- Cabos JST 2-pin para conexão do speaker

---

## 📦 Configuração de Pinagem

### Display TFT (SPI)
| Função | GPIO | Descrição |
|--------|------|-----------|
| TFT_MISO | 12 | Master In Slave Out |
| TFT_MOSI | 13 | Master Out Slave In |
| TFT_SCLK | 14 | Serial Clock |
| TFT_CS | 15 | Chip Select |
| TFT_DC | 2 | Data/Command |
| TFT_RST | -1 | Reset (ligado ao EN) |
| TFT_BL | 21 | Backlight |

### Repetidora (Extended IO)
| Função | GPIO | Conector | Descrição |
|--------|------|----------|-----------|
| PIN_COR | 22 | P3/CN1 | Entrada COR (squelch detection) |
| PIN_PTT | 27 | CN1 | Saída PTT (push-to-talk) |
| SPEAKER | 26 | JST 2-pin | Speaker onboard (I2S) |

### LED RGB
| Função | GPIO | Descrição |
|--------|------|-----------|
| LED_R | 4 | LED Vermelho |
| LED_G | 16 | LED Verde |
| LED_B | 17 | LED Azul |

> ⚠️ **Importante**: GPIO16/17 são do LED RGB - NÃO usar para COR/PTT para evitar conflitos

---

## 🚀 Instalação

### 1. Instalar o Arduino IDE
Baixe e instale a versão mais recente do [Arduino IDE](https://www.arduino.cc/en/software).

### 2. Adicionar suporte ESP32
1. Abra Arduino IDE
2. Vá em `File > Preferences`
3. Adicione a URL no campo "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Vá em `Tools > Board > Boards Manager`
5. Pesquise por "esp32" e instale o pacote

### 3. Instalar bibliotecas necessárias
Instale as seguintes bibliotecas via `Sketch > Include Library > Manage Libraries`:

- **TFT_eSPI** (por Bodmer)
- **XPT2046_Touchscreen**

### 4. Configurar TFT_eSPI
O arquivo `User_Setup.h` da biblioteca TFT_eSPI deve ser configurado assim:

```cpp
#define ILI9341_2_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1
#define TFT_BL   21
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000
#define SPI_USE_HW_SPI
```

> **Localização**: `Arduino/libraries/TFT_eSPI/User_Setup.h`

### 5. Carregar o código
1. Clone este repositório ou baixe o ZIP
2. Abra o arquivo `RPT2ESP32-com33beep.ino` no Arduino IDE
3. Selecione a placa: `ESP32 Dev Module` ou `ESP32-2432S028`
4. Conecte o ESP32 via USB
5. Carregue o código (`Sketch > Upload`)

---

## 🔌 Conexões com o Rádio

### ⚠️ Avisos Importantes
- **Level Shifter OBRIGATÓRIO**: Se o rádio usar 5V, SEMPRE use level shifter ou optocoupler
- **GND Comum**: Conecte GND comum entre CYD, rádio RX e TX
- **Teste com Multímetro**: Verifique todas as conexões antes de ligar

### Diagrama de Conexão
```
Rádio RX (COR)  ──[Level Shifter]── GPIO22 (P3/CN1) ── ESP32
Rádio TX (PTT)  ──[Level Shifter]── GPIO27 (CN1)    ── ESP32
GND Comum       ──────────────────── GND (P3/CN1)    ── ESP32
Speaker 8Ω      ──────────────────── JST 2-pin      ── GPIO26
```

### 1. COR (Squelch Detection)
- Conecte a saída de squelch do rádio ao GPIO22
- Configuração: `pinMode(PIN_COR, INPUT_PULLUP)`
- Funcionamento: LOW quando há sinal detectado

### 2. PTT (Push-to-Talk)
- Conecte a entrada de PTT do rádio ao GPIO27
- Configuração: `pinMode(PIN_PTT, OUTPUT)`
- Funcionamento: HIGH ativa transmissão

### 3. Speaker (Opcional)
- Conecte speaker 8Ω 1-3W ao conector JST 2-pin
- Controlado via GPIO26 (I2S)
- Reproduz courtesy tones após cada QSO

---

## 🎮 Como Usar

### Operação Básica

1. **Ligar a repetidora**: Conecte via USB, o display mostrará o status inicial
2. **EM ESCUTA**: A repetidora está em espera (LED rainbow)
3. **Quando alguém transmitir**:
   - O COR detecta o sinal
   - PTT é ativado automaticamente
   - LED muda para amarelo pulsante (RX)
   - Display mostra "RX ATIVO"
4. **Após o término da transmissão**:
   - Hang time de 600ms
   - Courtesy tone é tocado
   - LED volta para rainbow
5. **Trocar Courtesy Tone**:
   - Toque em qualquer lugar da tela
   - O CT avança para o próximo (circular 1-33)
   - Display atualiza mostrando o novo CT

### Lista de Courtesy Tones (33)

1. Boop
2. Beep
3. Apollo
4. Moonbounce
5. Tumbleweed
6. Bee-Boo
7. Bumble Bee
8. YellowJacket
9. ShootingStar
10. Comet
11. Stardust
12. Duncecap
13. Piano Chord
14. NBC
15. 3up
16. TelRing
17. BlastOff
18. Water Drop
19. Whippoorwhill
20. Sat Pass
21. OverHere
22. Nextel
23-31. RC210-1 a RC210-9
32. XP Error
33. XP OK

---

## 📊 Estatísticas no Display

| Coluna | Descrição |
|--------|-----------|
| QSOs | Número total de QSOs completados |
| Uptime | Tempo de operação (hh:mm) |
| CT | Índice do courtesy tone atual |

---

## 🛠️ Personalização

### Alterar Callsign
No arquivo `.ino`, modifique:
```cpp
const char* CALLSIGN = "PY2KEP SP";  // Altere para seu indicativo
// Exemplos: "PU2ABC", "PY1XYZ", "PU2PEG SP"
```

### Ajustar Volume
```cpp
float VOLUME = 0.70f;  // 0.0 a 1.0
```

### Modificar Hang Time
```cpp
#define HANG_TIME_MS 600  // Tempo em milissegundos após COR desativar
```

### Ajustar Frequência SPI
No `User_Setup.h`:
```cpp
#define SPI_FREQUENCY  27000000  // 27MHz - mais estável
// #define SPI_FREQUENCY  40000000  // 40MHz - mais rápido (pode causar artifacts)
```

---

## 🐛 Troubleshooting

### Display em branco
- ✅ Verifique se `User_Setup.h` está configurado corretamente
- ✅ Verifique pinos SPI (12, 13, 14, 15, 2, 21)
- ✅ Verifique backlight (GPIO 21)

### Touchscreen não funciona
- ✅ Verifique biblioteca XPT2046_Touchscreen instalada
- ✅ Verifique pino CS=33

### LED RGB não acende
- ✅ Verifique pinos: R=4, G=16, B=17
- ✅ Anodo comum: LOW acende, HIGH apaga

### Layout cortado ou virado
- ✅ Rotação deve ser 3 (paisagem)
- ✅ Resolução: 320x240
- ✅ Use ILI9341_2_DRIVER

### Ghosting na tela
- ✅ Use ILI9341_2_DRIVER (não ILI9341_DRIVER)
- ✅ Frequência SPI: 27MHz
- ✅ TFT_INVERSION_ON ativado

---

## 📚 Documentação

Para informações técnicas detalhadas, consulte:
- [`DOCUMENTACAO_ESP32-2432S028.md`](RPT2ESP32-com33beep/DOCUMENTACAO_ESP32-2432S028.md) - Documentação completa em português
- [`RPT2ESP32-com33beep.ino`](RPT2ESP32-com33beep/RPT2ESP32-com33beep.ino) - Código fonte com comentários detalhados

---

## 📝 Changelog

### v2.1 (Atual)
- ✅ LED RGB completo como indicador de status
- ✅ Controle via PWM (5kHz, 8 bits)
- ✅ Sistema de Debug Logging Avançado (NDJSON)
- ✅ Documentação completa em português

### v2.0 (Adaptado para CYD)
- ✅ Pins adaptados: GPIO22 (COR), GPIO27 (PTT), GPIO26 (Speaker)
- ✅ Driver ILI9341_2_DRIVER (elimina ghosting)
- ✅ Rotação 3 (landscape horizontal)
- ✅ Layout profissional com header, status central, rodapé
- ✅ Bordas arredondadas e cores dinâmicas
- ✅ Touchscreen com debounce melhorado
- ✅ Áudio I2S para speaker onboard

### v1.0 (Original)
- Layout básico para 320x240
- Suporte para touchscreen XPT2046
- LED RGB integrado
- Barra de progresso PTT

---

## 🤝 Contribuindo

Contribuições são bem-vindas! Sinta-se à vontade para:
- Reportar bugs
- Sugerir novas funcionalidades
- Enviar pull requests
- Melhorar a documentação

### Como contribuir
1. Fork este repositório
2. Crie uma branch para sua feature (`git checkout -b feature/MinhaFeature`)
3. Commit suas mudanças (`git commit -m 'Adiciona nova feature'`)
4. Push para a branch (`git push origin feature/MinhaFeature`)
5. Abra um Pull Request

---

## 👨‍💻 Autor

**Gabriel Ciandrini** - **PU2PEG**

Radioamador brasileiro e desenvolvedor de projetos para a comunidade.

📍 **Localização**: Brasil  
📻 **Indicativo**: PU2PEG  
💻 **GitHub**: [pantojinho](https://github.com/pantojinho)

### Sobre o Projeto

Desenvolvido como um projeto open source para a comunidade de rádio amador, com foco em:

- Transparência de código (totalmente auditável)
- Documentação detalhada em português
- Interface visual moderna e profissional
- Fácil de configurar e usar

### Contato

Para questões sobre o projeto:
- 📧 GitHub Issues: [pantojinho/Repetidora_Radio_Amador/issues](https://github.com/pantojinho/Repetidora_Radio_Amador/issues)
- 💬 GitHub Discussions: [pantojinho/Repetidora_Radio_Amador/discussions](https://github.com/pantojinho/Repetidora_Radio_Amador/discussions)

---

## 📄 Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.

---

## 🙏 Agradecimentos

- **Bodmer** pela biblioteca TFT_eSPI
- Comunidadade ESP32 e Arduino
- Comunidadade de rádio amador
- A todos que testaram e deram feedback

---

<div align="center">

**📡 Gabriel Ciandrini - PU2PEG**

Feito com ❤️ para a comunidade de rádio amador

[GitHub](https://github.com/pantojinho) | [Repositório](https://github.com/pantojinho/Repetidora_Radio_Amador)

</div>

