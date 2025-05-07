// Autor: Levi Silva Freitas
// Data: 05/05/2025

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ssd1306.h"
#include "font.h"
#include "final.pio.h"

// --- Definições ---
#define BOTAO_MODO_NOTURNO 5
#define LED_VERDE 11
#define LED_AZUL 12
#define LED_VERMELHO 13
#define BUZZER 21
#define LED_MATRIX_PIN 7
#define I2C_SDA 14
#define I2C_SCL 15
#define I2C_PORT i2c1
#define I2C_ENDERECO 0x3C
#define NUM_LEDS 25

// --- Variáveis Globais ---
static volatile bool modoNoturnoAtivo = false;
static volatile uint64_t debounceUltimaInterrupcao = 0;
ssd1306_t displayOLED;
PIO pioControlador = pio0;
uint estadoMaquina;

// Define o canal PWM do buzzer
#define BUZZER_PWM_CHANNEL pwm_gpio_to_channel(BUZZER)

// --- Protótipos ---
bool inicializarSistema();
static void tratarInterrupcaoBotao(uint gpio, uint32_t eventos);
void configurarPWM(uint gpio);
void controlarRGBLed(bool vermelho, bool verde, bool azul);
void tocarBuzzer(uint frequencia, uint duracaoMs);
void pararBuzzer();
void atualizarDisplay(const char* estado);
void atualizarMatrizLED(float vermelho, float verde, float azul);
uint32_t calcularCorRGB(double azul, double vermelho, double verde);
void vSemaforo(void *params);

// --- Main ---
int main() {
    if (!inicializarSistema()) {
        printf("Falha na inicialização do sistema!\n");
        return 1;
    }

    // Criação da tarefa principal do semáforo
    xTaskCreate(vSemaforo, "Semaforo", 2048, NULL, 1, NULL);
    vTaskStartScheduler();

    while (true) {} // Nunca deve chegar aqui
    return 0;
}

// --- Funções ---

// Inicializa o sistema, incluindo GPIOs, I2C, PWM e PIO
bool inicializarSistema() {
    stdio_init_all();

    // Configuração do botão com interrupção
    gpio_init(BOTAO_MODO_NOTURNO);
    gpio_set_dir(BOTAO_MODO_NOTURNO, GPIO_IN);
    gpio_pull_up(BOTAO_MODO_NOTURNO);
    gpio_set_irq_enabled_with_callback(BOTAO_MODO_NOTURNO, GPIO_IRQ_EDGE_FALL, true, &tratarInterrupcaoBotao);

    // Configuração do I2C e do display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&displayOLED, WIDTH, HEIGHT, false, I2C_ENDERECO, I2C_PORT);
    ssd1306_config(&displayOLED);
    ssd1306_fill(&displayOLED, false);
    ssd1306_send_data(&displayOLED);

    // Configuração dos LEDs RGB e do buzzer via PWM
    configurarPWM(LED_VERDE);
    configurarPWM(LED_AZUL);
    configurarPWM(LED_VERMELHO);
    configurarPWM(BUZZER);

    // Configuração da matriz de LEDs via PIO
    extern const pio_program_t final_program;
    uint offset = pio_add_program(pioControlador, &final_program);
    estadoMaquina = pio_claim_unused_sm(pioControlador, true);
    final_program_init(pioControlador, estadoMaquina, offset, LED_MATRIX_PIN);

    return true;
}

// Configura um pino para operar em modo PWM
void configurarPWM(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice, 4095);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(gpio), 0);
    pwm_set_enabled(slice, true);
}

// Controla os LEDs RGB
void controlarRGBLed(bool vermelho, bool verde, bool azul) {
    pwm_set_gpio_level(LED_VERDE, verde ? 4095 : 0); // faz o LED verde acender, caso verde seja verdadeiro
    pwm_set_gpio_level(LED_AZUL, azul ? 4095 : 0); // faz o LED azul acender, caso azul seja verdadeiro
    pwm_set_gpio_level(LED_VERMELHO, vermelho ? 4095 : 0); // faz o LED vermelho acender, caso vermelho seja verdadeiro
}

