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

// create a new api key and add it here 
#define API_KEY "AIzaSyCwBgvqH-j_TFMXijSNxnI2lC4f_l5zd3s"
// Copy your firebase real time database link here 
#define DATABASE_URL "https://smarttrap2022-9f9e7-default-rtdb.firebaseio.com/"  

#define USER_EMAIL "quannm293@gmail.com"  // This gmail does not exist outside your database. it only exists in the firebase project as a user
#define USER_PASSWORD "@Yasuo2002"      // Dont add your gmail credentials. Setup users authentication in your Firebase project first

#ifndef EI_RUN_IMPULSE_H
#define EI_RUN_IMPULSE_H

/* Include ----------------------------------------------------------------- */
#include <cstdint>

void ei_start_impulse(bool continuous, bool debug, bool use_max_uart_speed = false);
void ei_run_impulse(void);
void ei_stop_impulse(void);
bool is_inference_running(void);
void wifi_init_sta(void);
void wifiInit(void);
void send_signal_to_esp32(void);

#endif /* EI_RUN_IMPULSE_H */
