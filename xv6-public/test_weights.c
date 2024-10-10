#include "types.h"
#include "user.h"

int
main(void)
{
  int pid;
  struct proc_info info;

  pid = fork();
  if(pid < 0){
    printf(1, "Fork failed\n");
    exit();
  }

  if(pid == 0){
    // Child process
    sleep(50);  // Sleep to allow parent to get info
    exit();
  } else {
    // Parent process
    sleep(10);  // Ensure child has been allocated
    if(getprocinfo(pid, &info) < 0){
      printf(1, "Error: getprocinfo failed\n");
      exit();
    }

    printf(1, "Process %d Info - Nice Value: %d, Weight: %d, VRuntime: %d, CurrRuntime: %d\n",
           info.pid, info.nice_value, info.weight, info.vruntime, info.curr_runtime);

    // Expected default nice value is 0, weight should be computed accordingly
    int expected_weight = 1024;
    if(info.weight == expected_weight){
      printf(1, "Test Passed: Weight computed correctly\n");
    } else {
      printf(1, "Test Failed: Expected weight %d, got %d\n", expected_weight, info.weight);
    }

    wait();  // Wait for child
  }

  exit();
}