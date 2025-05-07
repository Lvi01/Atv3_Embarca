# Semáforo Inteligente com Modo Noturno

## Descrição  
É um sistema embarcado desenvolvido para simular o funcionamento de um semáforo inteligente com modo noturno. Utilizando a placa Raspberry Pi Pico e periféricos como LEDs RGB, buzzer, display OLED e matriz de LEDs, o projeto representa os estados de trânsito com alertas visuais e sonoros, incluindo uma lógica alternativa para operação noturna controlada por botão físico.

## Objetivo  
O projeto tem como objetivo demonstrar o funcionamento de uma máquina de estados aplicada a sistemas de sinalização viária. Também busca servir como base para aplicações reais em controladores de tráfego urbano, abordando aspectos de segurança, usabilidade e operação em diferentes condições de luminosidade.

## Tecnologias e Hardware Utilizados  
- **Placa Microcontroladora**: Raspberry Pi Pico  
- **Protocolos de Comunicação**: I2C (display OLED), PIO (matriz de LEDs), PWM (controle de buzzer e LEDs RGB)  
- **Sensores e Entradas**: Botão físico para alternância de modo (dia/noturno) com debounce  
- **Atuadores**: LEDs RGB para representar os estados verde, amarelo e vermelho, matriz de LEDs 5x5 para reforço visual, e buzzer para alertas sonoros  
- **Interface com o Usuário**: Display OLED para indicar o estado atual do semáforo

## Estrutura do Projeto  
1. **Máquina de Estados**: Três estados principais (Verde, Amarelo e Vermelho), cada um com duração total de 4 segundos e subdivisões internas para alertas sonoros.  
2. **Modo Noturno**: Ativado por botão, substitui os estados padrão por pisca-pisca amarelo intermitente, reduzindo consumo e poluição visual.  
3. **Alertas Sonoros**: Cada estado possui padrões distintos de buzzer para auxiliar deficientes visuais ou ambientes com pouca visibilidade.  
4. **Atualização Visual**: Display OLED informa o estado atual, enquanto a matriz de LEDs exibe a cor correspondente.  
5. **Controle de Interrupção**: Debounce por software garante leitura estável do botão de alternância.  
6. **Temporização com FreeRTOS**: O controle dos ciclos é gerenciado por tarefas FreeRTOS, permitindo escalabilidade e precisão.

## Link do Projeto  
- **Demonstração no YouTube**: *Em breve*

## Autor  
Desenvolvido por Levi Silva Freitas.
