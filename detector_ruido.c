#include "pico/stdlib.h"
#include "pico/cyw43_arch.h" // Para Wi-Fi
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "ssd1306.h"
#include "neopixel.h"
#include "lwip/apps/mqtt.h" // Biblioteca MQTT do Pico SDK

// Definições de pinos (ajuste conforme sua BitDogLab)
#define MIC_PIN 26        // Microfone no ADC0
#define LED_RGB_PIN 15    // LED RGB (PWM)
#define MATRIZ_PIN 12     // Matriz de LEDs RGB
#define NUM_LEDS 25       // Número de LEDs na matriz
#define BOTAO1_PIN 14     // Botão 1 (liga/desliga)
#define BOTAO2_PIN 13     // Botão 2 (reconecta Wi-Fi)
#define JOYSTICK_Y_PIN 27 // Eixo Y do joystick no ADC1
#define I2C_SDA_PIN 0     // SDA do SSD1306
#define I2C_SCL_PIN 1     // SCL do SSD1306

// Configurações Wi-Fi e MQTT
#define WIFI_SSID "SEU_SSID"
#define WIFI_PASS "SUA_SENHA"
#define MQTT_BROKER "SEU_BROKER" // ex.: "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_TOPIC "atividade/detectada"

// Variáveis globais
i2c_inst_t *i2c = i2c0;
ssd1306_t disp;
mqtt_client_t *mqtt_client;
bool monitoramento_ativo = false;
uint16_t limiar_som = 30000; // Valor inicial (ajustável)
absolute_time_t ultimo_evento;

void atualizar_display(const char *status, uint32_t tempo) {
    char buffer[16];
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 1, status);
    snprintf(buffer, sizeof(buffer), "Tempo: %lu s", tempo / 1000);
    ssd1306_draw_string(&disp, 0, 16, 1, buffer);
    ssd1306_show(&disp);
}

void matriz_amarelo(bool estado) {
    for (int i = 0; i < NUM_LEDS; i++) {
        neopixel_set_rgb(i, estado ? 255 : 0, estado ? 255 : 0, 0); // Amarelo ou apagado
    }
    neopixel_show(MATRIZ_PIN, NUM_LEDS);
}

void mqtt_conectar_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        pwm_set_gpio_level(LED_RGB_PIN, 65535); // Verde
    } else {
        pwm_set_gpio_level(LED_RGB_PIN, 0); // Vermelho
    }
}

void mqtt_publicar(const char *mensagem) {
    if (cyw43_arch_wifi_link_status() > 0) {
        mqtt_publish(mqtt_client, MQTT_TOPIC, mensagem, strlen(mensagem), 0, 0, NULL, NULL);
    }
}

void iniciar_wifi_mqtt() {
    cyw43_arch_init();
    cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000);
    struct mqtt_connect_client_info_t ci = {
        .client_id = "pico_w",
        .keepalive = 60,
    };
    mqtt_client = mqtt_client_new();
    mqtt_client_connect(mqtt_client, MQTT_BROKER, MQTT_PORT, mqtt_conectar_cb, NULL, &ci);
}

int main() {
    stdio_init_all();

    // Inicialização do ADC
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(0);

    // Configuração dos botões
    gpio_init(BOTAO1_PIN);
    gpio_set_dir(BOTAO1_PIN, GPIO_IN);
    gpio_pull_up(BOTAO1_PIN);
    gpio_init(BOTAO2_PIN);
    gpio_set_dir(BOTAO2_PIN, GPIO_IN);
    gpio_pull_up(BOTAO2_PIN);

    // Configuração do joystick
    adc_gpio_init(JOYSTICK_Y_PIN);

    // Configuração do LED RGB
    gpio_set_function(LED_RGB_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LED_RGB_PIN);
    pwm_set_wrap(slice_num, 65535);
    pwm_set_enabled(slice_num, true);

    // Configuração da matriz NeoPixel
    neopixel_init(MATRIZ_PIN, NUM_LEDS);

    // Configuração do I2C e SSD1306
    i2c_init(i2c, 400000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c);

    // Inicialização do Wi-Fi e MQTT
    iniciar_wifi_mqtt();

    while (1) {
        // Controle dos botões
        if (!gpio_get(BOTAO1_PIN)) { // Botão 1: liga/desliga monitoramento
            monitoramento_ativo = !monitoramento_ativo;
            sleep_ms(200); // Debounce
        }
        if (!gpio_get(BOTAO2_PIN)) { // Botão 2: reconecta Wi-Fi
            cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000);
            sleep_ms(200);
        }

        // Ajuste do limiar com o joystick
        adc_select_input(1);
        uint16_t jy = adc_read();
        limiar_som = 10000 + (jy * (50000 - 10000) / 4095); // Mapeia de 10000 a 50000

        // Monitoramento de som
        if (monitoramento_ativo) {
            adc_select_input(0);
            uint16_t som = adc_read();
            if (som > limiar_som) {
                ultimo_evento = get_absolute_time();
                atualizar_display("ATIVO", 0);
                matriz_amarelo(true);
                mqtt_publicar("Atividade detectada");
            } else {
                uint32_t tempo = absolute_time_diff_us(ultimo_evento, get_absolute_time()) / 1000000;
                atualizar_display("INATIVO", tempo);
                matriz_amarelo(false);
            }
        } else {
            atualizar_display("DESLIGADO", 0);
            matriz_amarelo(false);
        }

        sleep_ms(100);
    }
    return 0;
}