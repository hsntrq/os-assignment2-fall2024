#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_PROCS 7

int
main(void)
{
  int pids[NUM_PROCS];
  int nice_values[NUM_PROCS] = {0, 5, 10, 15, 20, 25, 30};
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
      exit();
    }
  }

  // Parent waits for all children to finish
  for(i = 0; i < NUM_PROCS; i++){
    wait();
  }

  printf(1, "CFS scheduler test completed\n");
  exit();
}