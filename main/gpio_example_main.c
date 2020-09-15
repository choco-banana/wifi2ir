/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output
 * GPIO19: output
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Test:
 * Connect GPIO18 with GPIO4
 * Connect GPIO19 with GPIO5
 * Generate pulses on GPIO18/19, that triggers interrupt on GPIO4/5
 *
 */

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   22

typedef uint8_t ir_record[800];

const ir_record on_off = {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0, \
                          1,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0, \
						  1,0,1,0,1,0,1,0,0,0,1,0,0,0,1,0, \
						  0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0, \
						  0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0, \
						  0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0, \
						  1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0};
const uint8_t on_off_data[14] = {0xFF, 0x00, 0x88, 0x8A, 0xAA, 0x22, 0x2A, 0xAA, 0x2A, 0xAA, 0x28, 0x88, 0x88, 0x88};

typedef enum e_state{
	IDLE = 0,
	SEND_ON_OFF,
	SEND_ONE,
	SEND_TWO,
	SEND_THREE
}e_state;

static e_state state = SEND_ON_OFF; 
static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void ir_input_task(void * arg)
{
	while(1)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}
static void ir_output_task(void * arg)
{
	while(1)
	{
		vTaskDelay(100 / portTICK_RATE_MS);
		switch(state)
		{
			case IDLE:
			break;
			case SEND_ON_OFF:
				printf("Send ON_OFF.");
				
				state = IDLE;
			break;
			default:
				printf("shouldn't be here!");
		}
	}
}

void app_main()
{
	esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=50
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=1777,           		//Clock out at 1.777kHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        //.pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);
	ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
	
	//start ir input task
	xTaskCreate(ir_input_task, "ir_input_task", 2048, NULL, 10, NULL);
	//start ir output task
	xTaskCreate(ir_output_task, "ir_output_task", 2048, NULL, 5, NULL);
	
	
	static spi_transaction_t ir_out;
	memset(&ir_out, 0, sizeof(spi_transaction_t));
    //ir_out.flags=SPI_TRANS_USE_TXDATA;
	ir_out.tx_buffer = on_off_data;
	ir_out.length = 14*8;
	for (int i=0;i<10;i++)
	{
		ret=spi_device_queue_trans(spi, &ir_out, portMAX_DELAY);
		vTaskDelay(115 / portTICK_RATE_MS);
	}
	assert(ret==ESP_OK);
	
	
	
    int cnt = 0;
    while(1) {
        //printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_RATE_MS);
        //gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
        //gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
    }
}

