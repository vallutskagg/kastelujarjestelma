#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>

#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#define RELAY_GPIO GPIO_NUM_26
#define SOIL_CHANNEL ADC1_CHANNEL_6 /* GPIO34 */

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* Fallbacks for stale sdkconfig in editor/intellisense */
#ifndef CONFIG_WATERING_ADC_SAMPLES
#define CONFIG_WATERING_ADC_SAMPLES 8
#endif

#ifndef CONFIG_WATERING_TEMPERATURE_ENABLED
#define CONFIG_WATERING_TEMPERATURE_ENABLED 1
#endif

#ifndef CONFIG_WATERING_TEMP_GPIO
#define CONFIG_WATERING_TEMP_GPIO 4
#endif

#ifndef CONFIG_WATERING_TEMP_READ_INTERVAL_MS
#define CONFIG_WATERING_TEMP_READ_INTERVAL_MS 10000
#endif

#ifndef CONFIG_WATERING_DRY_CONSECUTIVE_READS
#define CONFIG_WATERING_DRY_CONSECUTIVE_READS 3
#endif

#ifndef CONFIG_WATERING_MQTT_CMD_ARM_DELAY_MS
#define CONFIG_WATERING_MQTT_CMD_ARM_DELAY_MS 5000
#endif

#ifndef CONFIG_WATERING_MIN_WATERING_INTERVAL_HOURS
#define CONFIG_WATERING_MIN_WATERING_INTERVAL_HOURS 72
#endif

#ifndef CONFIG_WATERING_RELAY_SELF_TEST
#define CONFIG_WATERING_RELAY_SELF_TEST 0
#endif

#ifndef CONFIG_WATERING_RELAY_SELF_TEST_INTERVAL_MS
#define CONFIG_WATERING_RELAY_SELF_TEST_INTERVAL_MS 1000
#endif

#ifndef CONFIG_WATERING_MQTT_BROKER_URI
#define CONFIG_WATERING_MQTT_BROKER_URI "mqtt://broker.hivemq.com"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC
#define CONFIG_WATERING_MQTT_TOPIC "watering_system/soil"
#endif

#ifndef CONFIG_WATERING_MQTT_CLIENT_ID
#define CONFIG_WATERING_MQTT_CLIENT_ID "watering-esp32"
#endif

#ifndef CONFIG_WATERING_MQTT_USERNAME
#define CONFIG_WATERING_MQTT_USERNAME ""
#endif

#ifndef CONFIG_WATERING_MQTT_PASSWORD
#define CONFIG_WATERING_MQTT_PASSWORD ""
#endif

#ifndef CONFIG_WATERING_MQTT_QOS
#define CONFIG_WATERING_MQTT_QOS 1
#endif

#ifndef CONFIG_WATERING_PUMP_DURATION_MIN_S
#define CONFIG_WATERING_PUMP_DURATION_MIN_S 8
#endif

#ifndef CONFIG_WATERING_PUMP_DURATION_MAX_S
#define CONFIG_WATERING_PUMP_DURATION_MAX_S 20
#endif

#ifndef CONFIG_WATERING_LEVEL_SENSOR_GPIO
#define CONFIG_WATERING_LEVEL_SENSOR_GPIO 27
#endif

#ifndef CONFIG_WATERING_LEVEL_SENSOR_ACTIVE_STATE
#define CONFIG_WATERING_LEVEL_SENSOR_ACTIVE_STATE 1
#endif

#ifndef CONFIG_WATERING_LEVEL_LED_GPIO
#define CONFIG_WATERING_LEVEL_LED_GPIO 25
#endif

#ifndef CONFIG_WATERING_LEVEL_LED_ACTIVE_STATE
#define CONFIG_WATERING_LEVEL_LED_ACTIVE_STATE 1
#endif

#ifndef CONFIG_WATERING_LEVEL_LED_ON_WHEN_LOW
#define CONFIG_WATERING_LEVEL_LED_ON_WHEN_LOW 1
#endif

#ifndef CONFIG_WATERING_MOISTURE_WET_ADC
#define CONFIG_WATERING_MOISTURE_WET_ADC 1500
#endif

#ifndef CONFIG_WATERING_MOISTURE_DRY_ADC
#define CONFIG_WATERING_MOISTURE_DRY_ADC 3200
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_MOISTURE_RAW
#define CONFIG_WATERING_MQTT_TOPIC_MOISTURE_RAW "watering_system/soil/raw"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_MOISTURE_PERCENT
#define CONFIG_WATERING_MQTT_TOPIC_MOISTURE_PERCENT "watering_system/soil/dry_percent"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_TEMPERATURE_C
#define CONFIG_WATERING_MQTT_TOPIC_TEMPERATURE_C "watering_system/temperature/outdoor_c"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_DURATION_S
#define CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_DURATION_S "watering_system/cmd/pump_duration_s"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_MANUAL
#define CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_MANUAL "watering_system/cmd/pump_manual"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_ENABLE
#define CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_ENABLE "watering_system/cmd/pump_enable"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_CMD_MEASURE_NOW
#define CONFIG_WATERING_MQTT_TOPIC_CMD_MEASURE_NOW "watering_system/cmd/measure_now"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_STATUS_PUMP_DURATION_S
#define CONFIG_WATERING_MQTT_TOPIC_STATUS_PUMP_DURATION_S "watering_system/status/pump_duration_s"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_STATUS_PUMP_ENABLE
#define CONFIG_WATERING_MQTT_TOPIC_STATUS_PUMP_ENABLE "watering_system/status/pump_enable"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_STATUS_PUMP_RUNNING
#define CONFIG_WATERING_MQTT_TOPIC_STATUS_PUMP_RUNNING "watering_system/status/pump_running"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_STATUS_TANK_LEVEL
#define CONFIG_WATERING_MQTT_TOPIC_STATUS_TANK_LEVEL "watering_system/status/tank_level"
#endif

