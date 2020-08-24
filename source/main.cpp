/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <events/mbed_events.h>

#include <mbed.h>
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "DigitalIO.h"
#include "pretty_printer.h"

const static char DEVICE_NAME[] = "ButtonLED";

static EventQueue event_queue(/* event count */ 10 * EVENTS_EVENT_SIZE);


class DigitalIOdemo : ble::Gap::EventHandler {
public:
    DigitalIOdemo(BLE &ble, events::EventQueue &event_queue) :
        _ble(ble),
        _event_queue(event_queue),
        _led1(LED1, 1),
        _led3(LED3, 1),
        _actuated_led(BLE_LED_XEN_D2, 1),
        _button1(BUTTON1, PullUp),
        _button2(BLE_BUTTON_XEN_D3, BLE_BUTTON_PIN_PULL),
        _digitalio_service(NULL),
        _button_uuid(DigitalIOService::AUTOMATIONIO_SERVICE_UUID),
        _adv_data_builder(_adv_buffer) { }

    void start() {
        _ble.gap().setEventHandler(this);
        
        _server = &_ble.gattServer();

        
        // updates subscribtion handlers
        _server->onDataWritten(as_cb(&DigitalIOdemo::when_data_written));
        _server->onUpdatesEnabled(as_cb(&DigitalIOdemo::when_update_enabled));
        _server->onUpdatesDisabled(as_cb(&DigitalIOdemo::when_update_disabled));
        _server->onConfirmationReceived(as_cb(&DigitalIOdemo::when_confirmation_received));

        _ble.init(this, &DigitalIOdemo::on_init_complete);

        _event_queue.call_every(500ms, this, &DigitalIOdemo::blink);

        _event_queue.dispatch_forever();
    }

private:
    /** Callback triggered when the ble initialization process has finished */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *params) {
        if (params->error != BLE_ERROR_NONE) {
            serial.printf("Ble initialization failed.");
            return;
        }

        print_mac_address();

        /* Setup primary service. */

        _digitalio_service = new DigitalIOService(_ble, 0, 0, 1 /* initial values for button state & colour and led*/);

        _button1.fall(Callback<void()>(this, &DigitalIOdemo::button1_pressed));
        _button1.rise(Callback<void()>(this, &DigitalIOdemo::button1_released));

        _button2.fall(Callback<void()>(this, &DigitalIOdemo::button2_pressed));
        _button2.rise(Callback<void()>(this, &DigitalIOdemo::button2_released));

        start_advertising();
    }

    void start_advertising() {
        /* Create advertising parameters and payload */

        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(1000))
        );

        _adv_data_builder.setFlags();
        _adv_data_builder.setLocalServiceList(mbed::make_Span(&_button_uuid, 1));
        _adv_data_builder.setName(DEVICE_NAME);

        /* Setup advertising */

        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        if (error) {
            print_error(error, "_ble.gap().setAdvertisingParameters() failed");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(error, "_ble.gap().setAdvertisingPayload() failed");
            return;
        }

        /* Start advertising */

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            print_error(error, "_ble.gap().startAdvertising() failed");
            return;
        }
    }

    /**
     * This callback allows the DigitalIO Service to receive updates to the ledState Characteristic.
     *
     * @param[in] params Information about the characterisitc being updated.
     */
    void when_data_written(const GattWriteCallbackParams *params)
    {
        if ((params->handle == _digitalio_service->getLEDValueHandle()) && (params->len == 1)) {
            if (_actuated_led != *(params->data)) {
                _actuated_led = *(params->data);
                serial.printf("LED State is now %s\r\n", _actuated_led==1?"ON":"OFF");
            }
            else serial.printf("LED State is still %s\r\n", _actuated_led==1?"ON":"OFF");
        }
    }

    /**
     * Handler called after a client has subscribed to notification or indication.
     *
     * @param handle Handle of the characteristic value affected by the change.
     */
    void when_update_enabled(GattAttribute::Handle_t handle)
    {
        serial.printf("update enabled on handle %d\r\n", handle);
    }

    /**
     * Handler called after a client has cancelled his subscription from
     * notification or indication.
     *
     * @param handle Handle of the characteristic value affected by the change.
     */
    void when_update_disabled(GattAttribute::Handle_t handle)
    {
        serial.printf("update disabled on handle %d\r\n", handle);
    }

    /**
     * Handler called when an indication confirmation has been received.
     *
     * @param handle Handle of the characteristic value that has emitted the
     * indication.
     */
    void when_confirmation_received(GattAttribute::Handle_t handle)
    {
        serial.printf("confirmation received on handle %d\r\n", handle);
    }

    void button1_pressed(void) {
        _event_queue.call(Callback<void(bool)>(_digitalio_service, &DigitalIOService::updateButtonState), true);
    }

    void button1_released(void) {
        _event_queue.call(Callback<void(bool)>(_digitalio_service, &DigitalIOService::updateButtonState), false);
        _event_queue.call(Callback<void(void)>(when_button_pressed,1));
    }

    // Button 2 works in reverse....
    // ==============================
    void button2_pressed(void) {
        _event_queue.call(Callback<void(bool)>(_digitalio_service, &DigitalIOService::updateButtonColour), false);
        _event_queue.call(Callback<void(void)>(when_button_pressed,2));
    }

    void button2_released(void) {
        _event_queue.call(Callback<void(bool)>(_digitalio_service, &DigitalIOService::updateButtonColour), true);
    }

    void blink(void) {
        if (!cState) _led1 = !_led1;
    }

    // Message will be printed when button released
    static void when_button_pressed(uint8_t Msg) {
        // this does not run in the ISR so we create a queue
        serial.printf("Button %u was Pressed!\r\n", Msg);
    }
    
private:
    /**
     * Helper that construct an event handler from a member function of this
     * instance.
     */
    template<typename Arg>
    FunctionPointerWithContext<Arg> as_cb(void (DigitalIOdemo::*member)(Arg))
    {
        return makeFunctionPointer(this, member);
    }

    /* Connect and disconnect Event handlers */

    virtual void onConnectionComplete(const ble::ConnectionCompleteEvent&) {
        serial.printf("Now Connected...\r\n");
        cState = true;
        _led1 = 1;
        _led3 = 0;
        serial.printf("Starting LED State is %s\r\n", _actuated_led==1?"ON":"OFF");
    }

    virtual void onDisconnectionComplete(const ble::DisconnectionCompleteEvent&) {
        serial.printf("Now Disconnected\r\n");
        _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
        _led3 = 1;
        cState = false;
        serial.printf("LED State remains %s\r\n", _actuated_led==1?"ON":"OFF");
    }


private:
    BLE &_ble;
    GattServer* _server;
    events::EventQueue &_event_queue;

    DigitalOut  _led1;
    DigitalOut  _led3;
    DigitalOut  _actuated_led;
    InterruptIn _button1;
    InterruptIn _button2;
    DigitalIOService *_digitalio_service;

    UUID _button_uuid;

    bool cState = false;

    uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
    ble::AdvertisingDataBuilder _adv_data_builder;
};

/** Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context) {
    event_queue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(schedule_ble_events);

    DigitalIOdemo demo(ble, event_queue);
    demo.start();

    return 0;
}

