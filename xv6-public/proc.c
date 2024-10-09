#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

//Red-Black Tree data structure
struct rbtree {
  int length;
  int period;
  int total_weight;
  struct proc *root;
  struct proc *min_vruntime;
  struct spinlock lock;
}rbTree; 

static struct proc *initproc;

static struct rbtree *runnableTasks = &rbTree;

//Set target scheduler latency and minimum granularity constants
//Latency must be multiples of min_granularity
static int latency = NPROC / 2;
static int min_granularity = 2;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}


// compute_weight(int)
// The formula to determine weight of process is:
// 1024/(1.25 ^ nice value of process)
int
compute_weight(int nice_value){
  if(nice_value > 30){
	nice_value = 30;
  }
  
  double base = 1.25;
  double denominator = 1.0;
  for (int i = 0; i < nice_value; i++){
    denominator *= base;
  }
  return (int) (1024/denominator);
}

int
setnice(int pid, int nice_value)
{
  struct proc *p;
  // printf("Setting nice value for process %d to %d\n", pid, nice_value);

  if(nice_value < 0 || nice_value > 30)
    return -1;  // Invalid nice value

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      // printf("Process found\n");
      p->nice_value = nice_value;

      // Recalculate the weight and max execution time
      p->weight = compute_weight(p->nice_value);
      p->max_exec_time = (runnableTasks->period * p->weight) / runnableTasks->total_weight;

      release(&ptable.lock);
      return 0;  // Success
    }
  }
  release(&ptable.lock);
  return -1;  // Process not found
}

void
treeinit(struct rbtree *tree, char *lockName)
{
  initlock(&tree->lock, lockName);
  tree->length = 0;
  tree->root = 0;
  tree->total_weight = 0;
  tree->min_vruntime = 0;

  //Initially set time slice factor for all processes
  tree->period = latency;
}

int
empty(struct rbtree *tree)
{
  return tree->length == 0;
}

int
full(struct rbtree *tree)
{
  return tree->length == NPROC;
}

//This two process retrieval functions will retrive the grandparent or uncle process of the process passed into the functions. This is done to preserve red black tree properties by altering states and positions of the tree.
struct proc*
retrieveGrandparentproc(struct proc* process){
  if(process != 0 && process->p != 0){
	return process->p->p;
  } 
	
  return 0;
}

struct proc*
retrieveUncleproc(struct proc* process){
  struct proc* grandParent = retrieveGrandparentproc(process);
  if(grandParent != 0){
	if(process->p == grandParent->l){
		return grandParent->r;
	} else {
		return grandParent->l;
	}
  }
	
  return 0;
}

void 
leftrotate(struct rbtree* tree, struct proc* positionProc){
  struct proc* save_right_Proc = positionProc->r;
	
  positionProc->r = save_right_Proc->l;
  if(save_right_Proc->l != 0)
	save_right_Proc->l->p = positionProc;
  save_right_Proc->p = positionProc->p;
	
  if(positionProc->p == 0){
	tree->root = save_right_Proc;
  } else if(positionProc == positionProc->p->l){
	positionProc->p->l = save_right_Proc;
  } else {
	positionProc->p->r = save_right_Proc;
  }
  save_right_Proc->l = positionProc;
  positionProc->p = save_right_Proc;
}

void 
rightrotate(struct rbtree* tree, struct proc* positionProc){
	
  struct proc* save_left_Proc = positionProc->l;
	
  positionProc->l = save_left_Proc->r;
	
  //Determine parents for the process being rotated
  if(save_left_Proc->r != 0)
	save_left_Proc->r->p = positionProc;
  save_left_Proc->p = positionProc->p;
  if(positionProc->p == 0){
	tree->root = save_left_Proc;
  } else if(positionProc == positionProc->p->r){
	positionProc->p->r = save_left_Proc;
  } else {
	positionProc->p->l = save_left_Proc;
  }
  save_left_Proc->r = positionProc;
  positionProc->p = save_left_Proc;
	
}

struct proc*
minproc(struct proc* traversingProcess){
	
  if(traversingProcess != 0){
	if(traversingProcess->l != 0){
	    return minproc(traversingProcess->l);
	} else {
	    return traversingProcess;
	}
  }
	return 0;
}

struct proc*
insertproc(struct proc* traversingProcess, struct proc* insertingProcess){
	
  insertingProcess->color = RED;
	
  //i.e it is root or at leaf of tree
  if(traversingProcess == 0){
	return insertingProcess;
  }		
  //i.e everything after root
  //move process to the right of the current subtree
  if(traversingProcess->vruntime <= insertingProcess->vruntime){
	insertingProcess->p = traversingProcess;
	traversingProcess->r = insertproc(traversingProcess->r, insertingProcess);
  } else {
	insertingProcess->p = traversingProcess;		
	traversingProcess->l = insertproc(traversingProcess->l, insertingProcess);
  }
	
  return traversingProcess;
}

