#include "types.h"
#include "user.h"

int
main(void)
{
  int count, total_weight, period;
  int expected_period;
  int i;

  // Create several processes to increase the tree count
  for(i = 0; i < 5; i++){
    if(fork() == 0){
      sleep(50);  // Keep the child alive
      exit();
    }
  }

  sleep(10);  // Allow processes to be inserted

  gettreeinfo(&count, &total_weight, &period);
  expected_period = (count > (32 / 2)) ? count * 2 : 32;

  printf(1, "Tree Info - Count: %d, Total Weight: %d, Period: %d\n", count, total_weight, period);
  printf(1, "Expected Period: %d\n", expected_period);

  if(period == expected_period){
    printf(1, "Test Passed: Period calculated correctly\n");
  } else {
    printf(1, "Test Failed: Period calculation incorrect\n");
  }

  // Clean up child processes
  for(i = 0; i < 5; i++)
    wait();

  exit();
}