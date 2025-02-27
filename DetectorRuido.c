#include <stdio.h>
#include <math.h> 
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "pico/bootrom.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "ws2812.pio.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

// Definições de pinos
const uint MICROPHONE = 28; // Microfone conectado ao GPIO28 (ADC2)
const uint BTN_B_PIN = 6;   // Botão B conectado ao GPIO6
const uint BTN_A_PIN = 5;   // Botão A conectado ao GPIO5
const uint JOYSTICK_X = 26; // Eixo X do joystick no GPIO26 (ADC0)
const uint JOYSTICK_Y = 27; // Eixo Y do joystick no GPIO27 (ADC1)
const uint BUZZER_PIN = 21; // Buzzer conectado ao GPIO21
#define WS2812_PIN 7        // Pino da matriz de LEDs WS2812
#define NUM_PIXELS 25       // Número de pixels na matriz 5x5
#define I2C_PORT i2c1       // Porta I2C para o display SSD1306
#define I2C_SDA 14          // Pino SDA do SSD1306
#define I2C_SCL 15          // Pino SCL do SSD1306
#define SSD1306_ADDR 0x3C   // Endereço I2C do SSD1306

// Configurações de amostragem e debounce
const uint SAMPLES_PER_SECOND = 8000; // Taxa de amostragem de 8 kHz para o microfone
const uint DEBOUNCE_DELAY = 200;      // Atraso de debounce em milissegundos para os botões
const uint ADC_MAX_VALUE = 4094;      // Limite máximo ajustado para 4094

// Configurações do buzzer
#define BUZZER_FREQ_HZ 2000 // Frequência do buzzer em Hz
#define DOT_TIME 200        // Duração de um ponto no SOS (ms)
#define DASH_TIME 800       // Duração de um traço no SOS (ms)
#define GAP_TIME 125        // Pausa entre sinais no SOS (ms)
#define LETTER_GAP 250      // Pausa entre letras no SOS (ms)
#define CYCLE_GAP 3000      // Pausa entre ciclos completos de SOS (ms)

// Variáveis globais
volatile bool button_b_pressed = false; // Estado do botão B (pressionado ou não)
volatile bool button_a_pressed = false; // Estado do botão A (pressionado ou não)
uint32_t last_button_b_time = 0;        // Último tempo de pressão do botão B (ms)
uint32_t last_button_a_time = 0;        // Último tempo de pressão do botão A (ms)
bool out_of_range = false;              // Indica se o sinal do microfone está fora do range
bool program_running = false;           // Indica se o programa está no modo de execução
int threshold_min = 0;                  // Limite mínimo do range de detecção
int threshold_max = 0;                  // Limite máximo do range de detecção
int step = 0;                           // Etapa atual do programa (0 a 3)
int digit_pos = 0;                      // Posição do dígito sendo ajustado no range
int digits_min[3] = {0, 0, 0};          // Dígitos do valor mínimo (centena, dezena, unidade)
int digits_max[4] = {0, 0, 0, 0};       // Dígitos do valor máximo (milhar, centena, dezena, unidade)
PIO pio = pio0;                         // Instância PIO para controle dos LEDs WS2812
int sm = 0;                             // Máquina de estado para PIO
uint32_t led_buffer[NUM_PIXELS] = {0};  // Buffer para os estados dos LEDs WS2812
ssd1306_t ssd;                          // Estrutura para controle do display SSD1306

// Funções para controle dos LEDs WS2812
static inline void put_pixel(uint32_t pixel_grb)
{
    // Envia um pixel no formato GRB para a matriz WS2812 via PIO
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    // Converte valores RGB (0-255) em um único valor de 32 bits no formato GRB
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void set_leds_from_buffer()
{
    // Atualiza todos os LEDs da matriz WS2812 com base no buffer
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        put_pixel(led_buffer[i]);
    }
}

void set_all_leds(uint8_t r, uint8_t g, uint8_t b)
{
    // Define todos os LEDs da matriz WS2812 com a mesma cor RGB e atualiza
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        led_buffer[i] = urgb_u32(r, g, b);
    }
    set_leds_from_buffer();
}

// Função para entrar no modo BOOTSEL
void enter_bootsel()
{
    set_all_leds(0, 0, 0);  // Desliga todos os LEDs para indicar reinicialização
    set_leds_from_buffer(); // Força a atualização dos LEDs
    sleep_ms(50);           // Pausa de 50 ms para garantir que os LEDs sejam apagados
    reset_usb_boot(0, 0);   // Reinicia o Pico no modo BOOTSEL para reprogramação
}

