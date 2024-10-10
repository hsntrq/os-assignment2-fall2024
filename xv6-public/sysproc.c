#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_setnice(void)
{
  int pid, nice_value;

  if(argint(0, &pid) < 0 || argint(1, &nice_value) < 0)
    return -1;
  return setnice(pid, nice_value);
}

int
sys_gettreeinfo(void)
{
  int count;
  int total_weight;
  int period;
  int user_count, user_total_weight, user_period;

  if(argptr(0, (void*)&user_count, sizeof(int)) < 0)
    return -1;
  if(argptr(1, (void*)&user_total_weight, sizeof(int)) < 0)
    return -1;
  if(argptr(2, (void*)&user_period, sizeof(int)) < 0)
    return -1;

  gettreeinfo(&count, &total_weight, &period);

  if(copyout(myproc()->pgdir, user_count, (char*)&count, sizeof(int)) < 0)
    return -1;
  if(copyout(myproc()->pgdir, user_total_weight, (char*)&total_weight, sizeof(int)) < 0)
    return -1;
  if(copyout(myproc()->pgdir, user_period, (char*)&period, sizeof(int)) < 0)
    return -1;

  return 0;
}

int
sys_getprocinfo(void)
{
  int pid;
  struct proc_info *user_pinfo;
  struct proc_info pinfo;

  if(argint(0, &pid) < 0)
    return -1;
  if(argptr(1, (char**)&user_pinfo, sizeof(struct proc_info)) < 0)
    return -1;

  getprocinfo(pid, &pinfo);

  if (pinfo.pid == -1)
    return -1;

  if(copyout(myproc()->pgdir, (uint)user_pinfo, (void*)&pinfo, sizeof(struct proc_info)) < 0)
    return -1;
  return 0;
}

int
sys_gettreenodes(void)
{
  struct rb_node_info **user_nodes;
  struct rb_node_info *nodes;
  int max_nodes;
  int node_index;

  if(argint(0, &max_nodes) < 0)
    return -1;
  if(argptr(1, (char**)&user_nodes, sizeof(struct rb_node_info) * max_nodes) < 0)
    return -1;

  node_index = gettreenodes(max_nodes, nodes);

  if(copyout(myproc()->pgdir, (uint)&user_nodes, (void*)&nodes, sizeof(struct rb_node_info) * node_index) < 0)
    return -1;

  return node_index;
}
