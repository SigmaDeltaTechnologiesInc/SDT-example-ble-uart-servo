/* SDT-example-ble-uart-echo
 * 
 * Copyright (c) 2018 Sigma Delta Technologies Inc.
 * 
 * MIT License
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/services/UARTService.h"

/* Serial */
#define BAUDRATE 9600
Serial serial_pc(USBTX, USBRX, BAUDRATE);

/* DigitalOut */
DigitalOut do_servo0(PWM0);     // GPIO2
// DigitalOut do_servo1(PWM1);     // GPIO3, you cannot use PWM1.
// DigitalOut do_servo2(PWM2);     // GPIO0, you cannot use PWM2.
// DigitalOut do_servo3(PWM3);     // GPIO1, you cannot use PWM3.
DigitalOut* pDo_servo = &do_servo0;

/* Ticker */
Ticker ticker_pwm;

/* BLE */
#define BLE_DEVICE_NAME "SDT Device"
BLE& ble_SDTDevice = BLE::Instance();      // you can't use this name, 'ble', because this name is already declared in UARTService.h

/* UART service */
UARTService* pUartService;

/* Variable */
bool b_bleConnect = false;

unsigned char pwm_periodMs = 20;   // 20ms
float pwm_duty = 0.100;



void setPwmDuty(char num) {
    float duty;

    // serial_pc.printf("num : %c\n", num);

    switch(num) {
        case '0':
            duty = 0.050;
            break;
        case '1':
            duty = 0.060;
            break;
        case '2':
            duty = 0.070;
            break;
        case '3':
            duty = 0.080;
            break;
        case '4':
            duty = 0.090;
            break;
        case '5':
            duty = 0.100;
            break;
        default:
            duty = 0.100;
            break;
    }

    // serial_pc.printf("duty : %f\n", duty);

    pwm_duty = duty;
}

void callbackTickerPwm(void) {
    int timeUs = pwm_periodMs * (pwm_duty * 1000);
    // serial_pc.printf("duty : %d\n", time);

    *pDo_servo = 1;
    wait_us(timeUs);
    *pDo_servo = 0;
}

void callbackBleDataWritten(const GattWriteCallbackParams* params) {
    if ((pUartService != NULL) && (params->handle == pUartService->getTXCharacteristicHandle())) {
        uint16_t bytesRead = params->len;
        const uint8_t* pBleRxBuf = params->data;
        ble_SDTDevice.gattServer().write(pUartService->getRXCharacteristicHandle(), pBleRxBuf, bytesRead);

        // uint8_t *data = 0;
        // sscanf((const char*)pBleRxBuf, "%d", &data);
        // serial_pc.printf("data : %d\n", data);

        // serial_pc.printf("data : %c\n", *pBleRxBuf);
        ticker_pwm.detach();
        setPwmDuty(*pBleRxBuf);
        ticker_pwm.attach(callbackTickerPwm, 0.02);
    }
}

void callbackBleConnection(const Gap::ConnectionCallbackParams_t* params) {
    serial_pc.printf("Connected!\n");
    b_bleConnect = true;
    ticker_pwm.attach(callbackTickerPwm, 0.02);
}

void callbackBleDisconnection(const Gap::DisconnectionCallbackParams_t* params) {
    serial_pc.printf("Disconnected!\n");
    serial_pc.printf("Restarting the advertising process\n\r");
    b_bleConnect = false;
    ticker_pwm.detach();
    ble_SDTDevice.gap().startAdvertising();
}

void callbackBleInitComplete(BLE::InitializationCompleteCallbackContext* params) {
    BLE& ble = params->ble;                     // 'ble' equals ble_SDTDevice declared in global
    ble_error_t error = params->error;          // 'error' has BLE_ERROR_NONE if the initialization procedure started successfully.

    if (error == BLE_ERROR_NONE) {
        serial_pc.printf("Initialization completed successfully\n");
    }
    else {
        /* In case of error, forward the error handling to onBleInitError */
        serial_pc.printf("Initialization failled\n");
        return;
    }

    /* Ensure that it is the default instance of BLE */
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        serial_pc.printf("ID of BLE instance is not DEFAULT_INSTANCE\n");
        return;
    }

    /* Setup UARTService */
    pUartService = new UARTService(ble);

    /* Setup and start advertising */
    ble.gattServer().onDataWritten(callbackBleDataWritten);
    ble.gap().onConnection(callbackBleConnection);
    ble.gap().onDisconnection(callbackBleDisconnection);
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000);     // Advertising interval in units of milliseconds
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME, (const uint8_t *)BLE_DEVICE_NAME, sizeof(BLE_DEVICE_NAME) - 1);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS, (const uint8_t *)UARTServiceUUID_reversed, sizeof(UARTServiceUUID_reversed));
    ble.gap().startAdvertising();
    serial_pc.printf("Start advertising\n");
}

int main(void) {
    serial_pc.printf("< Sigma Delta Technologies Inc. >\n\r");

    /* Init BLE */
    // ble_SDTDevice.onEventsToProcess(callbackEventsToProcess);
    ble_SDTDevice.init(callbackBleInitComplete);

    while (true) {
        ble_SDTDevice.waitForEvent();
    }

    return 0;
}
