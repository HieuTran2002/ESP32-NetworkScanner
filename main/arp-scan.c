#include <stdio.h>
#include <string.h>

// library
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// my code
#include "wifiConnect.h"
#include "scan.h"
#include "web.h"

// define
#define TAG "MAIN"
#define BLINK_LED 2


void LED_Blink(void *param){
    // GPIO pin initialize
    gpio_reset_pin(BLINK_LED);
    gpio_set_direction(BLINK_LED, GPIO_MODE_OUTPUT);

    while(1){
        gpio_set_level(BLINK_LED, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_LED, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    // connect to wifi
    connect_wifi();

    // create ARP scan task
    TaskHandle_t ScanTask = NULL;
    xTaskCreate(
        arpScan,
        "ARP_SCAN",
        5096,
        NULL,
        configMAX_PRIORITIES - 2,
        &ScanTask);

    // create LED Blink task
    // TaskHandle_t LEDBlinkTask = NULL;
    // xTaskCreate(
    //     LED_Blink,
    //     "LED_BLINK",
    //     2048,
    //     NULL,
    //     tskIDLE_PRIORITY,
    //     &LEDBlinkTask);

    // start web server
    start_webserver();
}
