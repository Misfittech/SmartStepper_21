/*
 * nzs.h
 *
 *  Created on: Dec 8, 2016
 *      Author: trampas
 *
	Copyright (C) 2018  MisfitTech,  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    Written by Trampas Stern for MisfitTech.

    Misfit Tech invests time and resources providing this open source code,
    please support MisfitTech and open-source hardware by purchasing
	products from MisfitTech, www.misifittech.net!
 *********************************************************************/

#ifndef NZS_H_
#define NZS_H_

#include "board.h"
#include "drivers/uart/uart.h"
//#include "drivers/rs485/rs485.h"
#include "stepper_controller.h"
#include "planner.h"

//#include "libraries/comsproto/comsproto.h"


typedef struct {
	char str[15];
} options_t;

typedef struct
{
	int64_t angle;
	uint16_t encoderAngle;
	uint8_t valid;
}eepromData_t;

typedef enum {
	NZS_STATUS_CLEARED=0,
	NZS_STATUS_HOMED = (0x01)<<0,
	NZS_STATUS_CAL = (0x01)<<1,
}nzs_status_t;

#define NZS_SET_STATUS(x,y) (x)=(nzs_status_t)((uint32_t)(x) | (uint32_t)(y))
#define NZS_CLEAR_STATUS(x,y) (x)=(nzs_status_t)((uint32_t)(x) & (~(uint32_t)(y)))

typedef enum {
	MSG_ACK_REQ =0,
	MSG_ACK_RESPONSE,
	MSG_NACK,
	MSG_SET_ID,
	MSG_GET_ID,
	MSG_CALIBRATE,
	MSG_TEST_CAL,
	MSG_SET_POS,
	MSG_GET_POS,
	MSG_HOME,
	MSG_STATUS,
} messages_t;


class NZS //nano Zero Stepper
{

	public:
		void begin(void);
		void loop(void);

	private:
		void processRS485(void);
		size_t processCommand(uint32_t src, uint8_t *ptrData, size_t len);
		size_t sendResponse(uint32_t dst, uint8_t *ptrData, size_t len);
		UART _debug_uart;
		//RS485 _rs485_uart;
		//ComsProto _rs485Coms;
		nzs_status_t _rs485_status; //bit field status
		

};


#endif /* NZS_H_ */
