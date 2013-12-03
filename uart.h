/* 
 * This file is part of msp430 command line example
 *
 * Copyright (C) 2013 Wiktor Grajkowski <wiktor.grajkowski AT gmail.com>
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
 * This program lets you control your MSP430 in an easy way through terminal
 * like commands. Syntax: command param1 param2 ... You can specify the
 * commands, number and type of parameters, display simple help with commands'
 * descriptions. It can handle string, decimal, hex or binary parameters.
 *
 * For a tutorial and demo visit http://flemingsociety.wordpress.com
 *
 *
 * Wiktor Grajkowski
 * wiktor.grajkowski AT gmail.com
 * http://flemingsociety.wordpress.com
 ******************************************************************************/

#ifndef __UART_H
#define __UART_H

#define MAX_COMMAND_LENGTH   20    // including \r
#define NO_OF_COMMANDS       4
#define MAX_PARAMS           2
#define MAX_PARAM_LEN        11
#define NO_OF_PARAM_STRINGS  4
#define MAX_NUM_AS_PARAM     4294967296  // long (32 bit)
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
void uart_printi(long n);

char* commands;
char* command_list[];
char* command_list_desc[];

void command_check(char command_[]);
void command_type_check (char command);
void payload_split(char* payload_);
void command_execution (char command);


#endif