void
fixinsert(struct rbtree* tree, struct proc* rbProcess, int cases){
	
  struct proc* uncle;
  struct proc* grandparent;
	
  switch(cases){
  case 1:
	if(rbProcess->p == 0)
		rbProcess->color = BLACK;
	else
		fixinsert(tree, rbProcess, 2);
	break;
	
  case 2:
	if(rbProcess->p->color == RED)
		fixinsert(tree, rbProcess, 3);
	break;
	
  case 3:
	uncle = retrieveUncleproc(rbProcess);
	
	if(uncle != 0 && uncle->color == RED){
		rbProcess->p->color = BLACK;
		uncle->color = BLACK;
		grandparent = retrieveGrandparentproc(rbProcess);
		grandparent->color = RED;
		fixinsert(tree, grandparent, 1);
		grandparent = 0;
	} else {
		fixinsert(tree, rbProcess,4);
	}
	
	uncle = 0;
	break;
  
  case 4:
	grandparent = retrieveGrandparentproc(rbProcess);
	
	if(rbProcess == rbProcess->p->r && rbProcess->p == grandparent->l){
		leftrotate(tree, rbProcess->p);
		rbProcess = rbProcess->l;
	} else if(rbProcess == rbProcess->p->l && rbProcess->p == grandparent->r){
		rightrotate(tree, rbProcess->p);
		rbProcess = rbProcess->r;
	}
	fixinsert(tree, rbProcess, 5);
	grandparent = 0;
	break;
	
  case 5:
    grandparent = retrieveGrandparentproc(rbProcess);
	
	if(grandparent != 0){
		grandparent->color = RED;
		rbProcess->p->color = BLACK;
		if(rbProcess == rbProcess->p->l && rbProcess->p == grandparent->l){
			rightrotate(tree, grandparent);
		} else if(rbProcess == rbProcess->p->r && rbProcess->p == grandparent->r){
			leftrotate(tree, grandparent);
		}
	}
	
	grandparent = 0;
	break;
	
  default:
	break;
  }
  return;
}

void
add_to_tree(struct rbtree* tree, struct proc* p){
  if(!full(tree)){	
    //actually insert process into tree
    tree->root = insertproc(tree->root, p);
    if(tree->length == 0)
      tree->root->p = 0;
        
    tree->length += 1;
    
    //Calculate process weight
    p->weight = compute_weight(p->nice_value);

    //perform total weight calculation 
    tree->total_weight += p->weight;
    
    //Check for possible cases for Red Black tree property violations
    fixinsert(tree, p, 1);
      
    //This function call will find the process with the smallest vRuntime, unless 
    //there was no insertion of a process that has a smaller minimum virtual runtime then the process that is being pointed by min_vruntime
    if(tree->min_vruntime == 0 || tree->min_vruntime->l != 0)
      tree->min_vruntime = minproc(tree->root);
  }
}

void
fixdelete(struct rbtree* tree, struct proc* parentProc, struct proc* process, int cases){
  struct proc* parentProcess;
  struct proc* childProcess;
  struct proc* siblingProcess;
  
  switch(cases){
	case 1:
		//Replace smallest virtual Runtime process with its right child 
		parentProcess = parentProc;
		childProcess = process->r;
		
		//if the process being removed is on the root
		if(process == tree->root){
			
			tree->root = childProcess;
			if(childProcess != 0){
				childProcess->p = 0;
				childProcess->color = BLACK;
			}
			
		} else if(childProcess != 0 && !(process->color == childProcess->color)){
			//Replace current process by it's right child
			childProcess->p = parentProcess;
			parentProcess->l = childProcess;
			childProcess->color = BLACK;		
		} else if(process->color == RED){		
			parentProcess->l = childProcess;
		} else {	
			if(childProcess != 0)
				childProcess->p = parentProcess;
			
			
			parentProcess->l = childProcess;
			fixdelete(tree, parentProcess, childProcess, 2);
		}
		
		process->p = 0;
		process->l = 0;
		process->r = 0;
		parentProcess = 0;
		childProcess = 0;
		break;
		
	case 2:
		
		//Check if process is not root,i.e parentProc != 0, and process is black
		while(process != tree->root && (process == 0 || process->color == BLACK)){
			
			////Obtain sibling process
			if(process == parentProc->l){
				siblingProcess = parentProc->r;
				
				if(siblingProcess != 0 && siblingProcess->color == RED){
					siblingProcess->color = BLACK;
					parentProc->color = RED;
					leftrotate(tree, parentProc);
					siblingProcess = parentProc->r;
				}
				if((siblingProcess->l == 0 || siblingProcess->l->color == BLACK) && (siblingProcess->r == 0 || siblingProcess->r->color == BLACK)){
					siblingProcess->color = RED;
					//Change process pointer and parentProc pointer
					process = parentProc;
					parentProc = parentProc->p;
				} else {
					if(siblingProcess->r == 0 || siblingProcess->r->color == BLACK){
						//Color left child
						if(siblingProcess->l != 0){
							siblingProcess->l->color = BLACK;
						} 
						siblingProcess->color = RED;
						rightrotate(tree, siblingProcess);
						siblingProcess = parentProc->r;
					}
					
					siblingProcess->color = parentProc->color;
					parentProc->color = BLACK;
					siblingProcess->r->color = BLACK;
					leftrotate(tree, parentProc);
					process = tree->root;
				}
			} 
		}
		if(process != 0)
			process->color = BLACK;
		
		break;
	
	default:
		break;
  }
  return;
	
}

