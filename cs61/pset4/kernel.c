#include "kernel.h"
#include "lib.h"

// kernel.c
//
//    This is the kernel.


// INITIAL PHYSICAL MEMORY LAYOUT
//
//  +-------------- Base Memory --------------+
//  v                                         v
// +-----+--------------------+----------------+--------------------+---------/
// |     | Kernel      Kernel |       :    I/O | App 1        App 1 | App 2
// |     | Code + Data  Stack |  ...  : Memory | Code + Data  Stack | Code ...
// +-----+--------------------+----------------+--------------------+---------/
// 0  0x40000              0x80000 0xA0000 0x100000             0x140000
//                                             ^
//                                             | \___ PROC_SIZE ___/
//                                      PROC_START_ADDR

#define PROC_SIZE 0x40000       // initial state only

static proc processes[NPROC];   // array of process descriptors
                                // Note that `processes[0]` is never used.
proc* current;                  // pointer to currently executing proc

#define HZ 100                  // timer interrupt frequency (interrupts/sec)
static unsigned ticks;          // # timer interrupts so far

void schedule(void);
void run(proc* p) __attribute__((noreturn));


// PAGEINFO
//
//    The pageinfo[] array keeps track of information about each physical page.
//    There is one entry per physical page.
//    `pageinfo[pn]` holds the information for physical page number `pn`.
//    You can get a physical page number from a physical address `pa` using
//    `PAGENUMBER(pa)`. (This also works for page table entries.)
//    To change a physical page number `pn` into a physical address, use
//    `PAGEADDRESS(pn)`.
//
//    pageinfo[pn].refcount is the number of times physical page `pn` is
//      currently referenced. 0 means it's free.
//    pageinfo[pn].owner is a constant indicating who owns the page.
//      PO_KERNEL means the kernel, PO_RESERVED means reserved memory (such
//      as the console), and a number >=0 means that process ID.
//
//    pageinfo_init() sets up the initial pageinfo[] state.

typedef struct physical_pageinfo {
    int8_t owner;
    int8_t refcount;
} physical_pageinfo;

static physical_pageinfo pageinfo[PAGENUMBER(MEMSIZE_PHYSICAL)];

typedef enum pageowner {
    PO_FREE = 0,                // this page is free
    PO_RESERVED = -1,           // this page is reserved memory
    PO_KERNEL = -2              // this page is used by the kernel
} pageowner_t;

static void pageinfo_init(void);


// Memory functions

void virtual_memory_check(void);
void memshow_physical(void);
void memshow_virtual(x86_pagetable* pagetable, const char* name);
void memshow_virtual_animate(void);


// kernel(command)
//    Initialize the hardware and processes and start running. The `command`
//    string is an optional string passed from the boot loader.

static void process_setup(pid_t pid, int program_number);


void kernel(const char* command) {
    hardware_init();
    pageinfo_init();
    console_clear();
    timer_init(HZ);

    // Set up process descriptors
    memset(processes, 0, sizeof(processes));
    
      
    //virtual_memory_map(kernel_pagetable, 0, 0, PROC_START_ADDR, PTE_P | PTE_W);
    
    uintptr_t console = 0xb8000;
    virtual_memory_map(kernel_pagetable, 0, 0, console, PTE_P | PTE_W);
        
    virtual_memory_map(kernel_pagetable, console, console, PAGESIZE, PTE_P | PTE_W | PTE_U);
    
    virtual_memory_map(kernel_pagetable, console+PAGESIZE, console+PAGESIZE, PROC_START_ADDR, PTE_P | PTE_W);
    // initialize all processes
    for (pid_t i = 0; i < NPROC; i++) {
        processes[i].p_pid = i;
        processes[i].p_state = P_FREE;
    }

    if (command && strcmp(command, "fork") == 0)
        process_setup(1, 4);

    else if (command && strcmp(command, "forkexit") == 0)
        process_setup(1, 5);
    else
        for (pid_t i = 1; i <= 4; ++i)
            process_setup(i, i - 1);

    // Switch to the first process using run()
    run(&processes[1]);
}
/* 
 *  Loop through all the physical pages to find a free physical address. 
 *  If one is not available, print "Out of Physical Memory!"
 */
uintptr_t find_address () 
{
  for (int i = 0; i < NPAGES; ++i)
  {
     uintptr_t my_addr = PAGEADDRESS(i);
     
     if (pageinfo[PAGENUMBER(my_addr)].refcount == 0)
       return my_addr;
  }
  console_printf(CPOS(24, 0), 0x0C00, "Out of Physical Memory!");
  return -1;
} 

