#include <stdio.h>
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

// Pin definitions
const uint MICROPHONE = 28; // Microphone on GPIO28 (ADC2)
const uint BTN_B_PIN = 6;   // Button B on GPIO6
const uint BTN_A_PIN = 5;   // Button A on GPIO5
const uint JOYSTICK_X = 26; // Joystick X-axis on GPIO26 (ADC0)
const uint JOYSTICK_Y = 27; // Joystick Y-axis on GPIO27 (ADC1)
const uint BUZZER_PIN = 21; // Buzzer on GPIO21
#define WS2812_PIN 7        // WS2812 LED matrix pin
#define NUM_PIXELS 25       // Number of pixels in 5x5 matrix
#define I2C_PORT i2c1       // I2C port for SSD1306
#define I2C_SDA 14          // SDA pin for SSD1306
#define I2C_SCL 15          // SCL pin for SSD1306
#define SSD1306_ADDR 0x3C   // SSD1306 I2C address

// Sampling and debounce configuration
const uint SAMPLES_PER_SECOND = 8000; // 8 kHz sampling rate
const uint DEBOUNCE_DELAY = 200;      // Debounce delay in milliseconds

// Buzzer configuration
#define BUZZER_FREQ_HZ 2000
#define DOT_TIME 200
#define DASH_TIME 800
#define GAP_TIME 125
#define LETTER_GAP 250
#define CYCLE_GAP 3000 // Pausa entre ciclos SOS

// Global variables
volatile bool button_b_pressed = false;
volatile bool button_a_pressed = false;
uint32_t last_button_b_time = 0;
uint32_t last_button_a_time = 0;
bool out_of_range = false;
bool program_running = false;
int threshold_min = 0;
int threshold_max = 0;
int step = 0;
int digit_pos = 0;
int digits_min[3] = {0, 0, 0};
int digits_max[4] = {0, 0, 0, 0};
PIO pio = pio0;
int sm = 0;
uint32_t led_buffer[NUM_PIXELS] = {0};
ssd1306_t ssd;

// WS2812 Functions (movidas para o topo)
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void set_leds_from_buffer()
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        put_pixel(led_buffer[i]);
    }
}

void set_all_leds(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        led_buffer[i] = urgb_u32(r, g, b);
    }
    set_leds_from_buffer();
}

// Function to enter BOOTSEL mode
void enter_bootsel()
{
    set_all_leds(0, 0, 0);  // Garante que os LEDs sejam apagados
    set_leds_from_buffer(); // Força envio do buffer para os LEDs
    sleep_ms(50);           // Pequena pausa para garantir que os LEDs sejam apagados
    reset_usb_boot(0, 0);   // Entra no modo BOOTSEL
}

// Debounce function for button presses
bool debounce_button(uint32_t *last_time)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - *last_time >= DEBOUNCE_DELAY)
    {
        *last_time = current_time;
        return true;
    }
    return false;
}

// Interrupt handlers
void button_isr_handler(uint gpio, uint32_t events)
{
    if (gpio == BTN_B_PIN && events & GPIO_IRQ_EDGE_FALL)
    {
        if (debounce_button(&last_button_b_time))
        {
            button_b_pressed = true;
        }
    }
    if (gpio == BTN_A_PIN && events & GPIO_IRQ_EDGE_FALL)
    {
        if (debounce_button(&last_button_a_time))
        {
            button_a_pressed = true;
        }
    }
}

// Setup function for button interrupts
void setup_button_interrupts()
{
    gpio_init(BTN_B_PIN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_pull_up(BTN_B_PIN);

    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);

    gpio_set_irq_enabled_with_callback(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr_handler);
    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr_handler);
}

// SSD1306 initialization function
void setup_ssd1306()
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, SSD1306_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
}

// Custom function to draw inverted digit
void draw_inverted_digit(ssd1306_t *ssd, char digit, uint8_t x, uint8_t y)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
            ssd1306_pixel(ssd, x + i, y + j, true);
        }
    }

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

