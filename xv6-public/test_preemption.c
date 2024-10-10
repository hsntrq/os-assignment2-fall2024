#include "types.h"
#include "user.h"

int
main(void)
{
  int pid_high, pid_low;
  struct proc_info info_high, info_low;

  // High priority process
  pid_high = fork();
  if(pid_high < 0){
    printf(1, "Fork failed\n");
    exit();
  }

  if(pid_high == 0){
    // Child process with lower nice value (higher priority)
    setnice(getpid(), 0);
    printf(1, "High Priority Process PID: %d\n", getpid());
    while(1){
      // Do some work
      asm volatile("nop");
    }
    exit();
  }

  // Low priority process
  pid_low = fork();
  if(pid_low < 0){
    printf(1, "Fork failed\n");
    exit();
  }

  if(pid_low == 0){
    // Child process with higher nice value (lower priority)
    setnice(getpid(), 30);
    printf(1, "Low Priority Process PID: %d\n", getpid());
    while(1){
      asm volatile("nop");
    }
    exit();
  }

  // Parent process
  printf(1, "Parent Process PID: %d\n", getpid());
  sleep(100);  // Let the children run
  getprocinfo(pid_high, &info_high);
  getprocinfo(pid_low, &info_low);

  printf(1, "Parent Process VRuntime: %d\n", info_high.vruntime);

  printf(1, "High Priority Process PID: %d\n", info_high.pid);
  printf(1, "High Priority Process VRuntime: %d\n", info_high.vruntime);
  printf(1, "High Priority Process Nice Value: %d\n", info_high.nice_value);
  printf(1, "High Priority Process Weight: %d\n", info_high.weight);
  printf(1, "High Priority Process Curr Runtime: %d\n", info_high.curr_runtime);
  
  printf(1, "Low Priority Process PID: %d\n", info_low.pid);
  printf(1, "Low Priority Process VRuntime: %d\n", info_low.vruntime);
  printf(1, "Low Priority Process Nice Value: %d\n", info_low.nice_value);
  printf(1, "Low Priority Process Weight: %d\n", info_low.weight);
  printf(1, "Low Priority Process Curr Runtime: %d\n", info_low.curr_runtime);

  if(info_high.vruntime < info_low.vruntime){
    printf(1, "Test Passed: Preemption logic working correctly\n");
  } else {
    printf(1, "Test Failed: Preemption logic not working\n");
  }

  kill(pid_high);
  kill(pid_low);
  wait();
  wait();

  exit();
}