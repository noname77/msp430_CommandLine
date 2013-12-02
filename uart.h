/*
 * This file is part of the MSP430 SPI example.
 *
 * Copyright (C) 2012 Stefan Wendler <sw@kaltpost.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/******************************************************************************
 * MSP430 command line
 *
 * Initialize UART on USCI_A at 115200 bauds.
 *
 * Wiktor Grajkowski
 * wiktor.grajkowski@gmail.com
 * http://flemingsociety.wordpress.com
 *****************************************************************************/

#ifndef __UART_H
#define __UART_H

#define MAX_COMMAND_LENGTH   20    // including \r
#define NO_OF_COMMANDS       4
#define MAX_PARAMS           2
#define MAX_PARAM_LEN        10
#define NO_OF_PARAM_STRINGS  4
/**
 * Initialize soft UART
 */
void uart_init(void);

/**
 * Write one chracter to the UART blocking.
 *
 * @param[in]        *c        the character to write
 */
void uart_putc(unsigned char c);

/**
 * Write string to the UART blocking.
 *
 * @param[in]        *str        the 0 terminated string to write
 */
void uart_puts(const char *str);

//print integer
void uart_printi(int n);

char* commands;
char* command_list[];
char* command_list_desc[];

char command_check(char command_[]);
void command_type_check (char command);
void payload_split(char* payload_);
void command_execution (char command);


#endif