#ifndef CONFIG_WATERING_MQTT_TOPIC_STATUS_TANK_LEVEL_RAW
#define CONFIG_WATERING_MQTT_TOPIC_STATUS_TANK_LEVEL_RAW "watering_system/status/tank_level_raw"
#endif

#ifndef GPIO_NUM_NC
#define GPIO_NUM_NC ((gpio_num_t)-1)
#endif

static const char *TAG = "watering";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num;
static esp_mqtt_client_handle_t s_mqtt_client;
static int s_mqtt_connected;
static time_t s_last_watering_ts;
static int s_pump_enabled = 1;
static int s_pump_running;
static int s_pump_duration_ms = CONFIG_WATERING_PUMP_ON_MS;
static int s_manual_watering_requests;
static int s_manual_measure_requests;
static float s_last_temperature_c;
static int s_last_temperature_valid;
static gpio_num_t s_temp_gpio = GPIO_NUM_NC;
static int s_temperature_gpio_ready;
static gpio_num_t s_level_sensor_gpio = GPIO_NUM_NC;
static int s_level_sensor_ready;
static int s_tank_level_has_water = -1;
static gpio_num_t s_level_led_gpio = GPIO_NUM_NC;
static int s_level_led_ready;
static int64_t s_mqtt_connected_since_us;
static portMUX_TYPE s_state_lock = portMUX_INITIALIZER_UNLOCKED;

static int mqtt_publish_message(const char *topic, const char *payload, int retain);
static void mqtt_publish_int_message(const char *topic, int value, int retain);

static void set_last_watering_ts(time_t value)
{
    taskENTER_CRITICAL(&s_state_lock);
    s_last_watering_ts = value;
    taskEXIT_CRITICAL(&s_state_lock);
}

static time_t get_last_watering_ts(void)
{
    time_t value;

    taskENTER_CRITICAL(&s_state_lock);
    value = s_last_watering_ts;
    taskEXIT_CRITICAL(&s_state_lock);

    return value;
}

static void set_last_temperature(float value, int valid)
{
    taskENTER_CRITICAL(&s_state_lock);
    s_last_temperature_c = value;
    s_last_temperature_valid = valid ? 1 : 0;
    taskEXIT_CRITICAL(&s_state_lock);
}

static int get_last_temperature(float *out_value)
{
    int valid;
    float value;

    taskENTER_CRITICAL(&s_state_lock);
    value = s_last_temperature_c;
    valid = s_last_temperature_valid;
    taskEXIT_CRITICAL(&s_state_lock);

    if (valid && out_value != NULL) {
        *out_value = value;
    }
    return valid;
}

static void set_mqtt_connected_since_us(int64_t timestamp_us)
{
    taskENTER_CRITICAL(&s_state_lock);
    s_mqtt_connected_since_us = timestamp_us;
    taskEXIT_CRITICAL(&s_state_lock);
}

static int64_t get_mqtt_connected_since_us(void)
{
    int64_t timestamp_us;

    taskENTER_CRITICAL(&s_state_lock);
    timestamp_us = s_mqtt_connected_since_us;
    taskEXIT_CRITICAL(&s_state_lock);

    return timestamp_us;
}

static void set_tank_level_has_water(int has_water)
{
    int value = has_water > 0 ? 1 : (has_water == 0 ? 0 : -1);

    taskENTER_CRITICAL(&s_state_lock);
    s_tank_level_has_water = value;
    taskEXIT_CRITICAL(&s_state_lock);
}

static int get_tank_level_has_water(void)
{
    int has_water;

    taskENTER_CRITICAL(&s_state_lock);
    has_water = s_tank_level_has_water;
    taskEXIT_CRITICAL(&s_state_lock);

    return has_water;
}

static int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static void set_pump_enabled(int enabled)
{
    taskENTER_CRITICAL(&s_state_lock);
    s_pump_enabled = enabled ? 1 : 0;
    taskEXIT_CRITICAL(&s_state_lock);
}

static int get_pump_enabled(void)
{
    int enabled;

    taskENTER_CRITICAL(&s_state_lock);
    enabled = s_pump_enabled;
    taskEXIT_CRITICAL(&s_state_lock);

    return enabled;
}

static void set_pump_running(int running)
{
    taskENTER_CRITICAL(&s_state_lock);
    s_pump_running = running ? 1 : 0;
    taskEXIT_CRITICAL(&s_state_lock);
}

static int get_pump_running(void)
{
    int running;

    taskENTER_CRITICAL(&s_state_lock);
    running = s_pump_running;
    taskEXIT_CRITICAL(&s_state_lock);

    return running;
}

static int get_pump_duration_ms(void)
{
    int duration_ms;

    taskENTER_CRITICAL(&s_state_lock);
    duration_ms = s_pump_duration_ms;
    taskEXIT_CRITICAL(&s_state_lock);

    return duration_ms;
}

static void set_pump_duration_ms(int duration_ms)
{
    int min_ms = CONFIG_WATERING_PUMP_DURATION_MIN_S * 1000;
    int max_ms = CONFIG_WATERING_PUMP_DURATION_MAX_S * 1000;
    if (max_ms < min_ms) {
        max_ms = min_ms;
    }

    taskENTER_CRITICAL(&s_state_lock);
    s_pump_duration_ms = clamp_int(duration_ms, min_ms, max_ms);
    taskEXIT_CRITICAL(&s_state_lock);
}