struct proc*
next_process(struct rbtree* tree){
  struct proc* p;	//Process pointer utilized to hold the address of the process with smallest VRuntime
  if(!empty(tree)){
    //retrive the process with the smallest virtual runtime by removing it from the red black tree and returning it
    p = tree->min_vruntime;	

    fixdelete(tree, tree->min_vruntime->p, tree->min_vruntime, 1);
    tree->length -= 1;

    //Determine new process with the smallest virtual runtime
    tree->min_vruntime = minproc(tree->root);

    //If the number of processes are greater than the division between latency and minimum granularity
    //then recalculate the period for the processes
    //This condition is performed when the scheduler selects the next process to run
    //The formula can be found in CFS tuning article by Jacek Kobus and Refal Szklarski
    //In the CFS schduler tuning section:
    if(tree->length > (latency / min_granularity)){
      tree->period = tree->length * min_granularity;
    } else {
      tree->period = latency;
    }

    //Calculate retrieved process's time slice based on formula: period*(process's weight/ red black tree weight)
    //Where period is the length of the epoch
    //The formula can be found in CFS tuning article by Jacek Kobus and Refal Szklarski
    //In the scheduling section:
    p->max_exec_time = (tree->period * p->weight / tree->total_weight);
    
    //Recalculate total weight of red-black tree
    tree->total_weight -= p->weight;
    return p;
  }
  return 0;
}

int
should_preempt(struct proc* current, struct proc* min_vruntime){

  int runtime = current->curr_runtime;
  
  if((runtime >= current->max_exec_time) && (runtime >= min_granularity)){
  	return 1;
  }

  if(min_vruntime != 0 && min_vruntime->state == RUNNABLE && current->vruntime > min_vruntime->vruntime){
	  if((runtime == 0) || (runtime >= min_granularity)){
		  return 1;
  	}
  }

  //No preemption should occur
  return 0;
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  treeinit(runnableTasks, "runnableTasks");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  p->vruntime = 0;
  p->curr_runtime = 0;
  p->max_exec_time = 0;
  p->nice_value = 0;
  p->weight = compute_weight(p->nice_value);

  p->l = 0;
  p->r = 0;
  p->p = 0;
  
  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  add_to_tree(runnableTasks, p);

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  add_to_tree(runnableTasks, np);

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    p = next_process(runnableTasks);
    while(p != 0){
    if(p->state == RUNNABLE){		
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    } 

    p = next_process(runnableTasks);
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Determine if the currently running process should be preempted or allowed to continue running
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&ptable.lock);  //DOC: yieldlock
  if(should_preempt(p, runnableTasks->min_vruntime) == 1){
    p->state = RUNNABLE;
    p->vruntime = p->vruntime + p->curr_runtime;
    p->curr_runtime = 0;
    add_to_tree(runnableTasks, p);
    sched();
  }
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;

      // Update runtime stats of process being woken up
      p->vruntime = p->vruntime + p->curr_runtime;
      p->curr_runtime = 0;

      // Insert process after it has finished Sleeping
      add_to_tree(runnableTasks, p);
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING){
        p->state = RUNNABLE;

        // Update runtime stats of process being killed
        p->vruntime = p->vruntime + p->curr_runtime;
        p->curr_runtime = 0;

        // insert process into runnableTask tree
        add_to_tree(runnableTasks, p);
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runnable",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
