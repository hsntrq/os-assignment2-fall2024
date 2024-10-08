## Getting Familiar with Scheduling in xv6

### 1. **Process Life Cycle in xv6**

In xv6, processes go through the following states in their life cycle:
1. **Creation**: A process is created with the `fork()` system call.
2. **Ready to Run (RUNNABLE)**: The process is initialized and set to **RUNNABLE** once it’s ready to be scheduled by the CPU.
3. **Running**: The process is selected by the scheduler and starts executing on the CPU.
4. **Sleeping**: A process can voluntarily block (e.g., waiting for I/O), entering the **SLEEPING** state.
5. **Terminated (ZOMBIE)**: The process finishes execution and awaits its parent to collect its exit status.

### 2. **Process Initialization and Scheduling**

When a process is created using the `fork()` system call, it follows these steps:
1. **Process Control Block (PCB)**: A new `struct proc` is created for the process, which holds essential information such as process ID, state, and memory.
2. **Memory Allocation**: The kernel allocates memory for the new process.
3. **Initial State**: The new process starts in the **EMBRYO** state and is moved to the **RUNNABLE** state once initialized.

```c
// Process transitions to RUNNABLE state in proc.c
p->state = EMBRYO;
p->state = RUNNABLE;  // After initialization
```

### 3. **The Scheduler**

The xv6 kernel uses a **Round-Robin Scheduler**. The scheduler continuously looks for processes in the **RUNNABLE** state and gives each one a chance to run for a fixed **time slice** (also called a quantum). After each time slice, the scheduler preempts the process and moves on to the next **RUNNABLE** process.

#### Round-Robin Scheduling:
- **Equal CPU Time**: In Round-Robin scheduling, all **RUNNABLE** processes get equal CPU time.
- **Process Queue**: The scheduler checks all processes in the **ptable** (process table) and picks one **RUNNABLE** process at a time.

```c
void scheduler(void) {
    struct proc *p;
    for(;;) {
        // Enable interrupts on this processor
        sti();

        // Loop over the process table to find a RUNNABLE process
        acquire(&ptable.lock);
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if(p->state != RUNNABLE)
                continue;

            // Switch to the selected process
            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;
            swtch(&c->scheduler, p->context);
            switchkvm();

            // The process is done running for now
            c->proc = 0;
        }
        release(&ptable.lock);
    }
}
```

### 4. **Giving Up the CPU**

There are **three cases** where a process gives up the CPU in xv6:
1. **Voluntary Yield** (`yield()`): A process can voluntarily call `yield()` to give up the CPU and return to the scheduler. The process is then set to the **RUNNABLE** state.
2. **Sleep** (`sleep()`): When a process needs to wait for an event (such as I/O), it calls `sleep()`. This transitions the process from **RUNNING** to **SLEEPING** and it no longer competes for CPU time. Once the event it was waiting for is complete, it is awakened (via `wakeup()`) and moved back to the **RUNNABLE** state.
3. **Preemption by Timer Interrupt**: The **timer interrupt** is triggered regularly (every tick). It occurs in `trap.c`, where a `yield()` is called after each timer tick, causing the current process to give up the CPU. The process is then put back into the **RUNNABLE** state. This ensures no process runs for too long and every **RUNNABLE** process gets CPU time.

#### **Trap Handling and Preemption in `trap.c`**
The `sti()` call in the scheduler re-enables interrupts, and a timer interrupt is raised periodically. When a timer interrupt occurs, xv6 calls the `trap()` function in `trap.c`:
```c
void trap(struct trapframe *tf) {
  // Force process to give up CPU on clock tick.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();
}
```
This `trap()` handler calls `yield()` for the running process after each tick, forcing it to give up the CPU. The process is then moved back to the **RUNNABLE** state, making it available for the next round of scheduling.

### 5. **Process State Transitions**

The following functions are critical in moving processes between different states in xv6:
- **`fork()`**: When a new process is created, it inherits the state and resources from its parent. The new process is initially marked as **RUNNABLE** after creation.
- **`yield()`**: A process can voluntarily call `yield()` to give up the CPU and is transitioned from **RUNNING** to **RUNNABLE**.
- **`sleep()`**: When a process needs to wait for a specific event (such as I/O), it calls `sleep()`, transitioning from **RUNNING** to **SLEEPING**.
- **`wakeup()`**: When the event a sleeping process is waiting for occurs, `wakeup()` is called, which transitions the process from **SLEEPING** to **RUNNABLE**.
- **`userinit()`**: Initializes the first process (the "init" process) in xv6, setting its state to **RUNNABLE** and starting the scheduling process.
- **`kill()`**: Marks the process as “killed” and, if the process is sleeping, transitions it from SLEEPING to RUNNABLE so it can return to user space and exit safely.
- **`exit()`**: Safely terminates a process by transitioning it to the **ZOMBIE** state after it finishes execution. The process remains in this state until its parent collects its exit status.

### 6. **Termination of a Process**

When a process finishes execution or is killed, it enters the **ZOMBIE** state:
1. **Exit**: The `exit()` system call terminates the process, which moves it to the **ZOMBIE** state.
2. **Wait**: The parent process collects the exit status of the terminated process using the `wait()` system call, which frees the process's resources and removes it from the system.