static void queue_manual_watering_request(void)
{
    taskENTER_CRITICAL(&s_state_lock);
    if (s_manual_watering_requests < 10) {
        s_manual_watering_requests++;
    }
    taskEXIT_CRITICAL(&s_state_lock);
}

static int consume_manual_watering_request(void)
{
    int has_request = 0;

    taskENTER_CRITICAL(&s_state_lock);
    if (s_manual_watering_requests > 0) {
        s_manual_watering_requests--;
        has_request = 1;
    }
    taskEXIT_CRITICAL(&s_state_lock);

    return has_request;
}

static void queue_manual_measure_request(void)
{
    taskENTER_CRITICAL(&s_state_lock);
    if (s_manual_measure_requests < 10) {
        s_manual_measure_requests++;
    }
    taskEXIT_CRITICAL(&s_state_lock);
}

static int consume_manual_measure_request(void)
{
    int has_request = 0;

    taskENTER_CRITICAL(&s_state_lock);
    if (s_manual_measure_requests > 0) {
        s_manual_measure_requests--;
        has_request = 1;
    }
    taskEXIT_CRITICAL(&s_state_lock);

    return has_request;
}

static void relay_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    /* Active-low relay modules are common: HIGH = OFF, LOW = ON */
    ESP_ERROR_CHECK(gpio_set_level(RELAY_GPIO, 1));
}

static void soil_adc_init(void)
{
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(SOIL_CHANNEL, ADC_ATTEN_DB_11));
}

static int soil_read(void)
{
    return adc1_get_raw(SOIL_CHANNEL);
}

static int soil_read_average(void)
{
    int sum = 0;

    for (int i = 0; i < CONFIG_WATERING_ADC_SAMPLES; i++) {
        sum += soil_read();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    return sum / CONFIG_WATERING_ADC_SAMPLES;
}

static const char *tank_level_to_text(int has_water)
{
    if (has_water > 0) {
        return "FULL";
    }
    if (has_water == 0) {
        return "LOW";
    }
    return "UNKNOWN";
}

static int tank_level_sensor_read_has_water(void)
{
    int raw_level;
    int active_level = CONFIG_WATERING_LEVEL_SENSOR_ACTIVE_STATE ? 1 : 0;

    if (!s_level_sensor_ready) {
        return -1;
    }

    raw_level = gpio_get_level(s_level_sensor_gpio) ? 1 : 0;
    return raw_level == active_level ? 1 : 0;
}

static void mqtt_publish_tank_level_state(int has_water)
{
    mqtt_publish_message(CONFIG_WATERING_MQTT_TOPIC_STATUS_TANK_LEVEL,
                         tank_level_to_text(has_water),
                         1);
    mqtt_publish_int_message(CONFIG_WATERING_MQTT_TOPIC_STATUS_TANK_LEVEL_RAW,
                             has_water,
                             1);
}

static void level_led_apply_from_tank_level(int has_water)
{
    int should_on;
    int gpio_level;
    esp_err_t err;

    if (!s_level_led_ready || has_water < 0) {
        return;
    }

    should_on = CONFIG_WATERING_LEVEL_LED_ON_WHEN_LOW ? (has_water == 0) : (has_water > 0);
    gpio_level = should_on ? (CONFIG_WATERING_LEVEL_LED_ACTIVE_STATE ? 1 : 0)
                           : (CONFIG_WATERING_LEVEL_LED_ACTIVE_STATE ? 0 : 1);

    err = gpio_set_level(s_level_led_gpio, gpio_level);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set level LED GPIO%d (err=0x%x)", s_level_led_gpio, err);
    }
}

static int tank_level_refresh_state(int force_publish)
{
    int has_water = tank_level_sensor_read_has_water();
    int previous = get_tank_level_has_water();

    if (has_water < 0) {
        if (force_publish || previous != -1) {
            set_tank_level_has_water(-1);
            mqtt_publish_tank_level_state(-1);
        }
        return -1;
    }

    set_tank_level_has_water(has_water);
    level_led_apply_from_tank_level(has_water);

    if (force_publish || has_water != previous) {
        ESP_LOGI(TAG, "Tank level state: %s", tank_level_to_text(has_water));
        mqtt_publish_tank_level_state(has_water);
    }

    return has_water;
}

static void tank_level_sensor_init(void)
{
    gpio_config_t io_conf = {0};
    int initial_has_water;

    if (!GPIO_IS_VALID_GPIO(CONFIG_WATERING_LEVEL_SENSOR_GPIO)) {
        ESP_LOGW(TAG, "Invalid tank level sensor GPIO: %d", CONFIG_WATERING_LEVEL_SENSOR_GPIO);
        return;
    }

    s_level_sensor_gpio = (gpio_num_t)CONFIG_WATERING_LEVEL_SENSOR_GPIO;
    io_conf.pin_bit_mask = (1ULL << s_level_sensor_gpio);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    s_level_sensor_ready = 1;
    initial_has_water = tank_level_sensor_read_has_water();
    set_tank_level_has_water(initial_has_water);

    ESP_LOGI(TAG,
             "Tank level sensor on GPIO%d (active_state=%d), initial=%s",
             CONFIG_WATERING_LEVEL_SENSOR_GPIO,
             CONFIG_WATERING_LEVEL_SENSOR_ACTIVE_STATE ? 1 : 0,
             tank_level_to_text(initial_has_water));
}

