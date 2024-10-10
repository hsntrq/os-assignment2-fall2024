#include "types.h"
#include "user.h"

int
main(void)
{
  int count, total_weight, period;

  if(gettreeinfo(&count, &total_weight, &period) < 0){
    printf(1, "Error: gettreeinfo failed\n");
    exit();
  }

  printf(1, "Tree Info - Count: %d, Total Weight: %d, Period: %d\n", count, total_weight, period);

  if(count == 1){
    printf(1, "Test Passed: Tree initialized properly with the init process\n");
  } else {
    printf(1, "Test Failed: Tree count expected to be 1, got %d\n", count);
  }

  exit();
}