// Função de debounce para botões
bool debounce_button(uint32_t *last_time)
{
    // Implementa debounce verificando se passou tempo suficiente desde a última pressão
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - *last_time >= DEBOUNCE_DELAY)
    {
        *last_time = current_time;
        return true; // Botão válido
    }
    return false; // Botão ignorado (debounce ativo)
}

// Manipulador de interrupções dos botões
void button_isr_handler(uint gpio, uint32_t events)
{
    if (gpio == BTN_B_PIN && events & GPIO_IRQ_EDGE_FALL)
    {
        // Detecta borda de descida no botão B e aplica debounce
        if (debounce_button(&last_button_b_time))
        {
            button_b_pressed = true;
        }
    }
    if (gpio == BTN_A_PIN && events & GPIO_IRQ_EDGE_FALL)
    {
        // Detecta borda de descida no botão A e aplica debounce
        if (debounce_button(&last_button_a_time))
        {
            button_a_pressed = true;
        }
    }
}

// Configuração das interrupções dos botões
void setup_button_interrupts()
{
    gpio_init(BTN_B_PIN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_pull_up(BTN_B_PIN); // Configura pull-up interno no botão B

    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN); // Configura pull-up interno no botão A

    // Habilita interrupções de borda de descida para ambos os botões
    gpio_set_irq_enabled_with_callback(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr_handler);
    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr_handler);
}

// Inicialização do display SSD1306
void setup_ssd1306()
{
    i2c_init(I2C_PORT, 400 * 1000); // Inicializa I2C a 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL); // Configura pull-ups internos para SDA e SCL

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, SSD1306_ADDR, I2C_PORT); // Inicializa o SSD1306
    ssd1306_config(&ssd);                                             // Aplica configurações padrão
}

// Função para desenhar dígitos invertidos no display
void draw_inverted_digit(ssd1306_t *ssd, char digit, uint8_t x, uint8_t y)
{
    // Preenche um quadrado 8x8 com fundo preto
    for (uint8_t i = 0; i < 8; i++)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
            ssd1306_pixel(ssd, x + i, y + j, true);
        }
    }

    // Desenha o dígito em branco sobre o fundo preto usando a fonte
    uint16_t index = (digit - '0' + 1) * 8;
    for (uint8_t i = 0; i < 8; ++i)
    {
        uint8_t line = font[index + i];
        for (uint8_t j = 0; j < 8; ++j)
        {
            bool pixel_value = (line & (1 << j));
            if (pixel_value)
            {
                ssd1306_pixel(ssd, x + i, y + j, false);
            }
        }
    }
}

// Funções do buzzer
void start_buzzer(uint32_t duration_ms)
{
    // Gera um tom no buzzer com a frequência definida por BUZZER_FREQ_HZ
    uint32_t period = 1000000 / BUZZER_FREQ_HZ;
    uint32_t half_period = period / 2;
    uint32_t end_time = time_us_32() + (duration_ms * 1000);

    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    while (time_us_32() < end_time)
    {
        if (button_a_pressed || button_b_pressed)
        {
            gpio_put(BUZZER_PIN, 0); // Interrompe o buzzer se um botão for pressionado
            return;
        }
        gpio_put(BUZZER_PIN, 1);
        sleep_us(half_period);
        gpio_put(BUZZER_PIN, 0);
        sleep_us(half_period);
    }
    gpio_put(BUZZER_PIN, 0); // Garante que o buzzer esteja desligado ao final
}

void send_sos_buzzer()
{
    // Emite o padrão SOS (3 pontos, 3 traços, 3 pontos) no buzzer
    for (int i = 0; i < 3; i++)
    {
        start_buzzer(DOT_TIME);
        if (button_a_pressed || button_b_pressed) return;
        sleep_ms(GAP_TIME);
    }
    sleep_ms(LETTER_GAP);
    if (button_a_pressed || button_b_pressed) return;

    for (int i = 0; i < 3; i++)
    {
        start_buzzer(DASH_TIME);
        if (button_a_pressed || button_b_pressed) return;
        sleep_ms(GAP_TIME);
    }
    sleep_ms(LETTER_GAP);
    if (button_a_pressed || button_b_pressed) return;

    for (int i = 0; i < 3; i++)
    {
        start_buzzer(DOT_TIME);
        if (button_a_pressed || button_b_pressed) return;
        sleep_ms(GAP_TIME);
    }
}