static void level_led_init(void)
{
    gpio_config_t io_conf = {0};
    int initial_level;
    esp_err_t err;

    if (!GPIO_IS_VALID_OUTPUT_GPIO(CONFIG_WATERING_LEVEL_LED_GPIO)) {
        ESP_LOGW(TAG, "Invalid level LED GPIO: %d", CONFIG_WATERING_LEVEL_LED_GPIO);
        return;
    }

    s_level_led_gpio = (gpio_num_t)CONFIG_WATERING_LEVEL_LED_GPIO;
    io_conf.pin_bit_mask = (1ULL << s_level_led_gpio);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    s_level_led_ready = 1;
    initial_level = CONFIG_WATERING_LEVEL_LED_ACTIVE_STATE ? 0 : 1;
    err = gpio_set_level(s_level_led_gpio, initial_level);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize level LED GPIO%d (err=0x%x)", s_level_led_gpio, err);
        return;
    }
    ESP_LOGI(TAG,
             "Level LED ready on GPIO%d (active_state=%d, on_when_low=%d)",
             CONFIG_WATERING_LEVEL_LED_GPIO,
             CONFIG_WATERING_LEVEL_LED_ACTIVE_STATE ? 1 : 0,
             CONFIG_WATERING_LEVEL_LED_ON_WHEN_LOW ? 1 : 0);
}

static void onewire_drive_low(void)
{
    ESP_ERROR_CHECK(gpio_set_level(s_temp_gpio, 0));
}

static void onewire_release_line(void)
{
    ESP_ERROR_CHECK(gpio_set_level(s_temp_gpio, 1));
}

static int ds18b20_reset_and_presence(void)
{
    int presence = 0;

    onewire_drive_low();
    esp_rom_delay_us(480);
    onewire_release_line();
    esp_rom_delay_us(70);
    presence = (gpio_get_level(s_temp_gpio) == 0);
    esp_rom_delay_us(410);

    return presence;
}

static void ds18b20_write_bit(int bit_value)
{
    onewire_drive_low();
    if (bit_value) {
        esp_rom_delay_us(6);
        onewire_release_line();
        esp_rom_delay_us(64);
    } else {
        esp_rom_delay_us(60);
        onewire_release_line();
        esp_rom_delay_us(10);
    }
}

static int ds18b20_read_bit(void)
{
    int bit_value;

    onewire_drive_low();
    esp_rom_delay_us(3);
    onewire_release_line();
    esp_rom_delay_us(10);
    bit_value = gpio_get_level(s_temp_gpio);
    esp_rom_delay_us(53);

    return bit_value;
}

static void ds18b20_write_byte(uint8_t value)
{
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit((value >> i) & 0x1);
    }
}

static uint8_t ds18b20_read_byte(void)
{
    uint8_t value = 0;

    for (int i = 0; i < 8; i++) {
        if (ds18b20_read_bit()) {
            value |= (uint8_t)(1U << i);
        }
    }

    return value;
}

static uint8_t ds18b20_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;

    for (size_t i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (int bit = 0; bit < 8; bit++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }

    return crc;
}

static void temperature_sensor_init(void)
{
#if CONFIG_WATERING_TEMPERATURE_ENABLED
    gpio_config_t io_conf = {0};

    if (!GPIO_IS_VALID_OUTPUT_GPIO(CONFIG_WATERING_TEMP_GPIO)) {
        ESP_LOGW(TAG, "Invalid DS18B20 GPIO: %d", CONFIG_WATERING_TEMP_GPIO);
        return;
    }

    s_temp_gpio = (gpio_num_t)CONFIG_WATERING_TEMP_GPIO;
    io_conf.pin_bit_mask = (1ULL << s_temp_gpio);
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    onewire_release_line();
    s_temperature_gpio_ready = 1;

    if (ds18b20_reset_and_presence()) {
        ESP_LOGI(TAG, "DS18B20 detected on GPIO%d", CONFIG_WATERING_TEMP_GPIO);
    } else {
        ESP_LOGW(TAG, "No DS18B20 presence pulse on GPIO%d", CONFIG_WATERING_TEMP_GPIO);
    }
    ESP_LOGI(TAG, "Use external 4.7k pull-up between DS18B20 DATA and 3V3");
#else
    ESP_LOGI(TAG, "DS18B20 support disabled");
#endif
}

static esp_err_t ds18b20_read_celsius(float *out_celsius)
{
    uint8_t scratchpad[9];

    if (out_celsius == NULL || !s_temperature_gpio_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!ds18b20_reset_and_presence()) {
        return ESP_ERR_NOT_FOUND;
    }
    ds18b20_write_byte(0xCC); /* Skip ROM, single device on bus */
    ds18b20_write_byte(0x44); /* Convert T */

    vTaskDelay(pdMS_TO_TICKS(750));

    if (!ds18b20_reset_and_presence()) {
        return ESP_ERR_TIMEOUT;
    }
    ds18b20_write_byte(0xCC); /* Skip ROM */
    ds18b20_write_byte(0xBE); /* Read scratchpad */

    for (int i = 0; i < 9; i++) {
        scratchpad[i] = ds18b20_read_byte();
    }

    if (ds18b20_crc8(scratchpad, 8) != scratchpad[8]) {
        return ESP_FAIL;
    }

    int16_t raw_temp = (int16_t)((scratchpad[1] << 8) | scratchpad[0]);
    *out_celsius = (float)raw_temp / 16.0f;
    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_wifi_connect());
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONFIG_WATERING_WIFI_MAXIMUM_RETRY) {
            ESP_ERROR_CHECK(esp_wifi_connect());
            s_retry_num++;
            ESP_LOGI(TAG, "WiFi reconnecting... (%d/%d)", s_retry_num, CONFIG_WATERING_WIFI_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    s_wifi_event_group = xEventGroupCreate();

    wifi_config_t wifi_config = {0};
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", CONFIG_WATERING_WIFI_SSID);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", CONFIG_WATERING_WIFI_PASSWORD);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", CONFIG_WATERING_WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(30000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connection established");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG, "WiFi failed after %d retries", CONFIG_WATERING_WIFI_MAXIMUM_RETRY);
    } else {
        ESP_LOGW(TAG, "WiFi connect timeout (30 s)");
    }
}

