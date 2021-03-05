/*
 * commands.cpp
 *
 *  Created on: Aug 2, 2017
 *      Author: tstern
 */
#include "./libraries/command/command.h"
#include "commands.h"
#include "./drivers/delay/delay.h"
#include "./startup/system_samd51.h"
#include "./drivers/adc/adc.h"


#undef PGM_P
#define COMMANDS_PROMPT (":>")
sCmdUart dbgUart;


#define CMD_X(name, str)
#define COMMANDS_TABLE \
		CMD_X(help,"Displays this message") \
		CMD_X(version, "prints firmware build date") \
		CMD_X(memory, "Displays the heap and stack usage") \
		CMD_X(reboot, "reboots unit") \


#undef CMD_X


/*lint -save -e(19) Not useless */
//create the help string table
#define CMD_X(name, str) CMD_STR(name, str);
COMMANDS_TABLE
#undef CMD_X


//create the command table
/*lint -restore  Not useless */
#define CMD_X(name, str) COMMAND(name),
sCommand Cmds[] =
{
		COMMANDS_TABLE
		{"",0,""}, //End of list signal
};
#undef CMD_X

static int version_cmd(sCmdUart *ptrUart, __attribute__((unused)) int argc, __attribute__((unused)) char * argv[])
{
	CommandPrintf(ptrUart, "FW: " FW_VERSION_STR", build date: " __DATE__" " __TIME__ "\n\r");
	return 0;
}

static int reboot_cmd(sCmdUart *ptrUart, __attribute__((unused)) int argc, __attribute__((unused)) char * argv[])
{
	CommandPrintf(ptrUart, "rebooting\n\r");
	delay_ms(1000);
	NVIC_SystemReset();
	return 0;
}

// print out the stack and heap usage
static int memory_cmd(sCmdUart *ptrUart,__attribute__((unused))  int argc, __attribute__((unused)) char * argv[])
{
	CommandPrintf(ptrUart, "Heap used %" PRIu32 " bytes, out of %d bytes,%" PRIu32 "%%\r\n",
			getHeapUsed(), getHeapSize(), (100ul * getHeapUsed()) / getHeapSize());
	CommandPrintf(ptrUart, "Stack used %" PRIu32 " bytes, out of %d bytes, %" PRIu32 "%%\r\n",
			getStackUsed(),getStackSize(), (100ul * getStackUsed()) / getStackSize());
	return 0;
}

// print out the help strings for the commands
static int help_cmd(sCmdUart *ptrUart, __attribute__((unused)) int argc, __attribute__((unused)) char * argv[])
{
	sCommand cmd_list;
	int i;

	//now let's parse the command
	i=0;
	memcpy(&cmd_list, &Cmds[i], sizeof(sCommand));
	while(cmd_list.function!=0)
	{
		if (strlen(cmd_list.help)>0)
		{
			CommandPrintf(ptrUart,(cmd_list.name));
			CommandPrintf(ptrUart,(" - "));
			CommandPrintf(ptrUart,(cmd_list.help));
			CommandPrintf(ptrUart,("\n\r"));
		}
		i=i+1;
		memcpy(&cmd_list, &Cmds[i], sizeof(sCommand));
	}
	return 0;
}



void commandsInit(UART *ptrUart)
{
	CommandInit(&dbgUart, ptrUart); //set up the UART structure

}


int commandsProcess(void)
{
	return CommandProcess(&dbgUart, Cmds, (char) ' ', (char *) COMMANDS_PROMPT);
}
