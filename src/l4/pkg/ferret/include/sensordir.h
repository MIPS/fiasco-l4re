/*
 * (c) 2009 Technische Universität Dresden
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#pragma once

enum Protocols
{
	Client  = 0x10,
	Monitor = 0x20
};


enum ClientOperations
{
	Create = 0x10,
	Free   = 0x20,
	NewInstance = 0x30,
};


enum MonitorOperations
{
	Attach = 0x10,
	Detach = 0x20,
	List   = 0x30,
};
