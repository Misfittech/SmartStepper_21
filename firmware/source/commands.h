/*
 * commands.h
 *
 *  Created on: Aug 2, 2017
 *      Author: tstern
 */

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "./drivers/UART/UART.h"

void commandsInit(UART *ptrUart);
int commandsProcess(void);


#endif /* COMMANDS_H_ */
