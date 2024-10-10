#include "types.h"
#include "user.h"

#define MAX_NODES 64 // NPROC

int
main(void)
{
  struct rb_node_info nodes[MAX_NODES];
  int num_nodes, i;

  // Create several processes
  for(i = 0; i < 10; i++){
    if(fork() == 0){
      sleep(50);  // Keep the child alive
      exit();
    }
  }

  sleep(10);  // Allow processes to be inserted

  num_nodes = gettreenodes(MAX_NODES, nodes);

  printf(1, "Number of Nodes in RB Tree: %d\n", num_nodes);

  // Verify BST property: In-order traversal should yield sorted virtual runtimes
  int sorted = 1;
  for(i = 1; i < num_nodes; i++){
    printf(1, "Node %d: PID %d, VRuntime %d\n", i, nodes[i].pid, nodes[i].vruntime);
    if(nodes[i-1].vruntime > nodes[i].vruntime){
      sorted = 0;
      break;
    }
  }

  if(sorted){
    printf(1, "Test Passed: Red-Black Tree maintains BST property\n");
  } else {
    printf(1, "Test Failed: Red-Black Tree does not maintain BST property\n");
  }

  // Additional checks can be added to verify red-black properties

  // Clean up child processes
  for(i = 0; i < 5; i++)
    wait();

  exit();
}