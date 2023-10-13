/* Edge Impulse ingestion SDK
 * AutoEntangle based on EDGE IMPULSE TinyML Model
 */

/* Include ----------------------------------------------------------------- */
#include "ingestion-sdk-platform/espressif_esp32/ei_wifi_esp32.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <stdio.h>
#include "ei_device_espressif_esp32.h"
#include "ei_at_handlers.h"
#include "ei_classifier_porting.h"
#include "ei_run_impulse.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "time.h"


#define NIGHT_MODE_START_HOUR      0 // Giờ bắt đầu chạy hàm ei_run_impulse
#define NIGHT_MODE_END_HOUR        6 // Giờ kết thúc chạy hàm ei_run_impulse
EiDeviceInfo *EiDevInfo = dynamic_cast<EiDeviceInfo *>(EiDeviceESP32::get_device());
static ATServer *at;

extern "C" void app_main() {
	ei_printf(
	"AUTOENTANGLE!!!\r\n"
	"Compiled on %s %s\r\n",
	__DATE__,
	__TIME__);
	
	// Connect to WIFI
	wifiInit();
	
	// Attach GPIO
    gpio_pad_select_gpio(GPIO_NUM_21);
    gpio_reset_pin(GPIO_NUM_21);

    gpio_pad_select_gpio(GPIO_NUM_22);
    gpio_reset_pin(GPIO_NUM_22);    
    
    gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT);    

    EiDeviceESP32* dev = static_cast<EiDeviceESP32*>(EiDeviceESP32::get_device());

    at = ei_at_init(dev);
    //ei_printf("Type AT+HELP to see a list of commands.\r\n");
    at->print_prompt();
	
    dev->set_state(eiStateFinished);

	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);

	int current_hour = timeinfo.tm_hour;

	// Check present time in range 6:00 to 18:00
	if (current_hour >= NIGHT_MODE_START_HOUR && current_hour < NIGHT_MODE_END_HOUR) {
		ei_printf("Time in legit range! Capturing and detecting...\r\n");
		// Capture image and count fruit flies
		ei_start_impulse(false, true, false);
			   
	}
}