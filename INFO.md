# Documentação Completa - Repetidora de Rádio Amador

Este arquivo contém todas as informações adicionais do projeto consolidadas.

---

## 📋 Índice
- [Histórico de Versões](#changelog)
- [Informações de Segurança](#segurança)
- [Guia Rápido de Instalação](#guia-rápido)
- [Autor e Contato](#autor)

---

## 📅 Changelog

### v2.1.0 (Atual)
- ✅ Sistema completo de LED RGB como indicador visual
- ✅ Debug logging avançado (NDJSON)
- ✅ Documentação completa em português

### v2.0.0
- ✅ Adaptação para ESP32-2432S028R (CYD)
- ✅ Driver ILI9341_2_DRIVER (elimina ghosting)
- ✅ Layout profissional com header, status e rodapé
- ✅ Áudio I2S para speaker onboard

### v1.0.0
- ✅ Versão inicial com funcionalidades básicas

---

## 🔒 Segurança

### Hardware
- Use **level shifters** ao conectar com rádios de 5V
- Mantenha **GND comum** entre todos os dispositivos
- Use **fontes de alimentação** estáveis

### Software
- Código totalmente **transparente e auditável**
- **NÃO coleta** dados de uso ou telemetria
- User tem **controle total** do dispositivo

### Reportar Vulnerabilidades
- Use o [GitHub Security Advisory](https://github.com/pantojinho/Repetidora_Radio_Amador/security/advisories)
- NÃO abra issues públicas para vulnerabilidades
- Seremos notificados e corrigiremos o problema

---

## 🚀 Guia Rápido

### Pré-requisitos
- ESP32-2432S028R (CYD)
- Arduino IDE 2.x
- Rádio com COR e PTT
- Cabo USB-C

### Instalação (5 minutos)

**1. Adicionar suporte ESP32:**
- Arduino IDE → File → Preferences
- URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- Board Manager → Instalar "esp32 by Espressif Systems"

**2. Instalar bibliotecas:**
- Library Manager → TFT_eSPI (Bodmer)
- Library Manager → XPT2046_Touchscreen

**3. Configurar TFT_eSPI:**
- Editar `Arduino/libraries/TFT_eSPI/User_Setup.h`
- Usar configurações do projeto (ver pasta `RPT2ESP32-com33beep/User_Setup.h`)

**4. Carregar o código:**
- Abrir `RPT2ESP32-com33beep.ino`
- Carregar no ESP32

### Conexão com Rádio

```
Rádio RX (COR) → GPIO22 (P3/CN1)
Rádio TX (PTT) → GPIO27 (CN1)
GND Comum       → GND
Speaker 8Ω      → JST 2-pin (GPIO26)
```

⚠️ **Importante**: Use level shifter se rádio é 5V+

### Uso
- **Toque na tela**: Troca courtesy tone
- **LED RGB**: Indica status (TX/RX/Idle)
- **Display**: Mostra estatísticas em tempo real

---

## 🤝 Como Contribuir

Quer contribuir? Fork, clone e faça um Pull Request:

```bash
git clone https://github.com/pantojinho/Repetidora_Radio_Amador.git
# Faça suas mudanças
git commit -m "Descrição clara"
git push origin main
```

- 🐛 Reportar bugs: [Issues](https://github.com/pantojinho/Repetidora_Radio_Amador/issues)
- 💡 Sugerir melhorias: [Issues](https://github.com/pantojinho/Repetidora_Radio_Amador/issues)
- 🛠️ Enviar código: [Pull Requests](https://github.com/pantojinho/Repetidora_Radio_Amador/pulls)

---

## 👤 Autor

**Gabriel Ciandrini** - **PU2PEG**

Radioamador brasileiro e desenvolvedor.

- 📻 **Indicativo**: PU2PEG
- 💻 **GitHub**: [pantojinho](https://github.com/pantojinho)
- 🌐 **Repositório**: [github.com/pantojinho/Repetidora_Radio_Amador](https://github.com/pantojinho/Repetidora_Radio_Amador)

### Contato
- 📧 Issues: [github.com/pantojinho/Repetidora_Radio_Amador/issues](https://github.com/pantojinho/Repetidora_Radio_Amador/issues)
- 💬 Discussions: [github.com/pantojinho/Repetidora_Radio_Amador/discussions](https://github.com/pantojinho/Repetidora_Radio_Amador/discussions)

---

## 📜 Licença

Este projeto está licenciado sob a [Licença MIT](LICENSE).

---

<div align="center">

**📡 Gabriel Ciandrini - PU2PEG**

73! 📻

</div>

