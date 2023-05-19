/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"
#include <unistd.h>
#include <sys/wait.h>

void SysHalt()
{
  kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

int SysSub(int op1, int op2)
{
  return op1 - op2;
}

int SysMul(int op1, int op2)
{
  return op1 * op2;
}

int SysPow(int op1, int op2)
{
  int i, result = 1;
  for (i = 0; i < op2; i++)
  {
    result = result * op1;
  }
  return result;
}

int SysDiv(int op1, int op2)
{
  return op1 / op2;
}

int SysWrite(int Addr, int Count, int FileID)
{
  int ch;
  int i = 0;
  while (i < Count)
  {
    kernel->machine->ReadMem(Addr, 1, &ch);
    write(FileID, (char *)&ch, 1);
    Addr++;
    i++;
  }
  return i;
}

int SysRead(int Addr, int Count, int FileID)
{
  int ch;
  int i = 0;
  while (i < Count)
  {
    read(FileID, &ch, 1);
    kernel->machine->WriteMem(Addr, 1, ch);
    Addr++;
    i++;
    write(FileID, (char *)&ch, 1);
  }
  return i;
}

int SysExec(int Addr)
{
  int count = 0;
  int ch;
  char cmd[60];
  do
  {
    kernel->machine->ReadMem(Addr, 1, &ch);
    Addr++;
    cmd[count] = (char)ch;
  } while (ch != '\0' && count++ < 59);
  cmd[count] = '\0';
  pid_t child;
  child = vfork();
  if (child == 0)
  {
    execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
    _exit(EXIT_FAILURE);
  }
  else if (child < 0)
  {
    _exit(EXIT_FAILURE);
    return EPERM;
  }
  return child;
}

int SysJoin(int procid)
{
  return waitpid((pid_t)procid, (int *)0, 0);
}
#endif /* ! __USERPROG_KSYSCALL_H__ */
