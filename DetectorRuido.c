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
#define WS2812_PIN 7        // WS2812 LED matrix pin
#define NUM_PIXELS 25       // Number of pixels in 5x5 matrix
#define I2C_PORT i2c1       // I2C port for SSD1306
#define I2C_SDA 14          // SDA pin for SSD1306
#define I2C_SCL 15          // SCL pin for SSD1306
#define SSD1306_ADDR 0x3C   // SSD1306 I2C address

// Range settings
const uint RANGE_MIN = 500;  // Minimum acceptable value
const uint RANGE_MAX = 3500; // Maximum acceptable value

// Sampling and debounce configuration
const uint SAMPLES_PER_SECOND = 8000; // 8 kHz sampling rate
const uint DEBOUNCE_DELAY = 200;      // Debounce delay in milliseconds

// Global variables
volatile bool button_b_pressed = false; // Flag for Button B press
uint32_t last_button_b_time = 0;        // Last Button B press timestamp
bool out_of_range = false;              // Flag to track if noise went out of range
PIO pio = pio0;
int sm = 0;
uint32_t led_buffer[NUM_PIXELS] = {0};
ssd1306_t ssd; // SSD1306 display structure

// Function to enter BOOTSEL mode
void enter_bootsel()
{
    reset_usb_boot(0, 0);
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

// Interrupt handler for Button B
void button_isr_handler(uint gpio, uint32_t events)
{
    if (gpio == BTN_B_PIN && events & GPIO_IRQ_EDGE_FALL)
    {
        if (debounce_button(&last_button_b_time))
        {
            button_b_pressed = true;
        }
    }
}

// Setup function for Button B interrupt
void setup_button_b_interrupt()
{
    gpio_init(BTN_B_PIN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_pull_up(BTN_B_PIN);
    gpio_set_irq_enabled_with_callback(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr_handler);
}

// WS2812 Functions
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

int main()
{
    stdio_init_all();

    // Initialize ADC for microphone
    adc_init();
    adc_gpio_init(MICROPHONE);

    // Initialize WS2812
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    // Initialize SSD1306
    setup_ssd1306();

    setup_button_b_interrupt();

    // Display initial message
    ssd1306_fill(&ssd, false); // Clear display
    ssd1306_draw_string(&ssd, "Detector de", 0, 0);
    ssd1306_draw_string(&ssd, "Ruido", 0, 10);
    ssd1306_draw_string(&ssd, "Iniciado", 0, 20);
    ssd1306_send_data(&ssd);
    sleep_ms(2000); // Show message for 2 seconds

    // Clear display after showing message
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Calculate sampling interval
    uint64_t interval_us = 1000000 / SAMPLES_PER_SECOND;

    // Start with green LEDs (in range)
    set_all_leds(0, 10, 0); // Moderately bright green

    while (true)
    {
        if (button_b_pressed)
        {
            button_b_pressed = false;
            set_all_leds(0, 0, 0);     // Turn off all LEDs
            ssd1306_fill(&ssd, false); // Clear display
            ssd1306_send_data(&ssd);
            enter_bootsel();
        }

        // Only monitor if we haven't detected out of range yet
        if (!out_of_range)
        {
            adc_select_input(2);
            uint16_t mic_value = adc_read();

            if (mic_value < RANGE_MIN || mic_value > RANGE_MAX)
            {
                // Noise out of range detected
                out_of_range = true;
                set_all_leds(10, 0, 0); // Moderately bright red
            }
        }

        sleep_us(interval_us);
    }
}