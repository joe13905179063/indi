/*******************************************************************************
  Copyright(c) 2018 Jasem Mutlaq. All rights reserved.

  Pegasus Ultimate Power Box (v1 and v2)

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/

#pragma once

#include "defaultdevice.h"
#include "indifocuserinterface.h"
#include "indiweatherinterface.h"
#include "indipowerinterface.h" // Include the new power interface
#include <stdint.h>

namespace Connection
{
class Serial;
}

class PegasusUPB : public INDI::DefaultDevice, public INDI::FocuserInterface, public INDI::WeatherInterface,
    public INDI::PowerInterface
{
    public:
        PegasusUPB();

        enum
        {
            UPB_V1,
            UPB_V2
        };

        virtual bool initProperties() override;
        virtual bool updateProperties() override;

        virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
        virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
        virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;

    protected:
        const char *getDefaultName() override;
        virtual bool saveConfigItems(FILE *fp) override;

        // Event loop
        virtual void TimerHit() override;

        // Focuser Overrides
        virtual IPState MoveAbsFocuser(uint32_t targetTicks) override;
        virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
        virtual bool AbortFocuser() override;
        virtual bool ReverseFocuser(bool enabled) override;
        virtual bool SyncFocuser(uint32_t ticks) override;

        virtual bool SetFocuserBacklash(int32_t steps) override;
        virtual bool SetFocuserBacklashEnabled(bool enabled) override;

        // Weather Overrides
        virtual IPState updateWeather() override
        {
            return IPS_OK;
        }

        // Power Interface Overrides
        virtual bool SetPowerPort(size_t port, bool enabled) override;
        virtual bool SetDewPort(size_t port, bool enabled, double dutyCycle) override;
        virtual bool SetVariablePort(size_t port, bool enabled, double voltage) override;
        virtual bool SetLEDEnabled(bool enabled) override;
        virtual bool SetAutoDewEnabled(size_t port, bool enabled) override;
        virtual bool CyclePower() override;
        virtual bool SetUSBPort(size_t port, bool enabled) override;

    private:
        bool Handshake();

        // Get Data
        bool setupParams();
        bool sendFirmware();
        bool getSensorData();
        bool getPowerData();
        bool getStepperData();
        bool getDewAggData();
        std::vector<std::string> split(const std::string &input, const std::string &regex);

        // Device Control
        bool reboot();

        // Power
        bool setPowerEnabled(uint8_t port, bool enabled);
        bool setPowerLEDEnabled(bool enabled);
        bool setPowerOnBoot();
        bool getPowerOnBoot();
        bool setAdjustableOutput(uint8_t voltage);

        // Dew
        bool setAutoDewEnabled(bool enabled);
        bool toggleAutoDewV2(size_t port, bool enabled);
        bool setAutoDewAgg(uint8_t value);
        bool setDewPWM(uint8_t id, uint8_t value);

        // USB
        bool setUSBHubEnabled(bool enabled);
        bool setUSBPortEnabled(uint8_t port, bool enabled);

        // Focuser
        bool setFocuserMaxSpeed(uint16_t maxSpeed);
        //    bool setFocuserBacklash(uint16_t value);
        //    bool setFocuserBacklashEnabled(bool enabled);

        /**
         * @brief sendCommand Send command to unit.
         * @param cmd Command
         * @param res if nullptr, respones is ignored, otherwise read response and store it in the buffer.
         * @return
         */
        bool sendCommand(const char *cmd, char *res);

        /**
         * @brief cleanupResponse Removes all spaces
         * @param response buffer
         */
        void cleanupResponse(char *response);

        /**
         * @return Return true if sensor data different from last data
         *
         * If the previous sensor data is empty then this will always
         * return true.
         */
        bool sensorUpdated(const std::vector<std::string> &result, uint8_t start, uint8_t end);

        /**
         * @return Return true if stepper data different from last data.
         *
         * If the previous stepper data is empty then this will always
         * return true.
         */
        bool stepperUpdated(const std::vector<std::string> &result, uint8_t index);

        int PortFD { -1 };
        bool setupComplete { false };

        Connection::Serial *serialConnection { nullptr };

        ////////////////////////////////////////////////////////////////////////////////////
        /// Main Control
        ////////////////////////////////////////////////////////////////////////////////////
        /// Reboot Device
        INDI::PropertySwitch RebootSP {1};

        // Power Consumption
        INDI::PropertyNumber PowerConsumptionNP {3};
        enum
        {
            CONSUMPTION_AVG_AMPS,
            CONSUMPTION_AMP_HOURS,
            CONSUMPTION_WATT_HOURS,
        };

        // Select which power is ON on bootup
        INDI::PropertySwitch PowerOnBootSP {4};
        enum
        {
            POWER_PORT_1,
            POWER_PORT_2,
            POWER_PORT_3,
            POWER_PORT_4
        };

        // Overcurrent status
        INDI::PropertyLight OverCurrentLP {7};
        enum
        {
            DEW_A,
            DEW_B,
            DEW_C
        };

        ////////////////////////////////////////////////////////////////////////////////////
        /// Dew Group
        ////////////////////////////////////////////////////////////////////////////////////

        // Auto Dew v2 Aggressiveness
        enum
        {
            AUTO_DEW_AGG,
        };
        INDI::PropertyNumber AutoDewAggNP {1};

        ////////////////////////////////////////////////////////////////////////////////////
        /// Focuser
        ////////////////////////////////////////////////////////////////////////////////////

        // Focuser speed
        INDI::PropertyNumber FocuserSettingsNP {1};
        enum
        {
            SETTING_MAX_SPEED,
        };

        ////////////////////////////////////////////////////////////////////////////////////
        /// Firmware
        ////////////////////////////////////////////////////////////////////////////////////
        INDI::PropertyText FirmwareTP {2};
        enum
        {
            FIRMWARE_VERSION,
            FIRMWARE_UPTIME
        };


        std::vector<std::string> lastSensorData, lastPowerData, lastStepperData, lastDewAggData;
        bool focusMotorRunning { false };
        char stopChar { 0xD };
        uint8_t version { UPB_V1 };

        static constexpr const uint8_t PEGASUS_TIMEOUT {3};
        static constexpr const uint8_t PEGASUS_LEN {128};
        static constexpr const char *ENVIRONMENT_TAB {"Environment"};
        static constexpr const char *POWER_TAB {"Power"};
        static constexpr const char *FIRMWARE_TAB {"Firmware"};
};
