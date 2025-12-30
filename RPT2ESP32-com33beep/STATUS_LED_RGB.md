# Status do LED RGB - Resumo do Progresso

## 📊 Resumo Executivo

**Data:** 30 de Dezembro de 2025
**Placa:** ESP32-2432S028R (Cheap Yellow Display)
**Status:** PARCIALMENTE FUNCIONAL ⚠️

---

## ✅ O Que Está Funcionando (2/4)

| Estado | Cor | Status | Descrição |
|--------|-----|--------|-----------|
| **IDLE** | 🟢 Verde | ✅ FUNCIONANDO | LED verde quando a repetidora está em escuta |
| **WIFI** | 🔵 Azul | ✅ FUNCIONANDO | LED azul quando tela Wi-Fi está ativa |

## ⚠️ O Que NÃO Está Funcionando (2/4)

| Estado | Cor | Status | Descrição |
|--------|-----|--------|-----------|
| **TX (Morse/Voz)** | 🔴 Vermelho | ❌ NÃO FUNCIONA | LED não fica vermelho durante transmissão |
| **RX** | 🟡 Amarelo | ❌ NÃO TESTADO | LED não foi testado ainda durante RX |

---

## 🔧 Solução Aplicada

### Troca de PWM para digitalWrite()

**Problema Original:**
- LED iniciava com todas as cores acesas (luz branca)
- Isso acontecia porque PWM (`ledcAttach()`) iniciava com duty cycle 0 (LOW)
- Como o LED é Active Low, LOW = aceso

**Solução Implementada:**
- Substituí todo o sistema PWM por `digitalWrite()` simples
- Inicializa pinos como OUTPUT e HIGH (apagado) antes de qualquer controle
- Usa `digitalWrite(LOW)` para acender e `digitalWrite(HIGH)` para apagar

**Código Atual:**
```cpp
// Setup
pinMode(PIN_LED_R, OUTPUT);
pinMode(PIN_LED_G, OUTPUT);
pinMode(PIN_LED_B, OUTPUT);
digitalWrite(PIN_LED_R, HIGH);  // Inicializa apagado
digitalWrite(PIN_LED_G, HIGH);
digitalWrite(PIN_LED_B, HIGH);

// updateLED()
void updateLED() {
  if (show_ip_screen) {
    // Azul (WIFI)
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, HIGH);
    digitalWrite(PIN_LED_B, LOW);
  }
  else if (tx_mode != TX_NONE || ptt_state) {
    // Vermelho (TX) - PROBLEMA AQUI
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, HIGH);
    digitalWrite(PIN_LED_B, HIGH);
  }
  else if (cor_stable) {
    // Amarelo (RX)
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, HIGH);
  }
  else {
    // Verde (IDLE)
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, HIGH);
  }
}
```

---

## 🎯 Próximos Passos para Investigar

### 1. Verificar se tx_mode está sendo definido durante TX

**Locais onde tx_mode é definido:**
- Linha ~2910: `tx_mode = TX_VOICE;` (ID inicial)
- Linha ~2934: `tx_mode = TX_NONE;` (fim do ID)
- Linha ~2950: `tx_mode = TX_CW;` (ID CW)
- Linha ~2957: `tx_mode = TX_NONE;` (fim do CW)
- Linha ~2977: `tx_mode = TX_VOICE;` (ID a cada 11min)
- Linha ~3000: `tx_mode = TX_NONE;` (fim do ID voz)
- Linha ~3010: `tx_mode = TX_CW;` (ID CW a cada 16min)
- Linha ~3017: `tx_mode = TX_NONE;` (fim do CW)

**Hipótese:** O `tx_mode` pode estar sendo resetado para `TX_NONE` muito rápido, antes que o LED tenha tempo de atualizar.

### 2. Verificar se ptt_state está sendo ativado

**Função setPTT():**
- Linha ~1725: Define `ptt_state` e controla o pino PTT
- O código usa `digitalWrite(PIN_PTT, HIGH)` durante TX

**Hipótese:** Pode haver um problema de sincronização entre `tx_mode`, `ptt_state` e a chamada de `updateLED()`.

### 3. Adicionar Debug Logs

**Logs adicionados:**
```cpp
Serial.printf("[LED] Estado: %s | tx_mode=%d, ptt_state=%d, cor_stable=%d, show_ip_screen=%d\n",
              current_state == 1 ? "AZUL (WIFI)" :
              current_state == 2 ? "VERMELHO (TX)" :
              current_state == 3 ? "AMARELO (RX)" : "VERDE (IDLE)",
              tx_mode, ptt_state, cor_stable, show_ip_screen);
```

**O que verificar no Serial Monitor:**
- Durante TX: Esperamos ver `[LED] Estado: VERMELHO (TX) | tx_mode=1 ou 2, ptt_state=1`
- Se não mostrar isso, sabemos que a condição `tx_mode != TX_NONE || ptt_state` não está sendo verdadeira

---

## 💡 Possíveis Soluções para Testar

### Solução A: Chamar updateLED() imediatamente após definir tx_mode

```cpp
// Depois de tx_mode = TX_VOICE;
tx_mode = TX_VOICE;
updateLED();  // Chama imediatamente para atualizar LED
updateDisplay();
digitalWrite(PIN_PTT, HIGH);
```

### Solução B: Verificar se updateLED() está sendo chamada frequentemente

A função `updateLED()` é chamada no loop principal (linha ~2894), mas pode estar sendo limitada por algum throttle.

### Solução C: Adicionar delay pequeno após mudar tx_mode

```cpp
tx_mode = TX_VOICE;
delay(10);  // Pequeno delay para garantir atualização
updateDisplay();
```

---

## 📝 Conclusão

O sistema LED RGB está **50% funcional**. A mudança de PWM para `digitalWrite()` resolveu o problema do flash inicial e permitiu que os estados IDLE e WIFI funcionassem corretamente.

O próximo passo é investigar por que o estado TX (vermelho) não está funcionando. A hipótese principal é que há um problema de sincronização ou tempo entre a definição de `tx_mode`/`ptt_state` e a chamada de `updateLED()`.

**Recomendação:** Fazer upload do código atual e observar o Serial Monitor durante uma transmissão para ver quais valores são exibidos pelos logs de debug.