// Buzzer functions
void start_buzzer(uint32_t duration_ms)
{
    uint32_t period = 1000000 / BUZZER_FREQ_HZ;
    uint32_t half_period = period / 2;
    uint32_t end_time = time_us_32() + (duration_ms * 1000);

    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    while (time_us_32() < end_time)
    {
        // Verifica se algum botão foi pressionado para interromper o buzzer
        if (button_a_pressed || button_b_pressed)
        {
            gpio_put(BUZZER_PIN, 0); // Desliga o buzzer imediatamente
            return;                  // Sai da função
        }

        gpio_put(BUZZER_PIN, 1);
        sleep_us(half_period);
        gpio_put(BUZZER_PIN, 0);
        sleep_us(half_period);
    }

    gpio_put(BUZZER_PIN, 0);
}

void send_sos_buzzer()
{
    // 3 pontos (S)
    for (int i = 0; i < 3; i++)
    {
        start_buzzer(DOT_TIME);
        if (button_a_pressed || button_b_pressed)
            return; // Interrompe se botão for pressionado
        sleep_ms(GAP_TIME);
    }

    sleep_ms(LETTER_GAP);
    if (button_a_pressed || button_b_pressed)
        return;

    // 3 traços (O)
    for (int i = 0; i < 3; i++)
    {
        start_buzzer(DASH_TIME);
        if (button_a_pressed || button_b_pressed)
            return;
        sleep_ms(GAP_TIME);
    }

    sleep_ms(LETTER_GAP);
    if (button_a_pressed || button_b_pressed)
        return;

    // 3 pontos (S)
    for (int i = 0; i < 3; i++)
    {
        start_buzzer(DOT_TIME);
        if (button_a_pressed || button_b_pressed)
            return;
        sleep_ms(GAP_TIME);
    }
}

// Display update function
void update_display()
{
    char buffer[32];
    ssd1306_fill(&ssd, false);

    switch (step)
    {
    case 0: // Initial screen
        ssd1306_draw_string(&ssd, "Detector", 0, 0);
        ssd1306_draw_string(&ssd, "De Ruido", 0, 10);
        ssd1306_draw_string(&ssd, "Iniciado", 0, 20);
        ssd1306_draw_string(&ssd, "A: Prosseguir", 0, 40);
        break;
    case 1: // Min configuration
        snprintf(buffer, sizeof(buffer), "Min: %d%d%d", digits_min[0], digits_min[1], digits_min[2]);
        ssd1306_draw_string(&ssd, buffer, 0, 0);
        draw_inverted_digit(&ssd, digits_min[digit_pos] + '0', 40 + digit_pos * 8, 0);
        ssd1306_draw_string(&ssd, "X: mais:menos", 0, 10);
        ssd1306_draw_string(&ssd, "Y: digito", 0, 20);
        ssd1306_draw_string(&ssd, "A: Prosseguir", 0, 40);
        break;
    case 2: // Max configuration
        snprintf(buffer, sizeof(buffer), "Max: %d%d%d%d", digits_max[0], digits_max[1], digits_max[2], digits_max[3]);
        ssd1306_draw_string(&ssd, buffer, 0, 0);
        draw_inverted_digit(&ssd, digits_max[digit_pos] + '0', 40 + digit_pos * 8, 0);
        ssd1306_draw_string(&ssd, "X: mais:menos", 0, 10);
        ssd1306_draw_string(&ssd, "Y: digito", 0, 20);
        ssd1306_draw_string(&ssd, "A: Prosseguir", 0, 40);
        break;
    case 3: // Running
        snprintf(buffer, sizeof(buffer), "Min:%03d", threshold_min);
        ssd1306_draw_string(&ssd, buffer, 0, 0);
        snprintf(buffer, sizeof(buffer), "Max:%04d", threshold_max);
        ssd1306_draw_string(&ssd, buffer, 0, 8);
        ssd1306_draw_string(&ssd, "Funcionando...", 0, 40);
        break;
    }
    ssd1306_send_data(&ssd);
}

