/*
    KTop, the KDE Task Manager
   
	Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	$Id$
*/

#include <stdlib.h>
#include <stdio.h>

#include "Command.h"
#include "Memory.h"

#define MEMINFOBUFSIZE 1024
static char MemInfoBuf[MEMINFOBUFSIZE];
static int Dirty = 0;

static size_t Total = 0;
static size_t MFree = 0;
static size_t Appl = 0;
static size_t Used = 0;
static size_t Buffers = 0;
static size_t Cached = 0;
static size_t STotal = 0;
static size_t SFree = 0;
static size_t SUsed = 0;

static void
processMemInfo()
{
	sscanf(MemInfoBuf, "%*[^\n]\n"
		   "%*s %ld %ld %ld %*d %ld %ld\n"
		   "%*s %ld %ld %ld\n", 
		   &Total, &Used, &MFree, &Buffers, &Cached,
		   &STotal, &SUsed, &SFree);

	Appl = (Used - (Buffers + Cached)) / 1024;
	Total /= 1024;
	MFree /= 1024;
	Used /= 1024;
	Buffers /= 1024;
	Cached /= 1024;
	STotal /= 1024;
	SFree /= 1024;
	SUsed /= 1024;

	Dirty = 0;
}

/*
================================ public part =================================
*/

void
initMemory(void)
{
	FILE* meminfo;

	/* Make sure that /proc/meminfo exists and is readable. If not we do
	 * not register any monitors for memory. */
	if ((meminfo = fopen("/proc/meminfo", "r")) == NULL)
		return;
	fclose(meminfo);

	registerMonitor("mem/physical/free", "integer", printMFree,
					printMFreeInfo);
	registerMonitor("mem/physical/used", "integer", printUsed, printUsedInfo);
	registerMonitor("mem/physical/application", "integer", printAppl,
					printApplInfo);
	registerMonitor("mem/physical/buf", "integer", printBuffers,
					printBuffersInfo);
	registerMonitor("mem/physical/cached", "integer", printCached,
					printCachedInfo);
	registerMonitor("mem/swap/used", "integer", printSwapUsed,
					printSwapUsedInfo);
	registerMonitor("mem/swap/free", "integer", printSwapFree,
					printSwapFreeInfo);
}

void
exitMemory(void)
{
}

int
updateMemory(void)
{
	/*
	 * The amount of total and used memory is read from the /proc/meminfo.
	 * It also contains the information about the swap space.
	 * The 'file' looks like this:
	 *
	 *         total:    used:    free:  shared: buffers:  cached:
	 * Mem:  64593920 60219392  4374528 49426432  6213632 33689600
	 * Swap: 69636096   761856 68874240
	 * MemTotal:     63080 kB
	 * MeMFree:       4272 kB
	 * MemShared:    48268 kB
	 * Buffers:       6068 kB
	 * Cached:       32900 kB
	 * SwapTotal:    68004 kB
	 * SwapFree:     67260 kB
	 */

	FILE* meminfo;
	size_t n;

	if ((meminfo = fopen("/proc/meminfo", "r")) == NULL)
		return (-1);

	n = fread(MemInfoBuf, 1, MEMINFOBUFSIZE - 1, meminfo);
	fclose(meminfo);
	Dirty = 1;

	return (0);
}

void
printMFree(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("%ld\n", MFree);
}

void
printMFreeInfo(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("Free Memory\t0\t%ld\tKB\n", Total);
}

void
printUsed(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("%ld\n", Used);
}

void
printUsedInfo(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("Used Memory\t0\t%ld\tKB\n", Total);
}

void
printAppl(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("%ld\n", Appl);
}

void
printApplInfo(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("Application Memory\t0\t%ld\tKB\n", Total);
}

void
printBuffers(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("%ld\n", Buffers);
}

void
printBuffersInfo(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("Buffer Memory\t0\t%ld\tKB\n", Total);
}

void
printCached(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("%ld\n", Cached);
}

void
printCachedInfo(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("Cached Memory\t0\t%ld\tKB\n", Total);
}

void
printSwapUsed(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("%ld\n", SUsed);
}

void
printSwapUsedInfo(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("Used Swap Memory\t0\t%ld\tKB\n", STotal);
}

void
printSwapFree(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("%ld\n", SFree);
}

void
printSwapFreeInfo(const char* cmd)
{
	if (Dirty)
		processMemInfo();
	printf("Free Swap Memory\t0\t%ld\tKB\n", STotal);
}