static void time_init(void)
{
    setenv("TZ", CONFIG_WATERING_TIMEZONE, 1);
    tzset();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    ESP_LOGI(TAG, "Waiting for NTP sync...");
    for (int i = 0; i < 10; i++) {
        time_t now = 0;
        struct tm timeinfo = {0};
        time(&now);
        localtime_r(&now, &timeinfo);
        if (timeinfo.tm_year >= (2024 - 1900)) {
            ESP_LOGI(TAG, "Time synced: %s", asctime(&timeinfo));
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGW(TAG, "NTP sync not confirmed yet, continuing");
}

static int str_not_empty(const char *value)
{
    return value != NULL && value[0] != '\0';
}

static int uri_uses_tls(const char *uri)
{
    if (uri == NULL) {
        return 0;
    }

    return strncmp(uri, "mqtts://", 8) == 0 || strncmp(uri, "wss://", 6) == 0;
}

static void format_local_time_or_never(time_t ts, char *out, size_t out_len)
{
    struct tm timeinfo = {0};

    if (ts <= 0) {
        snprintf(out, out_len, "never");
        return;
    }

    if (localtime_r(&ts, &timeinfo) == NULL) {
        snprintf(out, out_len, "unknown");
        return;
    }

    if (strftime(out, out_len, "%Y-%m-%d %H:%M:%S", &timeinfo) == 0) {
        snprintf(out, out_len, "unknown");
    }
}

static int soil_to_dry_percent(int moisture)
{
    int wet = CONFIG_WATERING_MOISTURE_WET_ADC;
    int dry = CONFIG_WATERING_MOISTURE_DRY_ADC;

    if (dry <= wet) {
        return moisture >= CONFIG_WATERING_DRY_THRESHOLD ? 100 : 0;
    }

    int percent = ((moisture - wet) * 100) / (dry - wet);
    return clamp_int(percent, 0, 100);
}

static int mqtt_publish_message(const char *topic, const char *payload, int retain)
{
    if (s_mqtt_client == NULL || !s_mqtt_connected) {
        return -1;
    }

    return esp_mqtt_client_publish(s_mqtt_client,
                                   topic,
                                   payload,
                                   0,
                                   CONFIG_WATERING_MQTT_QOS,
                                   retain);
}

static void mqtt_publish_int_message(const char *topic, int value, int retain)
{
    char payload[16];

    snprintf(payload, sizeof(payload), "%d", value);
    mqtt_publish_message(topic, payload, retain);
}

static void mqtt_publish_float_message(const char *topic, float value, int retain)
{
    char payload[24];

    snprintf(payload, sizeof(payload), "%.2f", (double)value);
    mqtt_publish_message(topic, payload, retain);
}

static void mqtt_publish_pump_duration_state(void)
{
    mqtt_publish_int_message(CONFIG_WATERING_MQTT_TOPIC_STATUS_PUMP_DURATION_S,
                             get_pump_duration_ms() / 1000,
                             1);
}

static void mqtt_publish_pump_enabled_state(void)
{
    mqtt_publish_message(CONFIG_WATERING_MQTT_TOPIC_STATUS_PUMP_ENABLE,
                         get_pump_enabled() ? "ON" : "OFF",
                         1);
}

static void mqtt_publish_pump_running_state(void)
{
    mqtt_publish_message(CONFIG_WATERING_MQTT_TOPIC_STATUS_PUMP_RUNNING,
                         get_pump_running() ? "ON" : "OFF",
                         1);
}

static int mqtt_topic_equals(esp_mqtt_event_handle_t event, const char *topic)
{
    size_t topic_len = strlen(topic);
    return event != NULL &&
           event->topic != NULL &&
           event->topic_len == (int)topic_len &&
           strncmp(event->topic, topic, topic_len) == 0;
}

static int payload_equals_ci(const char *data, int data_len, const char *value)
{
    size_t value_len = strlen(value);

    if (data == NULL || data_len != (int)value_len) {
        return 0;
    }

    for (int i = 0; i < data_len; i++) {
        if (tolower((unsigned char)data[i]) != tolower((unsigned char)value[i])) {
            return 0;
        }
    }

    return 1;
}

static int mqtt_parse_int_payload(const char *data, int data_len, int *out_value)
{
    char buf[24];
    char *endptr = NULL;
    long parsed;

    if (data == NULL || out_value == NULL || data_len <= 0 || data_len >= (int)sizeof(buf)) {
        return 0;
    }

    memcpy(buf, data, data_len);
    buf[data_len] = '\0';

    parsed = strtol(buf, &endptr, 10);
    if (endptr == buf) {
        return 0;
    }
    while (*endptr != '\0' && isspace((unsigned char)*endptr)) {
        endptr++;
    }
    if (*endptr != '\0') {
        return 0;
    }

    *out_value = (int)parsed;
    return 1;
}

static int mqtt_parse_switch_payload(const char *data, int data_len, int *enabled)
{
    if (payload_equals_ci(data, data_len, "ON") ||
        payload_equals_ci(data, data_len, "1") ||
        payload_equals_ci(data, data_len, "TRUE")) {
        *enabled = 1;
        return 1;
    }
    if (payload_equals_ci(data, data_len, "OFF") ||
        payload_equals_ci(data, data_len, "0") ||
        payload_equals_ci(data, data_len, "FALSE")) {
        *enabled = 0;
        return 1;
    }
    return 0;
}

static int mqtt_payload_is_manual_trigger(const char *data, int data_len)
{
    return payload_equals_ci(data, data_len, "RUN") ||
           payload_equals_ci(data, data_len, "ON") ||
           payload_equals_ci(data, data_len, "1") ||
           payload_equals_ci(data, data_len, "START");
}

static int mqtt_payload_is_measure_trigger(const char *data, int data_len)
{
    return payload_equals_ci(data, data_len, "READ") ||
           payload_equals_ci(data, data_len, "MEASURE") ||
           payload_equals_ci(data, data_len, "ON") ||
           payload_equals_ci(data, data_len, "1");
}

static void pump_set_output(int on)
{
    ESP_ERROR_CHECK(gpio_set_level(RELAY_GPIO, on ? 0 : 1));
}

static int pump_run_for_ms(int duration_ms, const char *reason)
{
    int elapsed_ms = 0;
    int aborted = 0;
    int tank_has_water = tank_level_refresh_state(0);

    if (!get_pump_enabled()) {
        ESP_LOGI(TAG, "Pump command ignored (%s), pump is disabled", reason);
        return 0;
    }
    if (tank_has_water == 0) {
        ESP_LOGW(TAG, "Pump command ignored (%s), tank level is LOW", reason);
        return 0;
    }

    set_pump_running(1);
    mqtt_publish_pump_running_state();
    pump_set_output(1);
    ESP_LOGI(TAG, "Pump ON (%s), duration %d ms", reason, duration_ms);

    while (elapsed_ms < duration_ms) {
        int step_ms = duration_ms - elapsed_ms;
        if (step_ms > 100) {
            step_ms = 100;
        }

        if (!get_pump_enabled()) {
            aborted = 1;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(step_ms));
        elapsed_ms += step_ms;
    }

    pump_set_output(0);
    set_pump_running(0);
    mqtt_publish_pump_running_state();

    if (aborted) {
        ESP_LOGW(TAG, "Pump run aborted because pump was disabled");
        return 0;
    }

    return 1;
}

static void mqtt_subscribe_command_topics(void)
{
    int msg_id;

    msg_id = esp_mqtt_client_subscribe(
        s_mqtt_client, CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_DURATION_S, CONFIG_WATERING_MQTT_QOS);
    ESP_LOGI(TAG, "MQTT subscribe duration topic, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(
        s_mqtt_client, CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_MANUAL, CONFIG_WATERING_MQTT_QOS);
    ESP_LOGI(TAG, "MQTT subscribe manual topic, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(
        s_mqtt_client, CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_ENABLE, CONFIG_WATERING_MQTT_QOS);
    ESP_LOGI(TAG, "MQTT subscribe enable topic, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(
        s_mqtt_client, CONFIG_WATERING_MQTT_TOPIC_CMD_MEASURE_NOW, CONFIG_WATERING_MQTT_QOS);
    ESP_LOGI(TAG, "MQTT subscribe measure topic, msg_id=%d", msg_id);
}

static void mqtt_handle_data_event(esp_mqtt_event_handle_t event)
{
    int is_command_topic = mqtt_topic_equals(event, CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_DURATION_S) ||
                           mqtt_topic_equals(event, CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_ENABLE) ||
                           mqtt_topic_equals(event, CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_MANUAL) ||
                           mqtt_topic_equals(event, CONFIG_WATERING_MQTT_TOPIC_CMD_MEASURE_NOW);

    if (is_command_topic && CONFIG_WATERING_MQTT_CMD_ARM_DELAY_MS > 0) {
        int64_t connected_since_us = get_mqtt_connected_since_us();
        int64_t elapsed_us = esp_timer_get_time() - connected_since_us;
        int64_t required_us = (int64_t)CONFIG_WATERING_MQTT_CMD_ARM_DELAY_MS * 1000;

        if (connected_since_us > 0 && elapsed_us >= 0 && elapsed_us < required_us) {
            ESP_LOGW(TAG, "Ignoring MQTT command during reconnect arm-delay (%d ms)",
                     CONFIG_WATERING_MQTT_CMD_ARM_DELAY_MS);
            return;
        }
    }

    if (mqtt_topic_equals(event, CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_DURATION_S)) {
        int seconds = 0;
        int min_s = CONFIG_WATERING_PUMP_DURATION_MIN_S;
        int max_s = CONFIG_WATERING_PUMP_DURATION_MAX_S;

        if (max_s < min_s) {
            max_s = min_s;
        }

        if (!mqtt_parse_int_payload(event->data, event->data_len, &seconds)) {
            ESP_LOGW(TAG, "Invalid slider payload for duration topic");
            return;
        }

        seconds = clamp_int(seconds, min_s, max_s);
        set_pump_duration_ms(seconds * 1000);
        mqtt_publish_pump_duration_state();
        ESP_LOGI(TAG, "Pump duration set via MQTT slider: %d s", seconds);
        return;
    }

    if (mqtt_topic_equals(event, CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_ENABLE)) {
        int enabled = 0;

        if (!mqtt_parse_switch_payload(event->data, event->data_len, &enabled)) {
            ESP_LOGW(TAG, "Invalid switch payload, expected ON/OFF/1/0");
            return;
        }

        set_pump_enabled(enabled);
        mqtt_publish_pump_enabled_state();
        ESP_LOGI(TAG, "Pump enable state set via MQTT switch: %s", enabled ? "ON" : "OFF");
        return;
    }

    if (mqtt_topic_equals(event, CONFIG_WATERING_MQTT_TOPIC_CMD_PUMP_MANUAL)) {
        if (!mqtt_payload_is_manual_trigger(event->data, event->data_len)) {
            ESP_LOGW(TAG, "Invalid manual payload, expected RUN/ON/1/START");
            return;
        }

        queue_manual_watering_request();
        ESP_LOGI(TAG, "Manual watering request queued via MQTT button");
        return;
    }

    if (mqtt_topic_equals(event, CONFIG_WATERING_MQTT_TOPIC_CMD_MEASURE_NOW)) {
        if (!mqtt_payload_is_measure_trigger(event->data, event->data_len)) {
            ESP_LOGW(TAG, "Invalid measure payload, expected READ/MEASURE/ON/1");
            return;
        }

        queue_manual_measure_request();
        ESP_LOGI(TAG, "Manual moisture measurement request queued via MQTT button");
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_mqtt_connected = 1;
        set_mqtt_connected_since_us(esp_timer_get_time());
        ESP_LOGI(TAG, "MQTT connected");
        mqtt_subscribe_command_topics();
        mqtt_publish_pump_duration_state();
        mqtt_publish_pump_enabled_state();
        mqtt_publish_pump_running_state();
        tank_level_refresh_state(1);
        queue_manual_measure_request();
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_mqtt_connected = 0;
        set_mqtt_connected_since_us(0);
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_DATA:
        mqtt_handle_data_event(event);
        break;
    case MQTT_EVENT_ERROR:
        s_mqtt_connected = 0;
        set_mqtt_connected_since_us(0);
        if (event != NULL && event->error_handle != NULL) {
            ESP_LOGW(TAG,
                     "MQTT error type=%d tls_last=0x%x transport_sock=%d conn_refused=0x%x",
                     event->error_handle->error_type,
                     event->error_handle->esp_tls_last_esp_err,
                     event->error_handle->esp_transport_sock_errno,
                     event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "MQTT error");
        }
        break;
    default:
        break;
    }
}

static void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_WATERING_MQTT_BROKER_URI,
        .credentials.client_id = CONFIG_WATERING_MQTT_CLIENT_ID,
    };

    if (str_not_empty(CONFIG_WATERING_MQTT_USERNAME)) {
        mqtt_cfg.credentials.username = CONFIG_WATERING_MQTT_USERNAME;
    }
    if (str_not_empty(CONFIG_WATERING_MQTT_PASSWORD)) {
        mqtt_cfg.credentials.authentication.password = CONFIG_WATERING_MQTT_PASSWORD;
    }

    if (uri_uses_tls(CONFIG_WATERING_MQTT_BROKER_URI)) {
        /* Required for mqtts:// and wss:// so server cert can be verified */
        mqtt_cfg.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
    }

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGW(TAG, "MQTT init failed");
        return;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));

    ESP_LOGI(TAG, "MQTT starting, broker: %s, topic: %s",
             CONFIG_WATERING_MQTT_BROKER_URI, CONFIG_WATERING_MQTT_TOPIC);
}

