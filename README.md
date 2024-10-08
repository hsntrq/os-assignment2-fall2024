# An xv6 Complete Fair Scheduler (CFS)

In this assignment, you will implement a **Completely Fair Scheduler (CFS)** into the xv6 kernel. This assignment is designed to introduce you to key concepts of kernel development, process scheduling, and system calls. The CFS is a widely used scheduling algorithm in modern operating systems, which ensures that CPU time is fairly distributed among processes. By completing this assignment, you will gain hands-on experience in modifying a real kernel, writing system calls, and evaluating the performance of a new scheduler.

### Objectives
- Develop a deep understanding of xv6 and kernel internals.
- Learn the basics of scheduling algorithms, focusing on CFS.
- Modify the existing scheduler in xv6 and implement a more complex scheduler (CFS).
- Implement and test system calls within the kernel.
- Evaluate the performance of your scheduler and compare it to the default scheduler.

### Key Deliverables
- **Scheduler Implementation**: You will implement a CFS scheduler in xv6.
- **System Call Implementation**: You will implement the `setnice()` system call, which adjusts the priority of processes.
- **Performance Comparison**: You will run tests to evaluate the performance of your CFS implementation against the default Round-Robin scheduler.

### Getting Started
Please refer to the **[SETUP.md](./SETUP.md)** file for instructions on setting up xv6 on your local machine. You must have a working setup of xv6 before starting the assignment.

## The Completely Fair Scheduler (CFS)

The Completely Fair Scheduler is designed to divide CPU time fairly among all runnable processes. Instead of assigning fixed time slices as in Round-Robin scheduling, CFS keeps track of how much time each process has consumed and prioritizes those that have consumed less time. In practice, this results in a more balanced and "fair" distribution of CPU time, especially when there are processes of varying priorities (nice values).

### Details


1. **Modify the Scheduler**:
   - You will replace the default **Round-Robin** scheduler in xv6 with a simplified version of the **Completely Fair Scheduler (CFS)**.
   - The CFS will maintain a red-black tree where processes are ordered based on their **virtual runtime** (vRuntime), which tracks the amount of time a process has been allowed to execute.
   - Processes that have used less CPU time (lower vRuntime) are given higher priority.

2. **System Call: `setnice(int nice_value)`**:
   - This system call allows a process to adjust its **nice value**, which directly affects its weight and scheduling priority.
   - By default, all processes have a nice value of 0. Lowering the nice value (more negative) increases the process's priority, while raising it (more positive) decreases it.
   - You will implement the `setnice()` system call, which should:
     - Set the nice value for the calling process.
     - Adjust the process's weight based on the nice value (use a simplified version of Linux’s formula for weights: `weight = 1024 / (1.25 ^ nice_value)`).
     - Ensure proper integration with the CFS scheduler.
   - The `setnice()` system call signature:
     ```c
     int setnice(int nice_value);
     ```

3. **Fair Scheduling**:
   - When a new process is created, it should inherit the nice value and weight from its parent.
   - Each process’s time slice is determined by the ratio of its weight to the total weight of all runnable processes.


### Comparing with the Round-Robin Scheduler

1. **Run your test programs using the default Round-Robin scheduler**.
   - Compile xv6 without your CFS implementation to observe the behavior of the Round-Robin scheduler.
   - Record the CPU time distribution across processes.

2. **Run the same test program with your CFS implementation**.
   - Compile xv6 with your CFS scheduler.
   - Record the CPU time distribution again.

3. **Compare the results**:
   - Report your findings using a graph and compare the performance of both schedulers.

---
You can follow **[GUIDE.md](./GUIDE.md)** to get a head start on scheduling in xv6 and implementation details for CFS.
