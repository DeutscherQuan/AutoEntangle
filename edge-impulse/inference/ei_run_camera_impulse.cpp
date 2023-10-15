/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Include -------------------------------------------------------------------------- */
#include "model-parameters/model_metadata.h"

#if defined(EI_CLASSIFIER_SENSOR) && EI_CLASSIFIER_SENSOR == EI_CLASSIFIER_SENSOR_CAMERA

/* ---------------------------------- C++ Library ----------------------------------- */
#include <iostream>
#include "nvs_flash.h"
#include "time.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* --------------------------------- ESP32 Library ---------------------------------- */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_firebase/app.h"
#include "esp_firebase/rtdb.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_netif.h"

/* ---------------------------- Firmware - AI Library ------------------------------- */
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "ei_camera.h"
#include "firmware-sdk/at_base64_lib.h"
#include "firmware-sdk/jpeg/encode_as_jpg.h"
#include "stdint.h"
#include "ei_device_espressif_esp32.h"
#include "ei_run_impulse.h"

/* ----------------------------- Firebase Realtime DB ------------------------------- */
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "freertos/event_groups.h"
#include "jsoncpp/value.h"
#include "jsoncpp/json.h"

#define GPIO_DEEP_SLEEP_DURATION     10  // sleep 10 seconds and then wake up
RTC_DATA_ATTR static time_t last;        // remember last boot in RTC Memory

using namespace ESPFirebase;
/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define EXAMPLE_ESP_WIFI_SSID      "Quan Tung"
#define EXAMPLE_ESP_WIFI_PASS      "123@5689"
#define EXAMPLE_ESP_MAXIMUM_RETRY  10
#define GPIO_DEEP_SLEEP_DURATION   60  

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define DWORD_ALIGN_PTR(a)   ((a & 0x3) ?(((uintptr_t)a + 0x4) & ~(uintptr_t)0x3) : a)
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_TAG "wifi station"

typedef enum {
    INFERENCE_STOPPED,
    INFERENCE_WAITING,
    INFERENCE_SAMPLING,
    INFERENCE_DATA_READY
} inference_state_t;


/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ei_printf("retry to connect to the AP\r\n"); 
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ei_printf("connect to the AP fail\r\n");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ei_printf("got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		ei_printf("\r\n");
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ---------------------------------- Wifi Connection ----------------------------------- */
void wifi_init_sta(void)
{
	ei_printf("Wifi connecting...\r\n");
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			{.ssid = EXAMPLE_ESP_WIFI_SSID},
			{.password = EXAMPLE_ESP_WIFI_PASS},
			.threshold = {
				.authmode = WIFI_AUTH_WPA2_PSK,
			},
			.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
		},
	};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ei_printf("wifi_init_sta finished.\r\n");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ei_printf("connected to ap SSID: %s || password: %s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ei_printf("Failed to connect to SSID: %s || password: %s/n",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ei_printf("UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}


void wifiInit(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(WIFI_TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
}

/* ---------------------------------- TCP Connection ----------------------------------- */
void send_signal_to_esp32(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ei_printf("Failed to create socket\n");
        return;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(1234);  // Port number used by QT Creator
    server_address.sin_addr.s_addr = inet_addr("192.168.1.6");  // IP address of QT Creator

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        ei_printf("Failed to connect to ESP32\n");
        close(sock);
        return;
    }

    short int signal_value = '0';  // Tín hiệu trống
    ssize_t sent_bytes = send(sock, &signal_value, sizeof(signal_value), 0);
    if (sent_bytes < 0) {
        ei_printf("Failed to send signal to ESP32\n");
    } else {
        ei_printf("Sent order to ESP32\n");
		ei_printf("Adhesive plate are being replaced...\n");
		ei_printf("Done!\n");
    }

    close(sock);
}

static inference_state_t state = INFERENCE_STOPPED;
static uint64_t last_inference_ts = 0;

static bool debug_mode = false;
static bool continuous_mode = false;

static uint8_t *snapshot_buf = nullptr;
static uint32_t snapshot_buf_size;

static ei_device_snapshot_resolutions_t snapshot_resolution;
static ei_device_snapshot_resolutions_t fb_resolution;

static bool resize_required = false;
static uint32_t inference_delay;


static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
{
    // we already have a RGB888 buffer, so recalculate offset into pixel index
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0) {
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix + 2];

        // go to the next pixel
        out_ptr_ix++;
        pixel_ix+=3;
        pixels_left--;
    }

    // and done!
    return 0;
}