// Atualização do display SSD1306
void update_display()
{
    char buffer[32];
    ssd1306_fill(&ssd, false); // Limpa o display

    switch (step)
    {
    case 0: // Tela inicial
        ssd1306_draw_string(&ssd, "Detector", 0, 0);
        ssd1306_draw_string(&ssd, "De Ruido", 0, 10);
        ssd1306_draw_string(&ssd, "Iniciado", 0, 20);
        ssd1306_draw_string(&ssd, "A: Prosseguir", 0, 40);
        break;
    case 1: // Configuração do valor mínimo
        snprintf(buffer, sizeof(buffer), "Min: %d%d%d", digits_min[0], digits_min[1], digits_min[2]);
        ssd1306_draw_string(&ssd, buffer, 0, 0);
        draw_inverted_digit(&ssd, digits_min[digit_pos] + '0', 40 + digit_pos * 8, 0);
        ssd1306_draw_string(&ssd, "X: mais:menos", 0, 10);
        ssd1306_draw_string(&ssd, "Y: digito", 0, 20);
        ssd1306_draw_string(&ssd, "A: Prosseguir", 0, 40);
        break;
    case 2: // Configuração do valor máximo
        snprintf(buffer, sizeof(buffer), "Max: %d%d%d%d", digits_max[0], digits_max[1], digits_max[2], digits_max[3]);
        ssd1306_draw_string(&ssd, buffer, 0, 0);
        draw_inverted_digit(&ssd, digits_max[digit_pos] + '0', 40 + digit_pos * 8, 0);
        ssd1306_draw_string(&ssd, "X: mais:menos", 0, 10);
        ssd1306_draw_string(&ssd, "Y: digito", 0, 20);
        ssd1306_draw_string(&ssd, "A: Prosseguir", 0, 40);
        break;
    case 3: // Modo de execução
        snprintf(buffer, sizeof(buffer), "Min:%03d", threshold_min);
        ssd1306_draw_string(&ssd, buffer, 0, 0);
        snprintf(buffer, sizeof(buffer), "Max:%04d", threshold_max);
        ssd1306_draw_string(&ssd, buffer, 0, 8);
        ssd1306_draw_string(&ssd, "Monitoramento", 0, 40);
        ssd1306_draw_string(&ssd, "Iniciado", 0, 50); 
        break;
    }
    ssd1306_send_data(&ssd); // Envia os dados para o display
}

