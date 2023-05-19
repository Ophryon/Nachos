// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------
int pointer = 0;
void SimpleTLBMissHandler(int virtAddr)
{
	unsigned int vpn;
	vpn = (unsigned)virtAddr / PageSize;
	// 这里假设tlb仅有两个位置，用于测试tlb功能
	DEBUG('a', ">>>>>>>>add page to tlb>>>>>>>>>>>>");
	kernel->machine->tlb[pointer] = kernel->machine->pageTable[vpn];
	pointer = pointer ? 0 : 1;
}

void TLBMissHandler(int virtAddr)
{

	unsigned int vpn = (unsigned)virtAddr / PageSize;
	unsigned int tlbExchangeIndex = -1;

	// 如果TLB为空，直接插入
	for (int i = 0; i < TLBSize; ++i)
	{
		if (!kernel->machine->tlb[i].valid)
		{
			tlbExchangeIndex = i;
			break;
		}
	}
	// tlb满,使用fifo、lru、NRU置换算法
	if (tlbExchangeIndex == -1)
	{
		DEBUG('a', "tlb get max size.\n");

#ifdef TLB_FIFO
		tlbExchangeIndex = TLBSize - 1;
		// 将后n-1个元素向前挪动一个单位，并将新元素插入队尾
		for (int i = 0; i < TLBSize - 1; ++i)
		{
			kernel->machine->tlb[i] = kernel->machine->tlb[i + 1];
		}
#endif

#ifdef TLB_LRU
		unsigned int min = __INT_MAX__;
		// 找lastVisitedTime最小的元素
		for (int i = 0; i < TLBSize; ++i)
		{
			if (kernel->machine->tlb[i].lastVisitedTime < min)
			{
				min = kernel->machine->tlb[i].lastVisitedTime;
				tlbExchangeIndex = i;
			}
		}
#endif

#ifdef TLB_NRU
		while (tlbExchangeIndex == -1)
		{
			// 选择遇到的第一个(A=0, M=0)页，则进行置换
			for (int i = 0; i < TLBSize; ++i)
			{
				if (!kernel->machine->tlb[i].use && !kernel->machine->tlb[i].dirty)
				{
					tlbExchangeIndex = i;
					break;
				}
			}
			// 如果第1步失败，则重新扫描，查找(A=0, M=1)页,
			// 选择遇到的第一个这样的页则置换。在这个扫描过程中，对每个跳过的页，把它的使用位设置成0
			// 重复这个过程直到找到可以替换的页面
			if (tlbExchangeIndex == -1)
			{
				for (int i = 0; i < TLBSize; ++i)
				{
					if (!kernel->machine->tlb[i].use && kernel->machine->tlb[i].dirty)
					{
						tlbExchangeIndex = i;
						break;
					}
					else
					{
						kernel->machine->tlb[i].use = false;
					}
				}
			}
		}
#endif
	}
	cout << "--------------------缺页中断处理开始--------------------\n\n";
	cout << "TLB before replacement:\n";
	for (int i = 0; i < TLBSize; ++i)
	{
		cout << "tlb[" << i << "] valid :" << kernel->machine->tlb[i].valid << " lastVisitedTime: " << kernel->machine->tlb[i].lastVisitedTime << " use: " << kernel->machine->tlb[i].use << " dirty: " << kernel->machine->tlb[i].dirty << " virtualPage: " << kernel->machine->tlb[i].virtualPage << " physicalPage: " << kernel->machine->tlb[i].physicalPage << "\n\n";
	}
	cout << "Replacement: "
		 << "tlb[" << tlbExchangeIndex << "] has been exchanged by PageTable[" << vpn << "]\n";
	kernel->machine->tlb[tlbExchangeIndex] = kernel->machine->pageTable[vpn]; // 将页表中的页面加载到tlb中
	cout << "PageTable[" << vpn << "]Info: use: " << kernel->machine->pageTable[vpn].use << " dirty: " << kernel->machine->pageTable[vpn].dirty << " virtualPage: " << kernel->machine->pageTable[vpn].virtualPage << " physicalPage: " << kernel->machine->pageTable[vpn].physicalPage << "\n\n"
		 << endl;
#ifdef TLB_NRU
	// 随机修改dirty位，模拟写入修改，测试nru算法
	int tf = rand() % 2;
	if (tf == 1)
	{
		kernel->machine->tlb[tlbExchangeIndex].dirty = TRUE;
	}
	else
	{
		kernel->machine->tlb[tlbExchangeIndex].dirty = FALSE;
	}
#endif
	cout << "TLB after replacement:\n";
	for (int i = 0; i < TLBSize; ++i)
	{
		cout << "tlb[" << i << "] valid :" << kernel->machine->tlb[i].valid << " lastVisitedTime: " << kernel->machine->tlb[i].lastVisitedTime << " use: " << kernel->machine->tlb[i].use << " dirty: " << kernel->machine->tlb[i].dirty << " virtualPage: " << kernel->machine->tlb[i].virtualPage << " physicalPage: " << kernel->machine->tlb[i].physicalPage << "\n\n";
	}
	cout << "--------------------缺页中断处理结束--------------------\n\n";
}

