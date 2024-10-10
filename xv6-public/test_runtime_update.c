// In user/test_runtime_update.c
#include "types.h"
#include "user.h"

int
main(void)
{
  int pid;
  struct proc_info info_before, info_after;

  pid = fork();
  if(pid < 0){
    printf(1, "Fork failed\n");
    exit();
  }

  if(pid == 0){
    // Child process
    sleep(50);  // Simulate sleeping
    exit();
  } else {
    // Parent process
    sleep(10);  // Ensure child is running
    if(getprocinfo(pid, &info_before) < 0){
      printf(1, "Error: getprocinfo failed\n");
      exit();
    }

    kill(pid);  // Kill the child process

    if(getprocinfo(pid, &info_after) < 0){
      printf(1, "Error: getprocinfo failed\n");
      exit();
    }

    printf(1, "Before Kill - VRuntime: %d, CurrRuntime: %d\n", info_before.vruntime, info_before.curr_runtime);
    printf(1, "After Kill - VRuntime: %d, CurrRuntime: %d\n", info_after.vruntime, info_after.curr_runtime);

    if(info_after.vruntime == info_before.vruntime + info_before.curr_runtime){
      printf(1, "Test Passed: VRuntime updated correctly after kill\n");
    } else {
      printf(1, "Test Failed: VRuntime not updated correctly\n");
    }

    wait();
  }

  exit();
}