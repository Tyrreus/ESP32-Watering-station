/* Watering station

This software was created to build watering station used to water tomatoes.

*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

#define GPIO_PUMP    14
#define GPIO_VALVE_1 25
#define GPIO_VALVE_2 26
#define GPIO_VALVE_3 27
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_PUMP) | (1ULL<<GPIO_VALVE_1) | (1ULL<<GPIO_VALVE_2) | (1ULL<<GPIO_VALVE_3))

#define GPIO_WATER_TANK 12
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_WATER_TANK))


static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t adc_mois_1 = ADC_CHANNEL_5; // GPIO_33
static const adc_channel_t adc_mois_2 = ADC_CHANNEL_6; // GPIO_34
static const adc_channel_t adc_mois_3 = ADC_CHANNEL_7; // GPIO_35

static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

static void check_efuse()
{
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}

static void initAdc() {
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();
    adc1_config_width(ADC_WIDTH_BIT_12);

    adc1_config_channel_atten(adc_mois_1, atten);
    adc1_config_channel_atten(adc_mois_2, atten);
    adc1_config_channel_atten(adc_mois_3, atten);

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);
}

static void initGpio() {
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

static uint32_t getMoisture(adc_channel_t ch) {
    uint32_t adc_reading = 0;
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        adc_reading += adc1_get_raw((adc1_channel_t)ch);
    }
    adc_reading /= NO_OF_SAMPLES;
    return ((adc_reading * 100) / 4095);
}

static void water_for(gpio_num_t valve_gpio, uint32_t sec) {
    gpio_set_level(valve_gpio, 1);
    gpio_set_level(GPIO_PUMP, 1);
    vTaskDelay(pdMS_TO_TICKS(sec*1000));
    gpio_set_level(valve_gpio, 0);
    gpio_set_level(GPIO_PUMP, 0);
}


void app_main()
{
    initAdc();
    initGpio();

    while (1) {
        printf("Moisture in percent, metter_1: %d metter_2: %d metter_3: %d\n", getMoisture(adc_mois_1), getMoisture(adc_mois_2), getMoisture(adc_mois_3));

        if (!gpio_get_level(GPIO_WATER_TANK)) {
            printf("There is no water left in tank. Abort.\n");
        }

        if (getMoisture(adc_mois_1) <= 50 && gpio_get_level(GPIO_WATER_TANK)) {
            water_for(GPIO_VALVE_1, 5);
        }
        if (getMoisture(adc_mois_2) <= 50 && gpio_get_level(GPIO_WATER_TANK)) {
            water_for(GPIO_VALVE_2, 5);
        }
        if (getMoisture(adc_mois_3) <= 50 && gpio_get_level(GPIO_WATER_TANK)) {
            water_for(GPIO_VALVE_3, 5);
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}