void ExceptionHandler(ExceptionType which)
{

	int type = kernel->machine->ReadRegister(2);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");
	switch (which)
	{

	case PageFaultException:
#ifdef USE_TLB
		// SimpleTLBMissHandler(kernel->machine->ReadRegister(BadVAddrReg));
		TLBMissHandler(kernel->machine->ReadRegister(BadVAddrReg));
#endif
		break;

	case SyscallException:
		switch (type)
		{

		case SC_Halt:
			DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

			SysHalt();

			ASSERTNOTREACHED();
			break;
		case SC_Write:
			DEBUG(dbgSys, "Write " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysWrite Systemcall*/
			int writeResult;
			writeResult = SysWrite(/* int op1 */ (int)kernel->machine->ReadRegister(4),
								   /* int op2 */ (int)kernel->machine->ReadRegister(5),
								   /* int op3 */ (int)kernel->machine->ReadRegister(6));

			DEBUG(dbgSys, "Add returning with " << writeResult << "\n");

			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)writeResult);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;
		case SC_Read:
			DEBUG(dbgSys, "Read " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysRead Systemcall*/
			int readResult;
			readResult = SysRead(/* int op1 */ (int)kernel->machine->ReadRegister(4),
								 /* int op2 */ (int)kernel->machine->ReadRegister(5),
								 /* int op3 */ (int)kernel->machine->ReadRegister(6));

			DEBUG(dbgSys, "read returning with " << readResult << "\n");

			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)readResult);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;
		case SC_Exec:
			DEBUG(dbgSys, "Exec " << kernel->machine->ReadRegister(4) << "\n");

			/* Process SysExec Systemcall*/
			int execResult;
			execResult = SysExec(/* int op1 */ (int)kernel->machine->ReadRegister(4));

			DEBUG(dbgSys, "Exec returning with " << execResult << "\n");

			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)execResult);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;
		case SC_Join:
			DEBUG(dbgSys, "Join " << kernel->machine->ReadRegister(4) << "\n");

			/* Process SysJoin Systemcall*/
			int joinResult;
			joinResult = SysJoin(/* int op1 */ (int)kernel->machine->ReadRegister(4));

			DEBUG(dbgSys, "Join returning with " << joinResult << "\n");

			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)joinResult);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;
		case SC_Add:
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			int addResult;
			addResult = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							   /* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << addResult << "\n");

			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)addResult);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;

		case SC_Mul:
			DEBUG(dbgSys, "Mul " << kernel->machine->ReadRegister(4) << " * " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysMul Systemcall*/
			int mulResult;
			mulResult = SysMul(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							   /* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Mul returning with " << mulResult << "\n");

			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)mulResult);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;

		case SC_Pow:
			DEBUG(dbgSys, "Pow " << kernel->machine->ReadRegister(4) << " ^ " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysPow Systemcall*/
			int powResult;
			powResult = SysPow(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							   /* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Pow returning with " << powResult << "\n");

			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)powResult);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;

		case SC_Div:
			DEBUG(dbgSys, "Div " << kernel->machine->ReadRegister(4) << " / " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysDiv Systemcall*/
			int divResult;
			divResult = SysDiv(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							   /* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Div returning with " << divResult << "\n");

			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)divResult);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;

		case SC_Sub:
			DEBUG(dbgSys, "Sub " << kernel->machine->ReadRegister(4) << " - " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysSub Systemcall*/
			int subResult;
			subResult = SysSub(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							   /* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Sub returning with " << subResult << "\n");

			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)subResult);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;

		default:
			cerr << "Unexpected system call " << type << "\n";
			// ASSERTNOTREACHED();
			break;
		}
		break;
	default:
		cerr << "Unexpected user mode exception" << (int)which << "\n";
		break;
	}
	// ASSERTNOTREACHED();
}