x86_pagetable* page_from_new_addr(int8_t owner)
{
    uintptr_t my_addr = find_address(); 
   
    if (0 > (int) my_addr) return 0;
    
    x86_pagetable* my_pagetable = (x86_pagetable*) my_addr;
   
    uintptr_t r = physical_page_alloc(my_addr, owner);
   
    return my_pagetable;
}

/*
 * grabs a free pagetable and copies data into it from the arg pagetable's L2 table.
 */
x86_pagetable* copy_pagetable(x86_pagetable* pagetable, int8_t owner, size_t size)
{
    // declare pagetable.
    x86_pagetable* L2_table = page_from_new_addr(owner); 
   
    // get the page entry from the kernel level2 pagetable for memcpy.    
    x86_pageentry_t pe  = pagetable->entry[0];
    
    // get kernel level 2 page
    x86_pagetable* template_page = (x86_pagetable *) PTE_ADDR(pe);

    // memcopy into this page 
    memcpy(L2_table, template_page, size); 
    
    if (size < PAGESIZE) 
       memset((char *) L2_table+size, 0, PAGESIZE-size);
    
    return L2_table;
}

void process_setup(pid_t pid, int program_number) {
  
    // initialize a process.
    process_init(&processes[pid], 0);
    
    // Level 1
    x86_pagetable* L1_table = page_from_new_addr(pid);
    
    // Clear out Level 1 Table
    memset(L1_table, 0, sizeof(x86_pagetable));
    
    // define size to copy
    size_t size = PAGENUMBER(PROC_START_ADDR)*sizeof(x86_pageentry_t);
    
    // Level 2
    x86_pagetable* L2_table = copy_pagetable(kernel_pagetable, pid, size);
      
    // Link up
    L1_table->entry[0] = (x86_pageentry_t) L2_table | PTE_P | PTE_W | PTE_U;
 
    // Define
    processes[pid].p_pagetable = L1_table;
    
    // load the program
    int r = program_load(&processes[pid], program_number);
    assert(r >= 0);
    
    // set the stack to start at MEMSIZE_VIRTUAL.
    processes[pid].p_registers.reg_esp = MEMSIZE_VIRTUAL; 
    
    // we need this stack page for the physical address.
    uintptr_t p_page = PROC_START_ADDR + PROC_SIZE * pid - PAGESIZE;
    
    uintptr_t v_page = MEMSIZE_VIRTUAL - PAGESIZE;
    physical_page_alloc(p_page, pid);
    
    // pass in our basic process page.
    virtual_memory_map(processes[pid].p_pagetable, v_page, p_page,
                       PAGESIZE, PTE_P|PTE_W|PTE_U);
                       
    processes[pid].p_state = P_RUNNABLE;
}

// physical_page_alloc(addr, owner)
//    Allocates the page with physical address `addr` to the given owner.
//    Fails if physical page `addr` was already allocated. Returns 0 on
//    success and -1 on failure. Used by the program loader.
//
//    PAGENUMBER DIVIDES AN ADDRESS BY 4096.
//    PAGEADDRESS MULTIPLIES A NUMBER BY 4096.
//

int physical_page_alloc(uintptr_t addr, int8_t owner) {

    // check for initial address. 
    if ((addr & 0xFFF) != 0
        || addr >= MEMSIZE_PHYSICAL
        || pageinfo[PAGENUMBER(addr)].refcount != 0)
        return -1;
    else {
        // assign initial refcount.
        pageinfo[PAGENUMBER(addr)].refcount = 1;
        
        // assign owner. 
        pageinfo[PAGENUMBER(addr)].owner = owner;
        return 0;
    }
}
/* 
 *  Used in INT_SYS_FORK. Assigns a new physical page to a writeable address.
 */
int child_phys_page (int j, uintptr_t va, vamapping vam) {

   // get new CHILD phys table; alloc step taken care of.
   x86_pagetable* phys_table = page_from_new_addr(processes[j].p_pid);
 
   if (0 > phys_table) return -1;
   
   assert (vam.pa >= 0);

   // get template table from physical address. 
   x86_pagetable* template_table = (x86_pagetable*) PTE_ADDR(vam.pa);
                        
   assert(template_table >= 0);
                  
   // memcopy its data into new physical table
   memcpy(phys_table, template_table, PAGESIZE);
                        
   // translate physical table into an address. 
   uintptr_t pa = (uintptr_t) phys_table;
   
   if (!pa & PTE_P) return -1;
   // map virtual address to new physical address.           
   virtual_memory_map(processes[j].p_pagetable, va, pa,
   PAGESIZE, PTE_P|PTE_W|PTE_U);
                    
   return 0;
}
/*
 *  Does the heavy lifting for fork.
 */
