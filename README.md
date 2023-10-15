# AutoEntange - An Insects monitorning system using ESP32 and SSD-MobileNetV2 FOMO model

AutoEntangle, boasting advanced functionalities while conserving minimal resources on the ESP-EYE, not only achieves an impressive 97% F1 Score on the validation dataset but also operates efficiently with a mean processing time of just 6 seconds per image and peak RAM usage of 2.4Mb per task. Furthermore, the project encompasses a user-friendly dashboard built on Firebase as its database and utilizes Node-RED as the primary monitoring server.

## Requirements

### Hardware

- Espressif ESP32 based development boards, preferably ESP-EYE (ESP32) and FireBeetle Board (ESP32). Using with other boards is possible, but code modifications is needed. For more on that read **Using with other ESP32 boards**.

### Tools
- Install ESP IDF v4.4, following the instructions for your OS from [this page](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/get-started/index.html#installation-step-by-step).
- Clone this repository of course!?
  
### Build your own model?
If you are using Edge Impulse, do the following steps:
- Go to ```Deployment```, select and build model as C++ library, a folder will be downloaded automatically to your computer.
- Open the downloaded folder, copy three folders **edge-impulse-sdk**, **model-parameters** and **tflite-model** and paste to this repository.

### Building the application
- First, open the file **\edge-impulse\inference\ei_run_impulse.h** and configure your Firebase project information: 
```code
// create a new api key and add it here 
#define API_KEY "AIzaSyCwBgvqH-j_TFMXijSNxnI2lC4f_l5zd3s"
// Copy your firebase real time database link here 
#define DATABASE_URL "https://smarttrap2022-9f9e7-default-rtdb.firebaseio.com/"  

#define USER_EMAIL "quannm293@gmail.com"  // This gmail does not exist outside your database. it only exists in the firebase project as a user
#define USER_PASSWORD "123456788"      // Dont add your gmail credentials. Setup users authentication in your Firebase project first

#ifndef EI_RUN_IMPULSE_H
#define EI_RUN_IMPULSE_H
```

- Then from the firmware folder execute:
```bash
get_idf
clear && idf.py --no-ccache build
```
```get_idf``` is an alias for export.sh script that sets up ESP IDF environment variables. Read more about it [here](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/get-started/index.html#step-4-set-up-the-environment-variables).

#### Flash

Connect the ESP32 board to your computer.

Run:
   ```bash
   idf.py --no-ccache -p COM3 -b 115200 flash monitor
   ```

- ```COM3``` needs to be changed to actual port where ESP32 is connected on your system.
- The partition table needs to be adjusted to be compatible with your model and device.

### Serial connection

Use screen, minicom or Serial monitor in Arduino IDE to set up a serial connection over USB. The following UART settings are used: 115200 baud, 8N1.

## Some experiments
These are some experiments of SSD-MobileNetV2 FOMO model deployment on ESP-EYE

#### Stats comparison between two SSD-MobileNetV2 FOMO model Version
<p align="center">
  <img src="https://github.com/DeutscherQuan/AutoEntangleV01/assets/109386187/5a74cf23-dd75-4eb9-be80-3d9ceca8d643">
</p>

#### FOMO 0.1 model inference on ESP - EYE!
<p align="center">
  <img src="https://github.com/DeutscherQuan/AutoEntangleV01/assets/109386187/f4e6baaa-ca16-4d27-8470-76fe1165794b">
</p>




