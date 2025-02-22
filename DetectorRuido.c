#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "pico/bootrom.h"

// Pin definitions
const uint LED_PIN_RED = 13;   // Red LED on GPIO13
const uint LED_PIN_GREEN = 11; // Green LED on GPIO11
const uint MICROPHONE = 28;    // Microphone on GPIO28 (ADC2)
const uint BTN_B_PIN = 6;      // Button B on GPIO6

// Range settings
const uint RANGE_MIN = 2000; // Minimum acceptable value
const uint RANGE_MAX = 2080; // Maximum acceptable value

// Sampling and debounce configuration
const uint SAMPLES_PER_SECOND = 8000; // 8 kHz sampling rate
const uint DEBOUNCE_DELAY = 200;      // Debounce delay in milliseconds

// Global variables
volatile bool button_b_pressed = false; // Flag for Button B press
uint32_t last_button_b_time = 0;        // Last Button B press timestamp

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

int main()
{
    gpio_init(LED_PIN_RED);
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);

    adc_init();
    adc_gpio_init(MICROPHONE);

    setup_button_b_interrupt();

    // Calculate sampling interval
    uint64_t interval_us = 1000000 / SAMPLES_PER_SECOND;

    while (true)
    {
        if (button_b_pressed)
        {
            button_b_pressed = false;
            gpio_put(LED_PIN_RED, false);
            gpio_put(LED_PIN_GREEN, false);
            enter_bootsel();
        }

        adc_select_input(2);
        uint16_t mic_value = adc_read();

        if (mic_value >= RANGE_MIN && mic_value <= RANGE_MAX)
        {
            gpio_put(LED_PIN_GREEN, true);
            gpio_put(LED_PIN_RED, false);
        }
        else
        {
            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_RED, true);
        }

        sleep_us(interval_us);
    }
}