int fork_process(x86_pagetable* L1_parent, x86_pagetable* L2_child, int j)
{
  /*
   *  Instantiate Level 1 Child; Wire up to New Process
   */
  // get new pagetable for the child.
  x86_pagetable* L1_child = page_from_new_addr(processes[j].p_pid); 
  
  if (0 > L1_child) return -1;
  
  // blank out the L1 child data. 
  memset(L1_child, 0, PAGESIZE);
      
  // assign child L1 table to child process.
  processes[j].p_pagetable = L1_child;
  
  // and finally map page P at address V in the child process’s page table.
  L1_child->entry[0] = (x86_pageentry_t) L2_child | PTE_P | PTE_U | PTE_W;
  
  /* 
   *  Get Parent's Level 2 Page
   */           
  // get the page entry from the kernel level2 pagetable for memcpy.    
  x86_pageentry_t pe  = L1_parent->entry[0];
           
  // get address of template L2 page
  x86_pagetable* L2_parent = (x86_pagetable *) PTE_ADDR(pe);

  // iterate over all virtual addresses.
  for (uintptr_t va = PROC_START_ADDR; va < MEMSIZE_VIRTUAL-1; va+=PAGESIZE)
  {
    // map it!
    vamapping vam = virtual_memory_lookup(L1_parent, va);
    int owner = pageinfo[vam.pn].owner;
    
    /*
     *  Assign Physical Page or Increment Ref Count
     */            
    // check that this va belongs to this pagetable.
    if (vam.pn >= 0 && vam.pa >= 0)
    {
           // if non-kernel and writeable, assign a new phys page to child.
           // else, increment ref count to show it is being shared.
           if ((vam.perm & (PTE_U|PTE_W)) == (PTE_U|PTE_W))
           {
              if (child_phys_page(j, va, vam) < 0)
                return -1;
             
           }
           else if (pageinfo[PAGENUMBER(vam.pa)].refcount > 0 &&
               pageinfo[PAGENUMBER(vam.pa)].owner > 0)
               pageinfo[PAGENUMBER(vam.pa)].refcount++; 
     }  
  }
  /*
   *  Establish Parent & Child Registers
   */
        
  // copy registers
  processes[j].p_registers = current->p_registers;
           
  // set eax registers for parent and child
  current->p_registers.reg_eax = processes[j].p_pid;
  processes[j].p_registers.reg_eax = 0; //processes[j].p_pid;  
           
  // declare as runnable, stop looking for processes.
  processes[j].p_state = P_RUNNABLE; 
           
  return 0;
}
// NEXT UP: Indent, trace the PTE_P error, and figure out what is going on; 

// exception(reg)
//    Exception handler (for interrupts, traps, and faults).
//
//    The register values from exception time are stored in `reg`.
//    The processor responds to an exception by saving application state on
//    the kernel's stack, then jumping to kernel assembly code (in
//    k-exception.S). That code saves more registers on the kernel's stack,
//    then calls exception().
//
//    Note that hardware interrupts are disabled whenever the kernel is running.

