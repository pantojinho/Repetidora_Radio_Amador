# Problema: LED RGB ESP32-2432S028R - Status do Progresso

## 📋 Status Atual: PARCIALMENTE RESOLVIDO ✅⚠️

**Data:** 30 de Dezembro de 2025
**Versão do Código:** v2.3

## ✅ O Que Está Funcionando

1. **LED INICIA CORRETAMENTE**: O LED agora inicia completamente apagado (sem flash branco no boot)
2. **COR VERDE (IDLE)**: Funciona perfeitamente quando a repetidora está em escuta
3. **COR AZUL (WIFI)**: Funciona quando a tela de Wi-Fi está ativa (botão Wi-Fi pressionado)

## ⚠️ O Que Ainda Precisa Ser Investigado

1. **COR VERMELHA (TX)**: O LED NÃO está ficando vermelho durante:
   - Transmissão em Morse (TX_CW)
   - Transmissão em Voz (TX_VOICE)
   - Qualquer tipo de TX

## 🔍 Diagnóstico em Andamento

### Soluções Aplicadas até agora:

#### ✅ Solução 1: Mudança de PWM para digitalWrite()
**Status:** Funcional para estados IDLE e WIFI

**Problema Resolvido:**
- Antes: LED iniciava com todas as cores acesas (branco) devido ao PWM começar com duty cycle 0 (LOW)
- Agora: LED inicia apagado usando `digitalWrite(HIGH)`

**Código Implementado:**
```cpp
// Configuração (no setup)
pinMode(PIN_LED_R, OUTPUT);
pinMode(PIN_LED_G, OUTPUT);
pinMode(PIN_LED_B, OUTPUT);
digitalWrite(PIN_LED_R, HIGH);  // Apaga Vermelho
digitalWrite(PIN_LED_G, HIGH);  // Apaga Verde
digitalWrite(PIN_LED_B, HIGH);  // Apaga Azul

// Função updateLED()
void updateLED() {
  if (show_ip_screen) {
    // Azul (WIFI)
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, HIGH);
    digitalWrite(PIN_LED_B, LOW);
  }
  else if (tx_mode != TX_NONE || ptt_state) {
    // Vermelho (TX) - ESTA PARTE NÃO ESTÁ FUNCIONANDO
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
    // Verde (IDLE) - FUNCIONANDO
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, HIGH);
  }
}
```

### Próximo Passo: Adicionar Debug Logs

Adicionei logs na função `updateLED()` para investigar por que não está entrando no estado TX:

```cpp
Serial.printf("[LED] Estado: %s | tx_mode=%d, ptt_state=%d, cor_stable=%d, show_ip_screen=%d\n",
              current_state == 1 ? "AZUL (WIFI)" :
              current_state == 2 ? "VERMELHO (TX)" :
              current_state == 3 ? "AMARELO (RX)" : "VERDE (IDLE)",
              tx_mode, ptt_state, cor_stable, show_ip_screen);
```

## 🎯 Hipóteses para Investigar

1. **tx_mode não está sendo definido durante TX?**
   - Verificar se `tx_mode = TX_VOICE` e `tx_mode = TX_CW` estão sendo executados

2. **ptt_state não está sendo ativado?**
   - Verificar se `setPTT(true)` está sendo chamado durante TX

3. **Condição `else if` não está sendo avaliada corretamente?**
   - Possível problema de prioridade com outras condições

4. **Variáveis globais não estão sendo atualizadas?**
   - tx_mode pode estar sendo resetado antes da checagem do LED

## 📝 Especificações do Hardware (conforme documentação)

## Especificações do Hardware (conforme documentação)

### Pinout do LED RGB:
- **Red LED**: GPIO 4
- **Green LED**: GPIO 16  
- **Blue LED**: GPIO 17

### Características Importantes:
- **Lógica Invertida (Active Low)**: 
  - `HIGH` = LED **APAGADO**
  - `LOW` = LED **ACESO**
- Isso significa que para acender o LED, você deve escrever `LOW` (0) no pino
- Para apagar o LED, você deve escrever `HIGH` (1 ou 255)

## Implementação Atual no Código

### Configuração Atual:
```cpp
// Usando PWM via LEDC
ledc_channel_r = ledcAttach(PIN_LED_R, 5000, 8);  // GPIO 4
ledc_channel_g = ledcAttach(PIN_LED_G, 5000, 8);  // GPIO 16
ledc_channel_b = ledcAttach(PIN_LED_B, 5000, 8);  // GPIO 17

// Para acender: ledcWrite(canal, 0)   (LOW = acende)
// Para apagar: ledcWrite(canal, 255) (HIGH = apaga)
```

### Estados Esperados:
1. **Wi-Fi Ativo**: Azul (R=255, G=255, B=0)
2. **TX Ativo**: Vermelho (R=0, G=255, B=255)
3. **RX Ativo**: Amarelo (R=0, G=0, B=255)
4. **Idle**: Verde (R=255, G=0, B=255)

## Possíveis Causas e Soluções

### 1. Problema com PWM (LEDC) para LEDs Active Low

**Possível Causa**: O PWM pode não estar funcionando corretamente com LEDs active low, ou os canais podem não estar sendo configurados corretamente.

**Soluções a Testar**:

