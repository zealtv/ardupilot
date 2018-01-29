/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

/*
  AP_Radio implementation for CC2500 2.4GHz radio. 

  With thanks to cleanflight and betaflight projects
 */

#include "AP_Radio_backend.h"
#if CONFIG_HAL_BOARD == HAL_BOARD_PX4
#include <nuttx/arch.h>
#include <systemlib/systemlib.h>
#include <drivers/drv_hrt.h>
#elif CONFIG_HAL_BOARD == HAL_BOARD_CHIBIOS
#include "hal.h"
#endif
#include "telem_structure.h"
#include "driver_bk2425.h"

#define BEKEN_MAX_CHANNELS 8

class AP_Radio_beken : public AP_Radio_backend
{
public:
	// Override base class
    AP_Radio_beken(AP_Radio &radio);
    bool init(void) override; // init - initialise radio
    bool reset(void) override; // rest radio
    bool send(const uint8_t *pkt, uint16_t len) override; // send a packet
    void start_recv_bind(void) override; // start bind process as a receiver
    uint32_t last_recv_us(void) override; // return time in microseconds of last received R/C packet
    uint8_t num_channels(void) override; // return number of input channels
    uint16_t read(uint8_t chan) override; // return current "PWM" (value) of a channel
    void handle_data_packet(mavlink_channel_t chan, const mavlink_data96_t &m) override; // handle a data96 mavlink packet for fw upload
    void update(void) override; // update status
    uint32_t get_tx_version(void) override { return 0; } // get TX fw version
    const AP_Radio::stats &get_stats(void) override; // get radio statistics structure

	// Extra public functions
    void set_wifi_channel(uint8_t channel) { lastWifiChannel = channel; } // set the 2.4GHz wifi channel used by companion computer, so it can be avoided
    
private:
    AP_HAL::OwnPtr<AP_HAL::SPIDevice> dev;
    
    // Static data, for interrupt support
    static AP_Radio_beken *radio_instance;
    static thread_t *_irq_handler_ctx;
    static virtual_timer_t timeout_vt;

	// Static functions, for interrupt support
    static void irq_handler_thd(void* arg);
    static void trigger_irq_radio_event(void);
    static void trigger_timeout_event(void *arg);

	//  Private functions
    void radio_init(void);
	void ProcessPacket(const uint8_t* packet, uint8_t rxaddr);
    void setChannel(uint8_t channel);
    void nextChannel(uint8_t skip);
    void parse_frSkyX(const uint8_t *packet);
    uint16_t calc_crc(uint8_t *data, uint8_t len);
    bool check_crc(uint8_t ccLen, uint8_t *packet);
    void send_telemetry(void);
    void irq_handler(void);
    void irq_timeout(void);
    void save_bind_info(void);
    bool load_bind_info(void);
	void UpdateFccScan(void);
	void UpdateTxData(void);

    // semaphore between ISR and main thread
    AP_HAL::Semaphore *sem;    

    AP_Radio::stats stats;
    AP_Radio::stats last_stats;

    uint16_t pwm_channels[BEKEN_MAX_CHANNELS]; // Channel data

    Radio_Beken beken;

    uint8_t rxNum;
    uint8_t channr; // Corresponds to txChannel on the button board
    uint8_t chanskip;
    uint32_t packet_timer;
    static uint32_t irq_time_us;
    const uint32_t sync_time_us = 9000;
    uint8_t chan_count;
    uint32_t lost;
    uint32_t timeouts;
    bool have_bind_info;
    uint8_t packet3;
    bool telem_send_rssi;
    float rssi_filtered;

    uint32_t timeTunedMs;

    // bind structure saved to storage
    static const uint16_t bind_magic = 0x120a;
    struct PACKED bind_info {
        uint16_t magic;
        uint8_t bindTxId[5];
    };
    enum {
        STATE_INIT = 0,
        STATE_BIND,
        STATE_DATA,
        STATE_DFU,
        STATE_FCCTEST,
    } protocolState;

    struct config {
        uint8_t reg;
        uint8_t value;
    };
    static const config radio_config[];

    struct telem_status t_status;
    uint32_t last_pps_ms;
    
    ITX_SPEED spd;
    uint8_t myDroneId[4];
    uint8_t lastWifiChannel;
};