void exception(x86_registers* reg) {
    // Copy the saved registers into the `current` process descriptor
    // and always use the kernel's page table.
    current->p_registers = *reg;
    set_pagetable(kernel_pagetable);

    // It can be useful to log events using `log_printf`.
    // Events logged this way are stored in the host's `log.txt` file.
    /*log_printf("proc %d: exception %d\n", current->p_pid, reg->reg_intno);*/

    // Show the current cursor location and memory state.
    console_show_cursor(cursorpos);
    virtual_memory_check();
    memshow_physical();
    memshow_virtual_animate();

    // If Control-C was typed, exit the virtual machine.
    check_keyboard();

    // Actually handle the exception.
    switch (reg->reg_intno) {

    case INT_SYS_PANIC:
        panic(NULL);
        break;                  // will not be reached

    case INT_SYS_GETPID:
        current->p_registers.reg_eax = current->p_pid;
        break;

    case INT_SYS_YIELD:
        schedule();
        break;                  /* will not be reached */

    case INT_SYS_EXIT: {
        // I started working on this, and hope to return to it during reading period.
        if (current->p_pid > 1 && current->p_pagetable != kernel_pagetable)
        {
            int start = PAGENUMBER(PROC_START_ADDR);
            // loop through all phys pages.
            
            // is there an equivalent
            for (int pn = 0; pn < PAGENUMBER(MEMSIZE_PHYSICAL); ++pn)
            {
              // if page's owner is equal to id of the cancelled process...
              if (current->p_pid == pageinfo[pn].owner && 
                 pageinfo[pn].owner != PO_KERNEL &&
                 pageinfo[pn].owner != PO_RESERVED)
              {
                 // in pageinfo, declare this item's owner and refcount as 0. 
                 pageinfo[pn].owner = 0;
                 pageinfo[pn].refcount = 0;
                 
                 // grab page address. 
                 uintptr_t L1_addr = PAGEADDRESS(pn);
                 
                 // grab L1 page. 
                 x86_pagetable* L1_page = (x86_pagetable*) L1_addr;

                 memset (L1_page, 0, PAGESIZE);
                 
                 // Currently clearing out L2 pages is throwing
                 // the owner / expected owner assert that involves level 1 page tables, 
                 // so I have taken out that code for now. 
                 // I also know there are other steps to take care of here. 
                 // I hope to return to it during reading period.  
              }
  
            }
            // free the process only if there are no pages wired up to it.
            int pages = 0;
            for (int pn = 0; pn < PAGENUMBER(MEMSIZE_PHYSICAL); ++pn) 
              if ( pageinfo[pn].owner == current->p_pid)  pages++;
            
            if (pages == 0) 
               current->p_state = P_FREE; 
        }
        break;
    }
    case INT_SYS_PAGE_ALLOC: {
        // define the address;  
        uintptr_t addr = current->p_registers.reg_eax;
        
        // get any physical page. 
        int p_addr = find_address();

        physical_page_alloc(p_addr, current->p_pid);
        
        // map it to a virtual page. 
        if (p_addr >= 0)
            virtual_memory_map(current->p_pagetable, addr, p_addr,
                               PAGESIZE, PTE_P|PTE_W|PTE_U);
                               
        // not sure if I should be changing this.                      
        current->p_registers.reg_eax = p_addr;
        break;
    }
  
    case INT_SYS_FORK: {
      // this flag is set to 1 when a process is found
      int process_found = 0;    
      for (int j = 1; j < NPROC && process_found == 0; j++)
      {
        if (processes[j].p_state == P_FREE)
        {
           process_found = 1;
           // get the current process's pagetable. 
           x86_pagetable* L1_parent = current->p_pagetable;
           
           // pass in parent L1 pagetable because copy_pagetable will grab L1's L2 table.
           x86_pagetable* L2_child = copy_pagetable (L1_parent, processes[j].p_pid, PAGESIZE); 
           
           if (0 > (int) L2_child) 
           {
                current->p_registers.reg_eax = -1;
                processes[j].p_registers.reg_eax = -1; 
           }
           else
           {
             if (fork_process(L1_parent, L2_child, j) == -1)
             {    
                current->p_registers.reg_eax = -1;
                processes[j].p_registers.reg_eax = -1;
            
               // any pages the belongs to this process, reset refcount and owner.
               for (int pn = 0; pn < PAGENUMBER(MEMSIZE_PHYSICAL); ++pn)
               {
                  if (pageinfo[pn].owner == j)
                  {
                   pageinfo[pn].owner = 0;
                   pageinfo[pn].refcount = 0;
                  }
               }  
             } 
           }
         } 
       } 
       // return zero if we've completed the loop and no free processes were found.
       if (process_found == 0) current->p_registers.reg_eax = -1; 
       break;
    } 

    case INT_TIMER:
        ++ticks;
        schedule();
        break;                  /* will not be reached */

    case INT_PAGEFAULT: {
        // Analyze faulting address and access type.
        uintptr_t addr = rcr2();
        const char* operation = reg->reg_err & PFERR_WRITE
                ? "write" : "read";
        const char* problem = reg->reg_err & PFERR_PRESENT
                ? "protection problem" : "missing page";
        
        if (!(reg->reg_err & PFERR_USER))
            panic("Kernel page fault for 0x%08X (%s %s, eip=%p)!\n",
                  addr, operation, problem, reg->reg_eip);
        console_printf(CPOS(24, 0), 0x0C00,
                       "Process %d page fault for 0x%08X (%s %s, eip=%p)!\n",
                       current->p_pid, addr, operation, problem, reg->reg_eip);
        current->p_state = P_BROKEN;
        break;
    }

    default:
        panic("Unexpected exception %d!\n", reg->reg_intno);
        break;                  /* will not be reached */

    }


    // Return to the current process (or run something else).
    if (current->p_state == P_RUNNABLE)
        run(current);
    else
        schedule();
}


