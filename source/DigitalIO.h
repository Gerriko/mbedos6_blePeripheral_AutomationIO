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

#ifndef __BLE_BUTTON_SERVICE_H__
#define __BLE_BUTTON_SERVICE_H__

#include <mbed.h>
#include "ble/BLE.h"
#include "ble/Gap.h"

class DigitalIOService {
public:
    const static uint16_t AUTOMATIONIO_SERVICE_UUID             = 0x1815;
    const static uint16_t DIGITAL_CHARACTERISTIC_UUID           = 0x2A56;
    const static uint16_t DIGITAL_USERDESCRIPTION_UUID          = 0x2901;
    uint8_t BUTTONSTATE_USRDESCR[13] = "Button State";
    uint8_t BUTTONCOLOUR_USRDESCR[14] = "Button Colour";
    uint8_t LEDSTATE_USRDESCR[10] = "LED State";

    DigitalIOService(BLE &_ble, uint8_t button1PressedInitial, uint8_t button2PressedInitial, uint8_t ledStateInitial) :
        ble(_ble), 
        buttonState(
            DIGITAL_CHARACTERISTIC_UUID, 
            &button1PressedInitial, 
            GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
            &ButtonState_attr, 1),
        buttonColour(
            DIGITAL_CHARACTERISTIC_UUID, 
            &button2PressedInitial, 
            GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
            &ButtonColour_attr, 1),
        ledState(DIGITAL_CHARACTERISTIC_UUID,
            &ledStateInitial,
            GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NONE,
            &LedState_attr, 1)
    {
        ButtonState_attr = new GattAttribute( DIGITAL_USERDESCRIPTION_UUID, // attribute type
            BUTTONSTATE_USRDESCR, sizeof(BUTTONSTATE_USRDESCR), 20, // length of the buffer containing the value
            true // variable length
        );
        ButtonColour_attr = new GattAttribute( DIGITAL_USERDESCRIPTION_UUID, // attribute type
            BUTTONCOLOUR_USRDESCR, sizeof(BUTTONCOLOUR_USRDESCR), 20, // length of the buffer containing the value
            true // variable length
        );
        LedState_attr = new GattAttribute( DIGITAL_USERDESCRIPTION_UUID, // attribute type
            LEDSTATE_USRDESCR, sizeof(LEDSTATE_USRDESCR), 20, // length of the buffer containing the value
            true // variable length
        );
        GattCharacteristic *charTable[3] = {&buttonState, &buttonColour, &ledState};
        GattService         DigitalIOService(DigitalIOService::AUTOMATIONIO_SERVICE_UUID, charTable, sizeof(charTable) / sizeof(charTable[0]));
        ble.gattServer().addService(DigitalIOService);
    }

    void updateButtonState(uint8_t newState) {
        ble.gattServer().write(buttonState.getValueHandle(), (uint8_t *)&newState, sizeof(uint8_t));
    }

    void updateButtonColour(uint8_t newColour) {
        ble.gattServer().write(buttonColour.getValueHandle(), (uint8_t *)&newColour, sizeof(uint8_t));
    }
    
    GattAttribute::Handle_t getLEDValueHandle() const
    {
        return ledState.getValueHandle();
    }
    
private:
    BLE                                     &ble;
    GattAttribute                           *ButtonState_attr;
    GattAttribute                           *ButtonColour_attr;
    GattAttribute                           *LedState_attr;
    ReadOnlyGattCharacteristic<uint8_t>     buttonState;
    ReadOnlyGattCharacteristic<uint8_t>     buttonColour;
    WriteOnlyGattCharacteristic<uint8_t>    ledState;
};

#endif /* #ifndef __BLE_BUTTON_SERVICE_H__ */