// Toca o buzzer com uma frequência e duração específicas
void tocarBuzzer(uint frequencia, uint duracaoMs) {
    uint slice = pwm_gpio_to_slice_num(BUZZER);
    pwm_set_clkdiv(slice, 125.0f);
    pwm_set_wrap(slice, 125000 / frequencia);
    pwm_set_chan_level(slice, BUZZER_PWM_CHANNEL, (125000 / frequencia) / 2);
    vTaskDelay(pdMS_TO_TICKS(duracaoMs));
    pwm_set_chan_level(slice, BUZZER_PWM_CHANNEL, 0);
}

// Para o buzzer
void pararBuzzer() {
    uint slice = pwm_gpio_to_slice_num(BUZZER);
    pwm_set_chan_level(slice, BUZZER_PWM_CHANNEL, 0);
}

// Trata a interrupção do botão, alternando o modo noturno
static void tratarInterrupcaoBotao(uint gpio, uint32_t eventos) {
    uint64_t agora = to_us_since_boot(get_absolute_time());
    if ((agora - debounceUltimaInterrupcao) < 300000) return; // Debounce de 300ms
    debounceUltimaInterrupcao = agora;
    modoNoturnoAtivo = !modoNoturnoAtivo;
}

// Atualiza o display OLED com o estado atual
void atualizarDisplay(const char* estado) {
    ssd1306_fill(&displayOLED, false);
    ssd1306_draw_string(&displayOLED, "Estado ", 0, 0);
    ssd1306_draw_string(&displayOLED, estado, 50, 0);
    ssd1306_send_data(&displayOLED);
}

// Atualiza a matriz de LEDs com uma cor específica
void atualizarMatrizLED(float vermelho, float verde, float azul) {
    uint32_t cor = calcularCorRGB(azul, vermelho, verde);
    for (int i = 0; i < NUM_LEDS; i++) {
        pio_sm_put_blocking(pioControlador, estadoMaquina, cor);
    }
}

// Calcula a cor RGB para a matriz de LEDs
uint32_t calcularCorRGB(double azul, double vermelho, double verde) {
    unsigned char R = vermelho * 255;
    unsigned char G = verde * 255;
    unsigned char B = azul * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

void vSemaforo(void *params) {
    while (1) {
        if (!modoNoturnoAtivo) {
            // === VERDE ===
            atualizarDisplay("VERDE");
            controlarRGBLed(false, true, false);
            atualizarMatrizLED(0.0, 1.0, 0.0);
            for (int i = 0; i < 4; i++) { // 4 ciclos de 1000 ms
                tocarBuzzer(1000, 200);
                vTaskDelay(pdMS_TO_TICKS(800)); // 200ms buzzer + 800ms = 1000ms
            }

            // === AMARELO ===
            atualizarDisplay("AMARELO");
            controlarRGBLed(true, true, false);
            atualizarMatrizLED(1.0, 1.0, 0.0);
            for (int i = 0; i < 8; i++) { // 8 ciclos de 500 ms
                tocarBuzzer(500, 100);
                vTaskDelay(pdMS_TO_TICKS(400)); // 100ms buzzer + 400ms = 500ms
            }

            // === VERMELHO ===
            atualizarDisplay("VERMELHO");
            controlarRGBLed(true, false, false);
            atualizarMatrizLED(1.0, 0.0, 0.0);
            for (int i = 0; i < 2; i++) {
                tocarBuzzer(800, 500);
                vTaskDelay(pdMS_TO_TICKS(1500)); // 500 + 1500 = 2000 x 2 = 4000ms
            }
        } else {
            // === MODO NOTURNO ===
            atualizarDisplay("NOTURNO");
            controlarRGBLed(true, true, false);
            atualizarMatrizLED(1.0, 1.0, 0.0);
            tocarBuzzer(400, 2000);
            vTaskDelay(pdMS_TO_TICKS(2000));
            controlarRGBLed(false, false, false);
            atualizarMatrizLED(0.0, 0.0, 0.0);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}