// schedule
//    Pick the next process to run and then run it.
//    If there are no runnable processes, spins forever.

void schedule(void) {
    pid_t pid = current->p_pid;
    while (1) {
        pid = (pid + 1) % NPROC;
        if (processes[pid].p_state == P_RUNNABLE)
            run(&processes[pid]);
        // If Control-C was typed, exit the virtual machine.
        check_keyboard();
    }
}


// run(p)
//    Run process `p`. This means reloading all the registers from
//    `p->p_registers` using the `popal`, `popl`, and `iret` instructions.
//
//    As a side effect, sets `current = p`.

void run(proc* p) {
    assert(p->p_state == P_RUNNABLE);
    current = p;
    set_pagetable(p->p_pagetable);
    asm volatile("movl %0,%%esp\n\t"
                 "popal\n\t"
                 "popl %%es\n\t"
                 "popl %%ds\n\t"
                 "addl $8, %%esp\n\t"
                 "iret"
                 :
                 : "g" (&p->p_registers)
                 : "memory");

 spinloop: goto spinloop;       // should never get here
}


// pageinfo_init
//    Initialize the `pageinfo[]` array.

void pageinfo_init(void) {
    extern char end[];

    for (uintptr_t addr = 0; addr < MEMSIZE_PHYSICAL; addr += PAGESIZE) {
        int owner;
        if (physical_memory_isreserved(addr))
            owner = PO_RESERVED;
        else if ((addr >= KERNEL_START_ADDR && addr < (uintptr_t) end)
                 || addr == KERNEL_STACK_TOP - PAGESIZE)
            owner = PO_KERNEL;
        else
            owner = PO_FREE;
        pageinfo[PAGENUMBER(addr)].owner = owner;
        pageinfo[PAGENUMBER(addr)].refcount = (owner != PO_FREE);
    }
}


// virtual_memory_check
//    Check operating system invariants about virtual memory. Panic if any
//    of the invariants are false.

void virtual_memory_check(void) {
    // Process 0 must never be used.
    assert(processes[0].p_state == P_FREE);

    // The kernel page table should be owned by the kernel;
    // its reference count should equal 1, plus the number of processes
    // that don't have their own page tables.
    // Active processes have their own page tables. A process page table
    // should be owned by that process and have reference count 1.
    // All level-2 page tables must have reference count 1.

    // Calculate expected kernel refcount
    int expected_kernel_refcount = 1;
    for (int pid = 0; pid < NPROC; ++pid)
        if (processes[pid].p_state != P_FREE
            && processes[pid].p_pagetable == kernel_pagetable)
            ++expected_kernel_refcount;

    for (int pid = -1; pid < NPROC; ++pid) {
        if (pid >= 0 && processes[pid].p_state == P_FREE)
            continue;

        x86_pagetable* pagetable;
        int expected_owner, expected_refcount;
        if (pid < 0 || processes[pid].p_pagetable == kernel_pagetable) {
            pagetable = kernel_pagetable;
            expected_owner = PO_KERNEL;
            expected_refcount = expected_kernel_refcount;
        } else {
            pagetable = processes[pid].p_pagetable;
            expected_owner = pid;
            expected_refcount = 1;
        }

        // Check main (level-1) page table
        assert(PTE_ADDR(pagetable) == (uintptr_t) pagetable);
        assert(PAGENUMBER(pagetable) < NPAGES);
        assert(pageinfo[PAGENUMBER(pagetable)].owner == expected_owner);
        assert(pageinfo[PAGENUMBER(pagetable)].refcount == expected_refcount);

        // Check level-2 page tables
        for (int pn = 0; pn < PAGETABLE_NENTRIES; ++pn)
            if (pagetable->entry[pn] & PTE_P) {
                x86_pageentry_t pte = pagetable->entry[pn];
                assert(PAGENUMBER(pte) < NPAGES);
                assert(pageinfo[PAGENUMBER(pte)].owner == expected_owner);
                assert(pageinfo[PAGENUMBER(pte)].refcount == 1);
            }
    }
    // Check that all referenced pages refer to active processes
    for (int pn = 0; pn < PAGENUMBER(MEMSIZE_PHYSICAL); ++pn)
    {
        if (pageinfo[pn].refcount > 0 && pageinfo[pn].owner >= 0)
            assert(processes[pageinfo[pn].owner].p_state != P_FREE);
    }
}