#### Solução A: Usar digitalWrite() ao invés de PWM
Para LEDs simples que só precisam ligar/desligar (não precisam de intensidade variável), use `digitalWrite()`:

```cpp
// Configuração
pinMode(PIN_LED_R, OUTPUT);
pinMode(PIN_LED_G, OUTPUT);
pinMode(PIN_LED_B, OUTPUT);

// Para acender: digitalWrite(pin, LOW)
// Para apagar: digitalWrite(pin, HIGH)

// Exemplo: Acender vermelho
digitalWrite(PIN_LED_R, LOW);   // Acende
digitalWrite(PIN_LED_G, HIGH);  // Apaga
digitalWrite(PIN_LED_B, HIGH);  // Apaga
```

#### Solução B: Configurar pinMode antes do ledcAttach
Alguns ESP32 podem precisar de `pinMode()` antes de configurar PWM:

```cpp
pinMode(PIN_LED_R, OUTPUT);
pinMode(PIN_LED_G, OUTPUT);
pinMode(PIN_LED_B, OUTPUT);

ledc_channel_r = ledcAttach(PIN_LED_R, 5000, 8);
ledc_channel_g = ledcAttach(PIN_LED_G, 5000, 8);
ledc_channel_b = ledcAttach(PIN_LED_B, 5000, 8);
```

#### Solução C: Usar ledcSetup() e ledcAttachPin() separadamente
Algumas versões do ESP32 Arduino podem precisar de configuração mais explícita:

```cpp
// Configurar canais LEDC manualmente
ledcSetup(0, 5000, 8);  // Canal 0, 5kHz, 8 bits
ledcSetup(1, 5000, 8);  // Canal 1
ledcSetup(2, 5000, 8);  // Canal 2

// Anexar pinos aos canais
ledcAttachPin(PIN_LED_R, 0);
ledcAttachPin(PIN_LED_G, 1);
ledcAttachPin(PIN_LED_B, 2);

// Usar ledcWrite com número do canal
ledcWrite(0, 255);  // Apaga vermelho
ledcWrite(1, 255);  // Apaga verde
ledcWrite(2, 255);  // Apaga azul
```

### 2. Verificar se os Canais estão sendo Inicializados

**Teste**: Adicionar logs para verificar se os canais estão sendo atribuídos corretamente:

```cpp
ledc_channel_r = ledcAttach(PIN_LED_R, 5000, 8);
Serial.printf("Canal R: %d\n", ledc_channel_r);
if (ledc_channel_r < 0) {
  Serial.println("ERRO: Canal R não inicializado!");
}
```

### 3. Teste Simples de Funcionamento

**Teste Básico**: Criar uma função de teste para verificar se os LEDs respondem:

```cpp
void testLED() {
  Serial.println("Testando LED RGB...");
  
  // Teste 1: Acender apenas vermelho
  digitalWrite(PIN_LED_R, LOW);
  digitalWrite(PIN_LED_G, HIGH);
  digitalWrite(PIN_LED_B, HIGH);
  delay(1000);
  
  // Teste 2: Acender apenas verde
  digitalWrite(PIN_LED_R, HIGH);
  digitalWrite(PIN_LED_G, LOW);
  digitalWrite(PIN_LED_B, HIGH);
  delay(1000);
  
  // Teste 3: Acender apenas azul
  digitalWrite(PIN_LED_R, HIGH);
  digitalWrite(PIN_LED_G, HIGH);
  digitalWrite(PIN_LED_B, LOW);
  delay(1000);
  
  // Apagar todos
  digitalWrite(PIN_LED_R, HIGH);
  digitalWrite(PIN_LED_G, HIGH);
  digitalWrite(PIN_LED_B, HIGH);
}
```

## Termos de Busca para Pesquisa

Use estes termos para pesquisar soluções na internet:

1. **"ESP32-2432S028R RGB LED active low not working"**
2. **"ESP32 ledcAttach active low LED inverted logic"**
3. **"ESP32 Cheap Yellow Display RGB LED control"**
4. **"ESP32 LEDC PWM active low LED always on"**
5. **"ESP32 digitalWrite vs ledcWrite active low LED"**
6. **"ESP32-2432S028R GPIO4 GPIO16 GPIO17 RGB LED"**
7. **"ESP32 RGB LED all colors on simultaneously"**
8. **"ESP32 ledcSetup ledcAttachPin active low"**

## Referências da Documentação

- **Fonte**: https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/#rgb-led
- **Pinos**: GPIO 4 (R), GPIO 16 (G), GPIO 17 (B)
- **Lógica**: Active Low (HIGH = OFF, LOW = ON)

## Próximos Passos Recomendados

1. **Testar com digitalWrite() primeiro** - Mais simples e direto para LEDs active low
2. **Verificar logs de inicialização** - Confirmar que os canais estão sendo criados
3. **Teste básico de hardware** - Garantir que os LEDs respondem a comandos simples
4. **Se digitalWrite funcionar**, considerar manter assim (mais simples e confiável)
5. **Se precisar de PWM** (intensidade variável), investigar configuração LEDC mais detalhada

## Nota Importante

Para LEDs active low simples que só precisam ligar/desligar (não precisam de intensidade variável), **digitalWrite() é geralmente mais confiável e simples** do que PWM. O PWM só é necessário se você quiser controlar a intensidade/brilho dos LEDs.

