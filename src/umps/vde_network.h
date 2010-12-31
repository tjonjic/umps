/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * uMPS - A general purpose computer system simulator
 *
 * Copyright (C) 2004 Renzo Davoli
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/****************************************************************************
 *
 * This module contains definitions for the vde_Network class.  
 * It is an implementation of the netinterface class
 *
 ****************************************************************************/

#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/un.h>

class netblockq; 

/****************************************************************************/
/* Inclusion of header files.                                               */
/****************************************************************************/

#define PROMISQ  0x4
#define INTERRUPT  0x2
#define NAMED  0x1

int testnetinterface(const char *name);

class netinterface
{
	public:
		netinterface(const char *name, const char *addr, int intnum);
	
		~netinterface(void);

		unsigned int readdata(char *buf, int len);
		unsigned int writedata(char *buf, int len);
		unsigned int polling();
		void setaddr(char *iethaddr);
		void getaddr(char *pethaddr);
		void setmode(int imode);
		unsigned int getmode();

	private:
		int fdctl,fddata;
		char ethaddr[6];
		char mode;
		struct sockaddr_un datasock;
		struct pollfd polldata;
		class netblockq *queue;
};


