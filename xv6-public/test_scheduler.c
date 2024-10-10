#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_PROCS 7

int
main(void)
{
  int pids[NUM_PROCS];
  int nice_values[NUM_PROCS] = {0, 5, 10, 15, 20, 25, 30};
  struct proc_info info[NUM_PROCS];
  int count, total_weight, period;
  int i, j;
  int start_time, end_time;

  printf(1, "Starting CFS scheduler test\n");

  for(i = 0; i < NUM_PROCS; i++){
    pids[i] = fork();
    if(pids[i] < 0){
      printf(1, "Fork failed\n");
      exit();
    }
    if(pids[i] == 0){
      printf(1, "Started process %d\n", getpid());
      // Child process
      setnice(getpid(), nice_values[i]);

      // Simulate workload
      start_time = uptime();

      for(j = 0; j < 200000000; j++){
        asm volatile("nop");
      }
      end_time = uptime();

      printf(1, "Process %d with nice value %d ran for %d ticks\n",
             getpid(), nice_values[i], end_time - start_time);
      getprocinfo(getpid(), &info[i]);
      printf(1, "Process %d Info - Nice Value: %d, Weight: %d, VRuntime: %d, CurrRuntime: %d\n\n",
             info[i].pid, info[i].nice_value, info[i].weight, info[i].vruntime, info[i].curr_runtime);
      exit();
    }
  }

  // Parent waits for all children to finish
  
  sleep(10);
  gettreeinfo(&count, &total_weight, &period);
  printf(1, "Tree Info - Count: %d, Total Weight: %d, Period: %d\n\n", count, total_weight, period);

  for(i = 0; i < NUM_PROCS; i++){
    wait();
  }

  printf(1, "CFS scheduler test completed\n");
  exit();
}