int main()
{
    stdio_init_all();

    // Initialize ADC
    adc_init();
    adc_gpio_init(MICROPHONE);
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);

    // Initialize WS2812
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    // Initialize SSD1306
    setup_ssd1306();

    // Initialize buzzer
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    setup_button_interrupts();

    // Initial display with blue LEDs for configuration
    update_display();
    set_all_leds(0, 0, 10); // Blue LEDs

    uint64_t interval_us = 1000000 / SAMPLES_PER_SECOND;

    while (true)
    {
        // Handle Button B (reset to BOOTSEL)
        if (button_b_pressed)
        {
            button_b_pressed = false;
            set_all_leds(0, 0, 0);  // Desliga os LEDs
            set_leds_from_buffer(); // Força a atualização dos LEDs
            sleep_ms(50);           // Pequena pausa para garantir que os LEDs sejam apagados
            ssd1306_fill(&ssd, false);
            ssd1306_send_data(&ssd);
            gpio_put(BUZZER_PIN, 0); // Desliga o buzzer antes de entrar no BOOTSEL
            enter_bootsel();
        }

        // Read joystick
        adc_select_input(0); // X-axis
        uint16_t joy_x = adc_read();
        adc_select_input(1); // Y-axis
        uint16_t joy_y = adc_read();

        if (step < 3) // Configuration steps
        {
            set_all_leds(0, 0, 10); // Blue LEDs during configuration
            update_display();

            // Handle Button A (proceed to next step)
            if (button_a_pressed)
            {
                button_a_pressed = false;
                step++;
                digit_pos = 0;
                if (step == 3)
                {
                    threshold_min = digits_min[0] * 100 + digits_min[1] * 10 + digits_min[2];
                    threshold_max = digits_max[0] * 1000 + digits_max[1] * 100 + digits_max[2] * 10 + digits_max[3];
                    program_running = true;
                    update_display();
                    set_all_leds(0, 10, 0); // Green LEDs when configuration is complete
                    sleep_ms(2000);
                }
                update_display();
            }

            // Handle joystick for digit adjustment and navigation
            if (step == 1 || step == 2)
            {
                int *current_digits = (step == 1) ? digits_min : digits_max;
                int max_pos = (step == 1) ? 2 : 3;

                // X-axis: Adjust digit value
                if (joy_x < 1000 && current_digits[digit_pos] > 0) // Left
                {
                    current_digits[digit_pos]--;
                    update_display();
                    sleep_ms(200);
                }
                else if (joy_x > 3000 && current_digits[digit_pos] < 9) // Right
                {
                    current_digits[digit_pos]++;
                    update_display();
                    sleep_ms(200);
                }

                // Y-axis: Navigate digits
                if (joy_y < 1000 && digit_pos > 0) // Up (left)
                {
                    digit_pos--;
                    update_display();
                    sleep_ms(200);
                }
                else if (joy_y > 3000 && digit_pos < max_pos) // Down (right)
                {
                    digit_pos++;
                    update_display();
                    sleep_ms(200);
                }
            }
        }
        else if (program_running) // Running mode
        {
            if (!out_of_range)
            {
                adc_select_input(2);
                uint16_t mic_value = adc_read();

                if (mic_value < threshold_min || mic_value > threshold_max)
                {
                    out_of_range = true;
                    set_all_leds(10, 0, 0); // Red LEDs
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "Valor:%u", mic_value);
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_string(&ssd, "ATENCAO", 0, 0);
                    ssd1306_draw_string(&ssd, "FORA DO RANGE", 0, 10);
                    ssd1306_draw_string(&ssd, buffer, 0, 20);
                    ssd1306_draw_string(&ssd, "A: Reiniciar", 0, 40);
                    ssd1306_send_data(&ssd);
                }
            }
            else
            {
                // Repetir o SOS enquanto estiver fora do range
                send_sos_buzzer();
                sleep_ms(CYCLE_GAP); // Pausa entre ciclos SOS

                // Verificar botões para sair do estado out_of_range
                if (button_a_pressed) // Restart configuration
                {
                    button_a_pressed = false;
                    step = 0;
                    digit_pos = 0;
                    out_of_range = false;
                    program_running = false;
                    for (int i = 0; i < 3; i++)
                        digits_min[i] = 0;
                    for (int i = 0; i < 4; i++)
                        digits_max[i] = 0;
                }
            }
        }

        sleep_us(interval_us);
    }
}