static void mqtt_publish_soil_value(int moisture)
{
    if (s_mqtt_client == NULL || !s_mqtt_connected) {
        return;
    }

    time_t now = 0;
    time_t last_watering = get_last_watering_ts();
    float temperature_c = 0.0f;
    int has_temperature = get_last_temperature(&temperature_c);
    char last_watering_local[32];
    char payload[256];
    int dry_percent = soil_to_dry_percent(moisture);

    time(&now);
    format_local_time_or_never(last_watering, last_watering_local, sizeof(last_watering_local));
    if (has_temperature) {
        snprintf(payload,
                 sizeof(payload),
                 "{\"moisture\":%d,\"dry_percent\":%d,\"temperature_c\":%.2f,\"dry_threshold\":%d,\"ts\":%ld,"
                 "\"last_watering_ts\":%ld,\"last_watering_local\":\"%s\"}",
                 moisture,
                 dry_percent,
                 (double)temperature_c,
                 CONFIG_WATERING_DRY_THRESHOLD,
                 (long)now,
                 (long)last_watering,
                 last_watering_local);
    } else {
        snprintf(payload,
                 sizeof(payload),
                 "{\"moisture\":%d,\"dry_percent\":%d,\"temperature_c\":null,\"dry_threshold\":%d,\"ts\":%ld,"
                 "\"last_watering_ts\":%ld,\"last_watering_local\":\"%s\"}",
                 moisture,
                 dry_percent,
                 CONFIG_WATERING_DRY_THRESHOLD,
                 (long)now,
                 (long)last_watering,
                 last_watering_local);
    }

    if (mqtt_publish_message(CONFIG_WATERING_MQTT_TOPIC, payload, 0) < 0) {
        ESP_LOGW(TAG, "MQTT publish failed");
    } else {
        ESP_LOGI(TAG, "MQTT publish telemetry payload=%s", payload);
    }

    mqtt_publish_int_message(CONFIG_WATERING_MQTT_TOPIC_MOISTURE_RAW, moisture, 0);
    mqtt_publish_int_message(CONFIG_WATERING_MQTT_TOPIC_MOISTURE_PERCENT, dry_percent, 0);
}