int main()
{
    stdio_init_all(); // Inicializa comunicação serial padrão

    // Inicializa o ADC para microfone e joystick
    adc_init();
    adc_gpio_init(MICROPHONE); // Configura GPIO28 como entrada analógica para o microfone
    adc_gpio_init(JOYSTICK_X); // Configura GPIO26 como entrada analógica para o eixo X do joystick
    adc_gpio_init(JOYSTICK_Y); // Configura GPIO27 como entrada analógica para o eixo Y do joystick

    // Inicializa a matriz WS2812
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    // Inicializa o display SSD1306
    setup_ssd1306();

    // Inicializa o buzzer como saída
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    setup_button_interrupts(); // Configura interrupções para os botões

    // Exibe a tela inicial e define LEDs azuis para indicar modo de configuração
    update_display();
    set_all_leds(0, 0, 10); // LEDs azuis

    // Define o intervalo de amostragem do microfone (125 µs para 8 kHz)
    uint64_t interval_us = 1000000 / SAMPLES_PER_SECOND;

    while (true)
    {
        // Trata o botão B para entrar no modo BOOTSEL
        if (button_b_pressed)
        {
            button_b_pressed = false;
            set_all_leds(0, 0, 0);  // Desliga os LEDs
            set_leds_from_buffer(); // Força a atualização
            sleep_ms(50);           // Pausa para garantir que os LEDs sejam apagados
            ssd1306_fill(&ssd, false);
            ssd1306_send_data(&ssd); // Limpa o display
            gpio_put(BUZZER_PIN, 0); // Desliga o buzzer
            enter_bootsel();         // Entra no modo BOOTSEL
        }

        // Lê os valores analógicos do joystick
        adc_select_input(0); // Seleciona ADC0 (eixo X do joystick)
        uint16_t joy_x = adc_read(); // Valor de 0 a 4095
        adc_select_input(1); // Seleciona ADC1 (eixo Y do joystick)
        uint16_t joy_y = adc_read(); // Valor de 0 a 4095

        if (step < 3) // Etapas de configuração
        {
            set_all_leds(0, 0, 10); // Mantém LEDs azuis durante a configuração
            update_display();        // Atualiza o display com o estado atual

            // Trata o botão A para avançar entre as etapas
            if (button_a_pressed)
            {
                button_a_pressed = false;
                step++;
                digit_pos = 0; // Reseta a posição do dígito
                if (step == 3)
                {
                    // Converte os dígitos em valores inteiros para o range
                    threshold_min = digits_min[0] * 100 + digits_min[1] * 10 + digits_min[2];
                    threshold_max = digits_max[0] * 1000 + digits_max[1] * 100 + digits_max[2] * 10 + digits_max[3];
                    // Garante que threshold_max não exceda 4094
                    if (threshold_max > ADC_MAX_VALUE) threshold_max = ADC_MAX_VALUE;
                    program_running = true; // Ativa o modo de execução
                    update_display();
                    set_all_leds(0, 10, 0); // LEDs verdes indicando configuração concluída
                    sleep_ms(2000);         // Pausa de 2 segundos para feedback
                }
                update_display();
            }

            // Ajuste do range pelo joystick nas etapas 1 e 2
            if (step == 1 || step == 2)
            {
                int *current_digits = (step == 1) ? digits_min : digits_max; // Seleciona o array de dígitos
                int max_pos = (step == 1) ? 2 : 3;                           // Define o número máximo de dígitos
                int max_digit_value = (step == 1) ? 9 : ((digit_pos == 0 && digits_max[0] <= 4) ? 4 : 9); // Limita o primeiro dígito de threshold_max a 4

                // Calcula o valor atual de threshold_max para verificar o limite
                int temp_max = digits_max[0] * 1000 + digits_max[1] * 100 + digits_max[2] * 10 + digits_max[3];

                // Eixo X do joystick: ajusta o valor do dígito atual
                if (joy_x < 1000 && current_digits[digit_pos] > 0) // Movimento à esquerda diminui o dígito
                {
                    current_digits[digit_pos]--;
                    update_display();
                    sleep_ms(200); // Debounce manual de 200 ms
                }
                else if (joy_x > 3000 && current_digits[digit_pos] < max_digit_value) // Movimento à direita aumenta o dígito com limite
                {
                    // Verifica se o incremento mantém threshold_max <= 4094
                    int new_digit = current_digits[digit_pos] + 1;
                    if (step == 2)
                    {
                        int potential_max = digits_max[0] * 1000 + digits_max[1] * 100 + digits_max[2] * 10 + digits_max[3] +
                                            (new_digit - current_digits[digit_pos]) * (int)pow(10, 3 - digit_pos);
                        if (potential_max <= ADC_MAX_VALUE)
                        {
                            current_digits[digit_pos] = new_digit;
                        }
                    }
                    else
                    {
                        current_digits[digit_pos] = new_digit;
                    }
                    update_display();
                    sleep_ms(200); // Debounce manual de 200 ms
                }

                // Eixo Y do joystick: navega entre os dígitos
                if (joy_y < 1000 && digit_pos > 0) // Movimento para cima seleciona o dígito anterior
                {
                    digit_pos--;
                    update_display();
                    sleep_ms(200); // Debounce manual de 200 ms
                }
                else if (joy_y > 3000 && digit_pos < max_pos) // Movimento para baixo seleciona o próximo dígito
                {
                    digit_pos++;
                    update_display();
                    sleep_ms(200); // Debounce manual de 200 ms
                }
            }
        }
        else if (program_running) // Modo de execução
        {
            if (!out_of_range) // Monitoramento ativo do sinal do microfone
            {
                adc_select_input(2); // Seleciona ADC2 (microfone)
                uint16_t mic_value = adc_read(); // Lê o valor analógico do microfone (0 a 4095)

                // Verifica se o sinal está fora do range definido
                if (mic_value < threshold_min || mic_value > threshold_max)
                {
                    out_of_range = true;    // Marca o estado de fora do range
                    set_all_leds(10, 0, 0); // LEDs vermelhos para alerta
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "Valor:%u", mic_value);
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_string(&ssd, "ATENCAO", 0, 0);
                    ssd1306_draw_string(&ssd, "FORA DO RANGE", 0, 10);
                    ssd1306_draw_string(&ssd, buffer, 0, 20);      // Exibe o valor fora do range
                    ssd1306_draw_string(&ssd, "A: Reiniciar", 0, 40);
                    ssd1306_send_data(&ssd);
                }
            }
            else // Estado de fora do range
            {
                send_sos_buzzer();   // Emite o sinal SOS continuamente
                sleep_ms(CYCLE_GAP); // Pausa entre ciclos SOS

                // Botão A reinicia a configuração
                if (button_a_pressed)
                {
                    button_a_pressed = false;
                    step = 0;              // Volta à tela inicial
                    digit_pos = 0;         // Reseta a posição do dígito
                    out_of_range = false;  // Sai do estado de fora do range
                    program_running = false; // Desativa o modo de execução
                    for (int i = 0; i < 3; i++) digits_min[i] = 0; // Reseta os dígitos mínimos
                    for (int i = 0; i < 4; i++) digits_max[i] = 0; // Reseta os dígitos máximos
                }
            }
        }

        sleep_us(interval_us); // Mantém a taxa de amostragem de 8 kHz
    }
}