/* ---------------------------------- AI Functions ----------------------------------- */
void ei_run_impulse(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);
    uint8_t *jpeg_image;
    uint32_t jpeg_image_size = 0;
    
    EiCameraESP32 *camera = static_cast<EiCameraESP32*>(EiCameraESP32::get_camera());

    ei_printf("Taking photo...\n");

    if(camera->ei_camera_capture_jpeg(&jpeg_image, &jpeg_image_size) == false) {
        ei_printf("ERR: Failed to take a snapshot!\n");
        return;
    }

    snapshot_buf = (uint8_t*)ei_malloc(snapshot_buf_size);

    // check if allocation was successful
    if(snapshot_buf == nullptr) {
        ei_printf("ERR: Failed to allocate snapshot buffer!\n");
        return;
    }

    if(camera->ei_camera_jpeg_to_rgb888(jpeg_image, jpeg_image_size, snapshot_buf) == false) {
        ei_printf("ERR: Failed to decode JPEG image\n");
        ei_free(snapshot_buf);
        ei_free(jpeg_image);
        return;
    }

    ei_free(jpeg_image);
    jpeg_image_size = 0;

    int64_t fr_start = esp_timer_get_time();

    if (resize_required) {
        ei::image::processing::crop_and_interpolate_rgb888(
            snapshot_buf,
            fb_resolution.width,
            fb_resolution.height,
            snapshot_buf,
            snapshot_resolution.width,
            snapshot_resolution.height);
    }
    int64_t fr_end = esp_timer_get_time();

    if (debug_mode) {
        ei_printf("Time resizing: %d\n", (uint32_t)((fr_end - fr_start)/1000));
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    // print and discard JPEG buffer before inference to free some memory
    if (debug_mode) {
        ei_printf("Begin output\n");
        ei_printf("\r\n");
    }

    // run the impulse: DSP, neural network and the Anomaly algorithm
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR ei_error = run_classifier(&signal, &result, false);
    if (ei_error != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run impulse (%d)\n", ei_error);
        ei_free(snapshot_buf);
        return;
    }
    ei_free(snapshot_buf);

    // print the predictions
    ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                result.timing.dsp, result.timing.classification, result.timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    int num_valid_bounding_boxes = 0;
		
	// Config and Authentication
	user_account_t account = {USER_EMAIL, USER_PASSWORD};

	FirebaseApp app = FirebaseApp(API_KEY);

	app.loginUserAccount(account);

	RTDB db = RTDB(&app, DATABASE_URL);

	
    for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {
        auto bb = result.bounding_boxes[ix];
        if (bb.value > 0) {
            num_valid_bounding_boxes++;
            ei_printf("    %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\n", bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
        }
    }
    
    ei_printf("fruit_fly_num: %d\n", num_valid_bounding_boxes);
	
	if (num_valid_bounding_boxes >= 3) {
		// Tạo JSON object và gán giá trị cho nó
		Json::Value num_boxes_data;
		num_boxes_data["Yellow Flies Quantity"] = num_valid_bounding_boxes;

		// Chuyển JSON object thành chuỗi JSON
		Json::FastWriter writer;
		std::string num_boxes_json = writer.write(num_boxes_data);

		// Gửi dữ liệu lên Firebase Realtime Database
		db.putData("/YellowFliesQuantity", num_boxes_json.c_str());
		ei_printf("Data sent to Firebase Realtime Database!\n");
		send_signal_to_esp32();
		
	}
    
    if (num_valid_bounding_boxes == 0) {
        ei_printf("    No objects found\n");
    }
#else
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label,
                                    result.classification[ix].value);
    }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif
#endif

    if (debug_mode) {
        ei_printf("\r\n----------------------------------\r\n");
        ei_printf("End output\r\n");
		ei_printf("We will be back in 1 minute\n");
		printf("deep sleep (%lds since last reset, %lds since last boot)\n",now.tv_sec,now.tv_sec-last);
		last = now.tv_sec;
		// sleep duration time configuration here!
		esp_deep_sleep(1000000LL * GPIO_DEEP_SLEEP_DURATION);
		
    }
}

/* ------------------------------- CALL AI Function with Requirements -------------------------------- */
void ei_start_impulse(bool continuous, bool debug, bool use_max_uart_speed)
{
	esp_err_t smartconfig_init();
    snapshot_resolution.width = EI_CLASSIFIER_INPUT_WIDTH;
    snapshot_resolution.height = EI_CLASSIFIER_INPUT_HEIGHT;

    debug_mode = debug;
    continuous_mode = continuous;

    EiDeviceESP32* dev = static_cast<EiDeviceESP32*>(EiDeviceESP32::get_device());
    EiCameraESP32 *camera = static_cast<EiCameraESP32*>(EiCameraESP32::get_camera());

    /* check if minimum suitable sensor resolution is the same as
	   desired snapshot resolution
	   if not we need to resize later
	*/
	
    fb_resolution = camera->search_resolution(snapshot_resolution.width, snapshot_resolution.height);

    if (snapshot_resolution.width != fb_resolution.width || snapshot_resolution.height != fb_resolution.height) {
        resize_required = true;
    }

    if (!camera->init(snapshot_resolution.width, snapshot_resolution.height)) {
        ei_printf("Failed to init camera, check if camera is connected!\n");
        return;
    }

    snapshot_buf_size = fb_resolution.width * fb_resolution.height * 3;

    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tImage resolution: %dx%d\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    if(continuous_mode == true) {
        inference_delay = 0;
        state = INFERENCE_DATA_READY;
    }
    else {
        inference_delay = 5000;
        state = INFERENCE_WAITING;
        ei_printf("Starting inferencing in %d seconds...\n", inference_delay / 1000);
		vTaskDelay(inference_delay / portTICK_PERIOD_MS);
    }

    if (use_max_uart_speed) {
        ei_printf("OK\r\n");
        ei_sleep(100);
        dev->set_max_data_output_baudrate();
        ei_sleep(100);
    }

   while(!ei_user_invoke_stop()) {
      ei_run_impulse();
      ei_sleep(60000);
   }

    ei_stop_impulse();

    if (use_max_uart_speed) {
        ei_printf("\r\nOK\r\n");
        ei_sleep(100);
        dev->set_default_data_output_baudrate();
        ei_sleep(100);
    }

}

/* ---------------------------------- Stop Inferencing ----------------------------------- */
void ei_stop_impulse(void)
{
    state = INFERENCE_STOPPED;
}

bool is_inference_running(void)
{
    return (state != INFERENCE_STOPPED);
}

#endif /* defined(EI_CLASSIFIER_SENSOR) && EI_CLASSIFIER_SENSOR == EI_CLASSIFIER_SENSOR_CAMERA */