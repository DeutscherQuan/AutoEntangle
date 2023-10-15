# AutoEntange - An Insects monitorning system using ESP32 and SSD-MobilenetV2 FOMO model

AutoEntangle is equipped with advanced features and minimal resources on the ESP-EYE, achieves an impressive 97% F1 Score in validation dataset, with an efficient mean processing time of 6 seconds per image and peak RAM usage of 2.4Mb per task!

## Requirements

### Hardware

- Espressif ESP32 based development boards, preferably ESP-EYE (ESP32) and FireBeetle Board (ESP32). Using with other boards is possible, but code modifications is needed. For more on that read **Using with other ESP32 boards**.

### Tools
Install ESP IDF v4.4, following the instructions for your OS from [this page](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/get-started/index.html#installation-step-by-step).

### Building the application
Then from the firmware folder execute:
```bash
get_idf
clear && idf.py build 
```
```get_idf``` is an alias for export.sh script that sets up ESP IDF environment variables. Read more about it [here](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/get-started/index.html#step-4-set-up-the-environment-variables).

### Flash

Connect the ESP32 board to your computer.

Run:
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

Where ```/dev/ttyUSB0``` needs to be changed to actual port where ESP32 is connected on your system.

### Serial connection

Use screen, minicom or Serial monitor in Arduino IDE to set up a serial connection over USB. The following UART settings are used: 115200 baud, 8N1.

## Some experiments
These are some experiments of SSD-MobilenetV2 FOMO model deployment on ESP-EYE

### 
<a href="" target="blank"><img align="center"

src="[URL_TO_YOUR_IMAGE](https://www.messenger.com/messenger_media/?attachment_id=358351563203946&message_id=mid.%24cAAAB_VtvbPuRZQDiyWLM7s8C_sbW&thread_id=100073306429454)https://www.messenger.com/messenger_media/?attachment_id=358351563203946&message_id=mid.%24cAAAB_VtvbPuRZQDiyWLM7s8C_sbW&thread_id=100073306429454" height="100" /></a>

