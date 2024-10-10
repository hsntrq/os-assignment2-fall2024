#include "types.h"
#include "user.h"

int
main(void)
{
  int initial_count, final_count;
  int pid;

  gettreeinfo(&initial_count, 0, 0);
  printf(1, "Initial Tree Count: %d\n", initial_count);

  pid = fork();
  if(pid < 0){
    printf(1, "Fork failed\n");
    exit();
  }

  if(pid == 0){
    // Child process
    exit();
  } else {
    // Parent process
    wait();
    gettreeinfo(&final_count, 0, 0);
    printf(1, "Final Tree Count after fork and wait: %d\n", final_count);

    if(final_count == initial_count){
      printf(1, "Test Passed: Process inserted and removed from tree correctly\n");
    } else {
      printf(1, "Test Failed: Expected tree count %d, got %d\n", initial_count, final_count);
    }
  }

  exit();
}