// stats.h 
//	Routines for managing statistics about Nachos performance.
//
// DO NOT CHANGE -- these stats are maintained by the machine emulation.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "stats.h"

//----------------------------------------------------------------------
// Statistics::Statistics
// 	Initialize performance metrics to zero, at system startup.
//----------------------------------------------------------------------

Statistics::Statistics()
{
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;
    tlbHitCnt = tlbVisitCnt = 0;
}

//----------------------------------------------------------------------
// Statistics::Print
// 	Print performance metrics, when we've finished everything
//	at system shutdown.
//----------------------------------------------------------------------

void
Statistics::Print()
{
	  cout<< "TLB Hit: "<<tlbHitCnt<<", Total Visit: "<<tlbVisitCnt<<", TLB Hit Rate: "<<(double)(100 * 1.0 * tlbHitCnt / tlbVisitCnt)<<"\n";
    cout << "Ticks: total " << totalTicks << ", idle " << idleTicks;
		cout << ", system " << systemTicks << ", user " << userTicks <<"\n";
    cout << "Disk I/O: reads " << numDiskReads;
		cout << ", writes " << numDiskWrites << "\n";
		cout << "Console I/O: reads " << numConsoleCharsRead;
    cout << ", writes " << numConsoleCharsWritten << "\n";
    cout << "Paging: faults " << numPageFaults << "\n";
    cout << "Network I/O: packets received " << numPacketsRecvd;
		cout << ", sent " << numPacketsSent << "\n";
}