static void temperature_task(void *pvParameter)
{
    (void)pvParameter;
#if CONFIG_WATERING_TEMPERATURE_ENABLED
    int failure_count = 0;
    const int interval_ms = clamp_int(CONFIG_WATERING_TEMP_READ_INTERVAL_MS, 1000, 3600000);

    if (!s_temperature_gpio_ready) {
        ESP_LOGW(TAG, "Temperature task not started: DS18B20 GPIO not ready");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        float temperature_c = 0.0f;
        esp_err_t err = ds18b20_read_celsius(&temperature_c);

        if (err == ESP_OK) {
            if (failure_count > 0) {
                ESP_LOGI(TAG, "DS18B20 communication recovered");
            }
            failure_count = 0;
            set_last_temperature(temperature_c, 1);
            mqtt_publish_float_message(CONFIG_WATERING_MQTT_TOPIC_TEMPERATURE_C, temperature_c, 0);
            ESP_LOGI(TAG, "Outdoor temperature: %.2f C", (double)temperature_c);
        } else {
            failure_count++;
            if (failure_count == 1 || (failure_count % 12) == 0) {
                ESP_LOGW(TAG, "DS18B20 read failed (err=0x%x, count=%d)", err, failure_count);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
#else
    vTaskDelete(NULL);
#endif
}

static void watering_task(void *pvParameter)
{
    (void)pvParameter;
    time_t last_watering = 0;
    int dry_streak = 0;
    const time_t min_interval_s = (time_t)CONFIG_WATERING_MIN_WATERING_INTERVAL_HOURS * 3600;
    const int64_t check_interval_us = (int64_t)CONFIG_WATERING_CHECK_INTERVAL_MS * 1000;
    int64_t next_check_us = 0;

    while (1) {
        tank_level_refresh_state(0);

        if (consume_manual_watering_request()) {
            int manual_duration_ms = get_pump_duration_ms();
            if (pump_run_for_ms(manual_duration_ms, "manual MQTT button")) {
                time(&last_watering);
                set_last_watering_ts(last_watering);
                dry_streak = 0;
            }
        }

        if (consume_manual_measure_request()) {
            int moisture = soil_read_average();
            ESP_LOGI(TAG, "Manual moisture read: %d", moisture);
            mqtt_publish_soil_value(moisture);
        }

        int64_t now_us = esp_timer_get_time();
        if (next_check_us == 0 || now_us >= next_check_us) {
            int moisture = soil_read_average();
            time_t now;

            time(&now);
            ESP_LOGI(TAG, "Soil ADC avg: %d", moisture);
            mqtt_publish_soil_value(moisture);

            if (!get_pump_enabled()) {
                dry_streak = 0;
                next_check_us = now_us + check_interval_us;
                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }

            if (moisture > CONFIG_WATERING_DRY_THRESHOLD) {
                dry_streak++;
            } else {
                dry_streak = 0;
            }

            int dry_confirmed = dry_streak >= CONFIG_WATERING_DRY_CONSECUTIVE_READS;
            int cooldown_ok = (last_watering == 0) || ((now - last_watering) >= min_interval_s);

            if (dry_confirmed && cooldown_ok) {
                int auto_duration_ms = get_pump_duration_ms();

                ESP_LOGI(TAG,
                         "Dry soil confirmed (%d > %d, streak %d). Auto watering for %d ms",
                         moisture,
                         CONFIG_WATERING_DRY_THRESHOLD,
                         dry_streak,
                         auto_duration_ms);

                if (pump_run_for_ms(auto_duration_ms, "automatic dry soil")) {
                    last_watering = now;
                    set_last_watering_ts(last_watering);
                    dry_streak = 0;
                }
            } else if (dry_confirmed && !cooldown_ok) {
                time_t remaining_s = min_interval_s - (now - last_watering);
                ESP_LOGI(TAG, "Dry, but cooldown active. Next watering possible in %ld min",
                         (long)(remaining_s / 60));
            }

            next_check_us = now_us + check_interval_us;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void relay_self_test_task(void *pvParameter)
{
    (void)pvParameter;

    ESP_LOGW(TAG, "Relay self-test mode enabled: %d ms ON / %d ms OFF",
             CONFIG_WATERING_RELAY_SELF_TEST_INTERVAL_MS,
             CONFIG_WATERING_RELAY_SELF_TEST_INTERVAL_MS);

    while (1) {
        ESP_LOGI(TAG, "Relay self-test: PUMP ON");
        ESP_ERROR_CHECK(gpio_set_level(RELAY_GPIO, 0));
        vTaskDelay(pdMS_TO_TICKS(CONFIG_WATERING_RELAY_SELF_TEST_INTERVAL_MS));

        ESP_LOGI(TAG, "Relay self-test: PUMP OFF");
        ESP_ERROR_CHECK(gpio_set_level(RELAY_GPIO, 1));
        vTaskDelay(pdMS_TO_TICKS(CONFIG_WATERING_RELAY_SELF_TEST_INTERVAL_MS));
    }
}

void app_main(void)
{
    int check_interval_ms = clamp_int(CONFIG_WATERING_CHECK_INTERVAL_MS, 1000, 86400000);

    set_pump_enabled(1);
    set_pump_running(0);
    set_pump_duration_ms(CONFIG_WATERING_PUMP_ON_MS);

    relay_init();
    level_led_init();
    tank_level_sensor_init();
    tank_level_refresh_state(1);

#if CONFIG_WATERING_RELAY_SELF_TEST
    xTaskCreate(relay_self_test_task, "relay_self_test_task", 3072, NULL, 5, NULL);
    return;
#endif

    soil_adc_init();
    temperature_sensor_init();
    wifi_init_sta();
    time_init();
    mqtt_init();
    ESP_LOGI(TAG, "Pump duration initialized to %d s", get_pump_duration_ms() / 1000);
    ESP_LOGI(TAG, "Moisture check interval is %d s (%d min)",
             check_interval_ms / 1000, check_interval_ms / 60000);
    ESP_LOGI(TAG, "MQTT command arm-delay is %d ms", CONFIG_WATERING_MQTT_CMD_ARM_DELAY_MS);

    xTaskCreate(watering_task, "watering_task", 3072, NULL, 5, NULL);
#if CONFIG_WATERING_TEMPERATURE_ENABLED
    xTaskCreate(temperature_task, "temperature_task", 4096, NULL, 5, NULL);
#endif
}