```c
void exit(void) {
    struct proc *curproc = myproc();
    // Mark the process as ZOMBIE
    curproc->state = ZOMBIE;
    sched();  // Switch to another process
    panic("zombie exit");
}
```


## Implementing Completely Fair Scheduler (CFS) in xv6

### Why Use a Red-Black Tree?

A **Red-Black Tree (RBT)** is a self-balancing binary search tree that ensures efficient operations, such as insertions, deletions, and lookups. In CFS, we need to retrieve the process with the smallest virtual runtime (vRuntime) at each scheduling point. Using a plain binary search tree (BST) can lead to performance degradation if the tree becomes unbalanced, making lookups inefficient. The Red-Black Tree solves this problem by maintaining balance with every insertion and deletion, ensuring logarithmic time complexity for these operations.

For an introduction to Red-Black Trees, refer to:
- [GeeksforGeeks | Introduction to Red-Black Tree](https://www.geeksforgeeks.org/introduction-to-red-black-tree/)
- [Medium | Deletion in Red-Black Tree](https://medium.com/analytics-vidhya/deletion-in-red-black-rb-tree-92301e1474ea)

### Core Functions for CFS

To implement CFS in xv6, you will need to create the following functions:

#### 1. **Red-Black Tree Functions**
   - **Insert Node**: Insert a process into the Red-Black Tree based on its vRuntime. This will require maintaining the Red-Black Tree properties.
   - **Retrieve Minimum Node**: Find and retrieve the node with the smallest vRuntime (i.e., the next process to run).
   - **Fix Insert**: After inserting a node into the Red-Black Tree, ensure the tree remains balanced by performing rotations and recoloring.
   - **Fix Delete**: After deleting a node, rebalance the tree using the Red-Black Tree rules.
   - **Add to Tree**: Add process to the tree, rebalance the tree, and update tree properties (total_weight, length, min_vruntime, etc).
   - **Next Process**: Delete minimum vruntime node, rebalance the tree, update tree properties (total_weight, period, length, min_vruntime, etc), and update process time slice.
   
   These operations ensure the scheduler always efficiently retrieves the process with the minimum vRuntime.

#### 2. **Calculating Process Weights and Execution Times**

CFS uses several formulas to determine how much CPU time a process gets. The key concepts are **weight**, **period**, **maximum execution time (time slice)**, and **granularity**. Below are the formulas you’ll need:

1. **Weight Calculation**:
   The process weight is calculated using its nice value, following the formula:
   ```math
   weight = \frac{1024}{1.25^{nice}}
   ```
   Here, the nice value typically ranges from `-20` to `19` (with 0 being default), but in xv6, you'll use a simplified range `0` to `30`.

2. **Maximum Execution Time (Time Slice)**:
   The maximum time slice for a process is determined by the following formula:
   ```math
   max\_exec\_time = \frac{period \times process\_weight}{total\_weight}
   ```
   Where:
   - `period` is the length of a scheduling cycle.
   - `process_weight` is the weight of the process.
   - `total_weight` is the sum of the weights of all **RUNNABLE** processes.

3. **Period Calculation**:
   The period is dynamically adjusted based on the number of runnable processes:
   ```math
   period = min\_granularity \times number\_of\_processes
   ```
   The minimum granularity (`min_granularity`) ensures that processes with very high priority (low vRuntime) are not preempted too frequently.

For more details on the formulas, refer to [this article](http://fizyka.umk.pl/~jkob/prace-mag/cfs).

#### 3. **Tracking vRuntime and Preemption**

1. **vRuntime Update**:
   The **virtual runtime** is updated each time a process is scheduled.
   
   - Each time the timer interrupt is triggered, the `curr_runtime` of the process has to be incremented by 1 tick in [trap.c](./xv6-public/trap.c) before handing control to `yield()`

   - When a process is added to the scheduler again, its `vRuntime` needs to be incremented with `curr_runtime` after which `curr_runtime` needs to be reset.

2. **Checking for Preemption**:
   Preemption should occur when:
	- The process has run for at least `min_granularity` (minimum time slice) to avoid frequent preemptions.
    - `curr_runtime` (time process has been running) exceeds `max_exec_time` (its time slice) or,
	- The current process’s vruntime (virtual runtime) is greater than another RUNNABLE process’s vruntime.

### 4. **Key Operations to Manage Process States**

To implement the scheduling logic, you will need to manage when processes transition between **RUNNABLE** and **RUNNNING** states:
- When a process transitions into the **RUNNABLE** state it needs to be added to the red black tree.
- The scheduler should loop through the tree until it is empty. In each iteration, it should retrieves the next process from the tree. If the next process is **RUNNABLE**, you need to change the state to **RUNNING** and switch to the process.

### Reference for CFS Implementation

- For a deeper understanding and to draw inspiration, you can check out the [**Linux CFS Implementation**](https://github.com/torvalds/linux/blob/master/kernel/sched/fair.c). This is a more complex version of what you will be building, but it’s an excellent resource for ideas and deeper exploration.

