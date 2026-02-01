#pragma once

#include <Arduino.h>
#include "driver/uart.h"
#include "driver/gpio.h"

// --- CONFIGURATION DEFAULTS ---
#define DMX_BUF_SIZE 513      // Start Code + 512 Channels
#define RX_BUF_SIZE 1024      // Hardware RX FIFO buffer
#define EVENT_QUEUE_SIZE 20   // UART event queue depth

class DMXReceiver {
public:
    DMXReceiver() : _running(false), _newFrameReceived(false), _taskHandle(NULL) {
        memset((void*)_dmxData, 0, DMX_BUF_SIZE);
    }

    ~DMXReceiver() {
        end();
    }

    bool begin(int rxPin, uart_port_t uartNum = UART_NUM_1) {
        if (_running) return true;

        _rxPin = rxPin;
        _uartNum = uartNum;

        uart_config_t uart_config = {
            .baud_rate = 250000,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_2,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };

        // Install driver with event queue
        esp_err_t err = uart_driver_install(_uartNum, RX_BUF_SIZE * 2, 0, EVENT_QUEUE_SIZE, &_dmxEventQueue, 0);
        if (err != ESP_OK) return false;

        if (uart_param_config(_uartNum, &uart_config) != ESP_OK) return false;

        if (uart_set_pin(_uartNum, UART_PIN_NO_CHANGE, _rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) return false;

        _running = true;

        int taskCore = 0;
    #if defined(portNUM_PROCESSORS) && (portNUM_PROCESSORS > 1)
        taskCore = 1;
    #endif
        xTaskCreatePinnedToCore(
            dmxTaskTrampoline, "DMX_RX_Task", 4096, this, 12, &_taskHandle, taskCore
        );

        return true;
    }

    void end() {
        if (!_running) return;
        _running = false;
        if (_taskHandle) {
            vTaskDelete(_taskHandle);
            _taskHandle = NULL;
        }
        uart_driver_delete(_uartNum);
    }

    uint8_t read(int channel) {
        if (channel < 1 || channel > 512) return 0;
        return _dmxData[channel];
    }

    /**
     * Copies the DMX buffer to your destination array.
     * @return true if a new frame has arrived since the last call.
     */
    bool readAll(uint8_t* dest, size_t size) {
        if (size > DMX_BUF_SIZE) size = DMX_BUF_SIZE;
        
        // Copy data
        memcpy(dest, (void*)_dmxData, size);
        
        // Check and reset the flag atomically-ish
        // (bool read/write is atomic enough for this use case on ESP32)
        if (_newFrameReceived) {
            _newFrameReceived = false;
            return true;
        }
        return false;
    }

    const volatile uint8_t* getBuffer() {
        return _dmxData;
    }

    bool isConnected() {
        return _dmxData[0] == 0; 
    }

private:
    bool _running;
    // Flag to track updates
    volatile bool _newFrameReceived;
    uart_port_t _uartNum;
    int _rxPin;
    TaskHandle_t _taskHandle;
    QueueHandle_t _dmxEventQueue;
    volatile uint8_t _dmxData[DMX_BUF_SIZE];

    static void dmxTaskTrampoline(void *pvParameters) {
        DMXReceiver* instance = (DMXReceiver*)pvParameters;
        instance->runTask();
    }

    void runTask() {
        uart_event_t event;
        uint8_t* dtmp = (uint8_t*) malloc(RX_BUF_SIZE);
        int currentChannel = 0;

        while (_running) {
            if (xQueueReceive(_dmxEventQueue, (void *)&event, (TickType_t)portMAX_DELAY)) {
                switch (event.type) {
                    case UART_BREAK:
                        // A break means the PREVIOUS frame is done and a new one is starting.
                        // If we read data in the previous cycle, mark frame as received.
                        if (currentChannel > 0) {
                            _newFrameReceived = true;
                        }
                        currentChannel = 0;
                        uart_flush_input(_uartNum);
                        break;

                    case UART_DATA:
                        uart_read_bytes(_uartNum, dtmp, event.size, portMAX_DELAY);
                        for (int i = 0; i < event.size; i++) {
                            if (currentChannel < DMX_BUF_SIZE) {
                                _dmxData[currentChannel] = dtmp[i];
                                currentChannel++;
                            }
                        }
                        break;

                    case UART_FIFO_OVF:
                    case UART_BUFFER_FULL:
                        uart_flush_input(_uartNum);
                        xQueueReset(_dmxEventQueue);
                        break;
                    default: break;
                }
            }
        }
        free(dtmp);
        vTaskDelete(NULL);
    }
};