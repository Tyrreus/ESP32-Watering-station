/* Watering station

This software was created to build watering station for plants.

This version can handle many channels to do watering when soil moisture will
drop below specific level (can be set for each channel).

*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

static const int kNoOfSamples = 64;
static const adc_atten_t kAtten = ADC_ATTEN_DB_11;
static const gpio_num_t kPumpGPIO = 14;
static const gpio_num_t kTankGPIO = 13;
static const gpio_num_t kEmptyLedIndicatorGPIO = 23;
static const gpio_num_t kValve1GPIO = 25;
static const gpio_num_t kValve2GPIO = 26;
static const gpio_num_t kValve3GPIO = 27;
static const uint32_t kCheckTimeSec = 1;
static const uint32_t kMsInSec = 1000;

static uint64_t current_gpio_output_pin_mask = 0;
static uint64_t current_gpio_input_pin_mask = 0;
static size_t waterting_channels_cnt = 0;

typedef struct {
    gpio_num_t pump_gpio;
    void (*start)(void *self);
    void (*stop)(void *self);
} tPump;

typedef struct {
    gpio_num_t tank_gpio;
    bool (*isEmpty)(void *self);
} tTank;

typedef struct {
    gpio_num_t valve_gpio;
    void (*open)(const void *self);
    void (*close)(const void *self);
} tValve;

typedef struct {
    tValve valve;
    tPump *pump;
    tTank *tank;
    adc_channel_t adc_ch;
    uint8_t watering_time_sec;
    uint8_t min_moisture;
    uint32_t (*getMoisture)(const void* self);
} tWateringChannel;

static uint32_t read_adc(const adc_channel_t ch) {
    uint32_t adc_reading = 0;
    for (int i = 0; i < kNoOfSamples; i++) {
        adc_reading += adc1_get_raw((adc1_channel_t)ch);
    }
    return adc_reading /= kNoOfSamples;
}

static uint32_t getMoist(const tWateringChannel *ch) {
    uint32_t moist = read_adc(ch->adc_ch);
    return ((moist * 100) / 4095); // Calculate percent without fraction.
}

static void initAdcChannel(const adc_channel_t adc_ch) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(adc_ch, kAtten);
}

static void initGpioAsOutput(const gpio_num_t gpio) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    current_gpio_output_pin_mask |= (1ULL<<gpio);
    io_conf.pin_bit_mask = current_gpio_output_pin_mask;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

static void initGpioAsInpit(const gpio_num_t gpio) {
    gpio_config_t io_conf;
    current_gpio_input_pin_mask |= (1ULL<<gpio);
    io_conf.pin_bit_mask = current_gpio_input_pin_mask;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

static void openValve(tValve *valve) {
    gpio_set_level(valve->valve_gpio, 1);
}

static void closeValve(tValve *valve) {
    gpio_set_level(valve->valve_gpio, 0);
}

static void startPump(tPump *pump) {
    gpio_set_level(pump->pump_gpio, 1);
}

static void stopPump(tPump *pump) {
    gpio_set_level(pump->pump_gpio, 0);
}

static bool isTankEmpty(tTank *tank) {
    if (gpio_get_level(tank->tank_gpio)) {
        return true;
    }
    return false;
}

void initChanel(tWateringChannel *ch) {
    initGpioAsOutput(ch->valve.valve_gpio);
    initGpioAsOutput(ch->pump->pump_gpio);
    initGpioAsInpit(ch->tank->tank_gpio);
    initAdcChannel(ch->adc_ch);
}

void checkAndWaterSingle(tWateringChannel *ch) {
    uint32_t curr_moisture = ch->getMoisture(ch);
    tPump *pump = ch->pump;
    tTank *tank = ch->tank;
    tValve *valve = &ch->valve;

    if (!tank->isEmpty(tank)) {
        gpio_set_level(kEmptyLedIndicatorGPIO, 0);
        if (curr_moisture < ch->min_moisture) {
            valve->open(valve);
            pump->start(pump);
            vTaskDelay(pdMS_TO_TICKS(ch->watering_time_sec * kMsInSec));
            pump->stop(pump);
            valve->close(valve);
        }
    } else {
        printf("Tank is empty!\n");
        gpio_set_level(kEmptyLedIndicatorGPIO, 1);
        // Light LED up.
    }
}

void checkAndWaterAll(tWateringChannel *ch) {
    for (size_t i = 0; i < waterting_channels_cnt; ++i) {
        checkAndWaterSingle(&ch[i]);
    }
}

void app_main()
{
    tPump common_pump = { .pump_gpio = kPumpGPIO, .start = (void*) startPump, .stop = (void*) stopPump};
    tTank common_tank = { .tank_gpio = kTankGPIO, .isEmpty = (void*) isTankEmpty};

    tWateringChannel channels[] = {
            {
                .valve = { .valve_gpio = kValve1GPIO, .open = (void*) openValve, .close = (void*) closeValve},
                .pump = &common_pump,
                .tank = &common_tank,
                .adc_ch = ADC_CHANNEL_5,
                .watering_time_sec = 5,
                .min_moisture = 50,
                .getMoisture = (void*) getMoist
            },
            {
                .valve = { .valve_gpio = kValve2GPIO, .open = (void*) openValve, .close = (void*) closeValve},
                .pump = &common_pump,
                .tank = &common_tank,
                .adc_ch = ADC_CHANNEL_6,
                .watering_time_sec = 5,
                .min_moisture = 50,
                .getMoisture = (void*) getMoist
            },
            {
                .valve = { .valve_gpio = kValve3GPIO, .open = (void*) openValve, .close = (void*) closeValve},
                .pump = &common_pump,
                .tank = &common_tank,
                .adc_ch = ADC_CHANNEL_7,
                .watering_time_sec = 5,
                .min_moisture = 50,
                .getMoisture = (void*) getMoist
            },
    };

    waterting_channels_cnt = (sizeof(channels)/sizeof(tWateringChannel));

    for (size_t s = 0; s < waterting_channels_cnt; ++s) {
        initChanel(&channels[s]);
    }

    initGpioAsOutput(kEmptyLedIndicatorGPIO);

    while(1) {
        printf("Moisture: ");
        for (size_t s = 0; s < waterting_channels_cnt; ++s) {
            printf("channel[%d]: %d, ", s, channels[s].getMoisture(&channels[s]));
        }
        printf("\n");

        checkAndWaterAll(channels);

        vTaskDelay(pdMS_TO_TICKS(kCheckTimeSec * kMsInSec));
    }
}