// memshow_physical
//    Draw a picture of physical memory on the CGA console.

static const uint16_t memstate_colors[] = {
    'K' | 0x0D00, 'R' | 0x0700, '.' | 0x0700, '1' | 0x0C00,
    '2' | 0x0A00, '3' | 0x0900, '4' | 0x0E00, '5' | 0x0F00,
    '6' | 0x0C00, '7' | 0x0A00, '8' | 0x0900, '9' | 0x0E00,
    'A' | 0x0F00, 'B' | 0x0C00, 'C' | 0x0A00, 'D' | 0x0900,
    'E' | 0x0E00, 'F' | 0x0F00
};

void memshow_physical(void) {
    console_printf(CPOS(0, 32), 0x0F00, "PHYSICAL MEMORY");
    for (int pn = 0; pn < PAGENUMBER(MEMSIZE_PHYSICAL); ++pn) {
        if (pn % 64 == 0)
            console_printf(CPOS(1 + pn / 64, 3), 0x0F00, "0x%06X ", pn << 12);

        int owner = pageinfo[pn].owner;
        if (pageinfo[pn].refcount == 0)
            owner = PO_FREE;
        uint16_t color = memstate_colors[owner - PO_KERNEL];
        // darker color for shared pages
        if (pageinfo[pn].refcount > 1)
            color &= 0x77FF;

        console[CPOS(1 + pn / 64, 12 + pn % 64)] = color;
   
    }
}


// memshow_virtual(pagetable, name)
//    Draw a picture of the virtual memory map `pagetable` (named `name`) on
//    the CGA console.

void memshow_virtual(x86_pagetable* pagetable, const char* name) {
    assert((uintptr_t) pagetable == PTE_ADDR(pagetable));

    console_printf(CPOS(10, 26), 0x0F00, "VIRTUAL ADDRESS SPACE FOR %s", name);
    for (uintptr_t va = 0; va < MEMSIZE_VIRTUAL; va += PAGESIZE) {
        vamapping vam = virtual_memory_lookup(pagetable, va);
        uint16_t color;
        if (vam.pn < 0)
            color = ' ';
        else {
            assert(vam.pa < MEMSIZE_PHYSICAL);
            int owner = pageinfo[vam.pn].owner;
            if (pageinfo[vam.pn].refcount == 0)
                owner = PO_FREE;
            color = memstate_colors[owner - PO_KERNEL];
            // reverse video for user-accessible pages
            if (vam.perm & PTE_U)
                color = ((color & 0x0F00) << 4) | ((color & 0xF000) >> 4)
                    | (color & 0x00FF);
            // darker color for shared pages
            if (pageinfo[vam.pn].refcount > 1)
                color &= 0x77FF;
        }
        uint32_t pn = PAGENUMBER(va);
        if (pn % 64 == 0)
            console_printf(CPOS(11 + pn / 64, 3), 0x0F00, "0x%06X ", va);
        console[CPOS(11 + pn / 64, 12 + pn % 64)] = color;
    }
}


// memshow_virtual_animate
//    Draw a picture of process virtual memory maps on the CGA console.
//    Starts with process 1, then switches to a new process every 0.25 sec.

void memshow_virtual_animate(void) {
    static unsigned last_ticks = 0;
    static int showing = 1;

    // switch to a new process every 0.25 sec
    if (last_ticks == 0 || ticks - last_ticks >= HZ / 2) {
        last_ticks = ticks;
        ++showing;
    }

    // the current process may have died -- don't display it if so
    while (showing <= 2*NPROC && processes[showing % NPROC].p_state == P_FREE)
        ++showing;
    showing = showing % NPROC;

    if (processes[showing].p_state != P_FREE) {
        char s[4];
        snprintf(s, 4, "%d ", showing);
        memshow_virtual(processes[showing].p_pagetable, s);
    }
}
