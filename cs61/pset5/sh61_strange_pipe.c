#include "sh61.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>


// struct command
//    Data structure describing a command. Add your own stuff.

typedef struct command command;
struct command {
  int argc;      // number of arguments
  char** argv;   // arguments, terminated by NULL
  pid_t pid;     // process ID running this command, -1 if none
  command * next;
  int wait;
  int cond;
  int pipe;
};


// command_alloc()
//    Allocate and return a new command structure.

static command* command_alloc(void) {
    command* c = (command*) malloc(sizeof(command));
    c->argc = 0;
    c->argv = NULL;
    c->pid = -1;
    c->next = NULL;
    c->cond = 0;
    c->pipe = 0;
    return c;
}
// struct command_list
//   A list of command lists. In essence, a 2D array except with linked lists. 
 
typedef struct command_list command_list;
struct command_list  {
  command_list * next;
  int background;
  int pipe_input; 
  command* commands;
};

static command_list* command_list_alloc(void) {
  command_list* cl = (command_list *) malloc(sizeof(command_list));
  cl->background = 0;
  cl->pipe_input = 0;
  cl->next = NULL;
  cl->commands = NULL;
  return cl;
}

/*
// command_group -- a group of command lists
typedef struct command_group command_group;
struct command_group {
   // this will point to the next command group. 
   command_group* next;
   // this is the head of the lists it has. 
   command_list* lists;
};*/
/*
// allocate a command group
static command_group* command_group_alloc(void) {
  command_group* cg = (command_group *) malloc(sizeof(command_group));
  cg->next = NULL;
  cg->lists = NULL;
  return cg;
}
*/
// command_matrix_free(c)
//    Free a 2D matrix of commands.
//    we may want to change command_list to command_matrix. 



// command_free(c)
//    Free command structure `c`, including all its words.
static void command_free(command* c) {
    for (int i = 0; i != c->argc; ++i)
        free(c->argv[i]);
    free(c->argv);
    free(c);
}

// command_list_free(command* c)
//    Free an entire command list. 
static void command_chain_free  (command* c) {
    while (c) 
    {
      command* temp = c->next;
      command_free(c);
      c = temp;
    }
}

// chain_list_free (command_list* cl) 
//     frees up a list of command chains. 
static void command_list_free (command_list* cl) {
    while (cl) 
    {
      command_list* temp = cl->next;
      command* c = cl->commands;
      command_chain_free(c);
      cl = temp;
    }
}
// command_append_arg(c, word)
//    Add `word` as an argument to command `c`. This increments `c->argc`
//    and augments `c->argv`.

static void command_append_arg(command* c, char* word) {
    c->argv = (char**) realloc(c->argv, sizeof(char*) * (c->argc + 2));
    c->argv[c->argc] = word;
    c->argv[c->argc + 1] = NULL;
    ++c->argc;
}


// COMMAND EVALUATION

// start_command(c, pgid)
//    Start the single command indicated by `c`. Sets `c->pid` to the child
//    process running the command, and returns `c->pid`.
//
//    PART 1: Fork a child process and run the command using `execvp`.
//    PART 5: Set up a pipeline if appropriate. This may require creating a
//       new pipe (`pipe` system call), and/or replacing the child process's
//       standard input/output with parts of the pipe (`dup2` and `close`).
//       Draw pictures!
//    PART 7: Handle redirections.
//    PART 8: The child process should be in the process group `pgid`, or
//       its own process group (if `pgid == 0`). To avoid race conditions,
//       this will require TWO calls to `setpgid`.

pid_t start_command(command* c, pid_t pgid) {
  (void) pgid;
    // Your code here!
  //printf("%s \n", c->argv[0]);
  int pid = -1;
  
  // we already forked in the case of pipe. 
   
  if (c->pipe == 0) pid = fork();
    // should we set a pid for pipes? 
    
  if(pid == 0 || c->pipe == 1) {
    const char* file = (const char *) c->argv[0];
    char **argv = &(c->argv[0]);
    execvp(file, argv);
  }
  return pid;
}


// run_list(c)
//    Run the command list starting at `c`.
//
//    PART 1: Start the single command `c` with `start_command`,
//        and wait for it to finish using `waitpid`.
//    The remaining parts may require that you change `struct command`
//    (e.g., to track whether a command is in the background)
//    and write code in run_list (or in helper functions!).
//    PART 2: Treat background commands differently.
//    PART 3: Introduce a loop to run all commands in the list.
//    PART 4: Change the loop to handle conditionals.
//
//    PART 5: Change the loop to handle pipelines. Start all processes in
//       the pipeline in parallel. The status of a pipeline is the status of
//       its LAST command.
//    PART 8: - Choose a process group for each pipeline.
//       - Call `set_foreground(pgid)` before waiting for the pipeline.
//       - Call `set_foreground(0)` once the pipeline is complete.
//       - Cancel the list when you detect interruption.

// is groups compatible with what I have set up here? 

/* 
 * Do we need to have our pipe logic in run_list? 
 * What does it mean to start a pipeline in parallel? Is it required? 
 */
void run_list(command* c) {
  int exit_status = 0;
  while(c)
    {
       if(((c->cond == 1) && exit_status) || ((c->cond == 2) && !exit_status))
	   {
	     c = c->next;
	     continue;
	   }
       pid_t pid = start_command(c, 0);
      
       // if we are not a child process,
       // invoke waitpid followed by exit logic.
       if(pid != 0){
     
	   int status;
	   waitpid(pid, &status, 0);
	  	  
	   if(WIFEXITED(status)) 
	   exit_status = WEXITSTATUS(status); 

    }

      c = c->next;
    }
}

/* 
 * Starting with an initial pipe framework here. 
 * Currently only works for one pipe. 
 * Pass along file descriptors
 * each command group could store its input and output file descriptor...
 * every turn of the loop, if pipeline is continuing, dup file descriptors appropriaately. 
 */
/*command_list* pipe_command(command_list* my_list)
{
   command_list* cur_list = my_list;
   command* c1 = cur_list->commands;
   c1->pipe = 1;
  
   command_list* list2 = my_list->next;
   command* c2 = list2->commands;
 
   // create pipe array, which holds two file descriptors. 
   // ***take stdout and make it the stdin of the second command
   // loop over everything in command group list
   // each group has its own pipefd in its struct. 
   // split pipes between processes. 
  
   int pipefd[2];
   
   // convert to pipe.
   int r = pipe(pipefd);
   
   // validate. 
   if (r < 0) {
        fprintf(stderr, "pipe: %s\n", strerror(errno));
        // or, shorter: `perror("pipe");`
        exit(1);
    }
    // fork a process.
    pid_t firstpid = fork();
    assert (firstpid >= 0);
    if (firstpid == 0)
    {
       // close child's read end. 
       close(pipefd[0]);  
       
       // set write end of child to standard out. 
       dup2(pipefd[1], STDOUT_FILENO);
 
       // now we can close this end. 
       close(pipefd[1]);
        
       run_list(c1);
 
    }
    pid_t secondpid = fork();
      if (secondpid == -1) {
        perror("Could not fork!\n");
        exit(1);
      }
    else if (secondpid == 0)
    {
      // close the write end. 
      close(pipefd[1]); 

      // make second child's stdin the pipe's read end
      dup2(pipefd[0], STDIN_FILENO);
    
      close(pipefd[0]); // no need keep 2 copies of pipe

      run_list(c2);
      exit(1);
    }

    // back in main parent process
    close(pipefd[0]); // parent does not read or write to pipe
    close(pipefd[1]);
    waitpid(firstpid, NULL, 0);
    waitpid(secondpid, NULL, 0);

   return list2;
}*/
command_list* pipe_command(command_list* my_list)
{
   int first_pass = 0;
   command_list* cur_list = my_list;
   command_list* list2 = my_list->next;
   
   while (cur_list->pipe_input == 1)
   {
     printf("proc 1\n");
     command* c1 = cur_list->commands;
     c1->pipe = 1;
  
     list2 = my_list->next;
     command* c2 = list2->commands;
 
     // declare array with file descriptors.
     int pipefd[2];
   
     // convert to pipe.
     int r = pipe(pipefd);
   
     // validate. 
     if (r < 0) 
     {
        fprintf(stderr, "pipe: %s\n", strerror(errno));
        // or, shorter: `perror("pipe");`
        exit(1);
     }
    
      // fork a process.
      pid_t firstpid = fork();
      assert (firstpid >= 0);
      if (firstpid == 0)
      {
      
       if (first_pass == 0)
         close(pipefd[0]);  
       else
         dup2(pipefd[0], STDIN_FILENO);
       
       // set write end of child to standard out.
       // this will WRITE to standard out.  
       dup2(pipefd[1], STDOUT_FILENO);
 
       // now we can close this end. 
       close(pipefd[1]);
       
       if (first_pass == 1)
         close(pipefd[0]);
        
       // all data will be printed to standard out. 
       run_list(c1);
       
       first_pass = 1;
 
    }
    printf("finished first process\n");
    pid_t secondpid = fork();
      if (secondpid == -1) {
        perror("Could not fork!\n");
        exit(1);
      }
    else if (secondpid == 0)
    {
      if (list2->pipe_input == 0) 
        close(pipefd[1]); 

      // get the arguments by reading them in. 
      dup2(pipefd[0], STDIN_FILENO);
      
      printf("about to close\n");
      close(pipefd[0]); // no need keep 2 copies of pipe

      printf("about to run_list\n");
      
      
      // if this is also input, write to standard out. 
      if (list2->pipe_input == 1)
      {
        c2->pipe = 1;
        printf("about to dup2\n");
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
      }
      printf("run list\n");
      run_list(c2);
      
      // pipe hygiene
      close(pipefd[1]);
      exit(1);
    }

    // back in main parent process
    close(pipefd[0]); // parent does not read or write to pipe
    close(pipefd[1]);
    waitpid(firstpid, NULL, 0);
    waitpid(secondpid, NULL, 0);
    
     cur_list = list2;   
   }
   return list2;
}
// eval_line(c)
//    Parse the command list in `s` and run it via `run_list`.

void eval_line(const char* s) {

    int type;
    char* token;

    /* 
     *  List
     */
    // allocate space for a list of command chains.
    command_list* cl = command_list_alloc();
    
    // declare the cl_head as the start of our "command chain" list. 
    command_list* cl_head = cl;
    
    /* 
     * Initialize command in place at head of in list; place list in head of group. 
     */
    // build the first command
    command* c = command_alloc();
    
    // c will be the first command chain in our first command list.
    cl->commands =  c;
    
    // cl will be the first list in our group. 
    //cg->lists = cl;
    
    while ((s = parse_shell_token(s, &type, &token)) != NULL) {

    switch(type)
	{
	case 3: // &
	  cl->background = 1;
	  
	case 2: // ; 
	  // initialize the next list of command chains.
	  cl->next = command_list_alloc();	  
	
	  // we now focus on this next list that was just initialized.
	  cl = cl->next;
	  
	  // for this list, we initialize a new command.
	  c = command_alloc();
	  
	  // and add this command to the head of the list. 
	  cl->commands = c;
	  break;
	
	// do we need to initialize a new group? 
	// if so we risk swallowing up more output then we'd like. 
	// how about switching list and group for pipe and semicolon.
	// OR just not use group and set a pipe command to true. 
	// because we are looking at equal precedence to semicolons.
	
    case 4: // |
      // initialize the next list of command chains.
      cl->pipe_input = 1;
      
	  cl->next = command_list_alloc();	  
	
	  // we now focus on this next list that was just initialized.
	  cl = cl->next;
	  
	  // for this list, we initialize a new command.
	  c = command_alloc();
	  
	  // and add this command to the head of the list. 
	  cl->commands = c;
   
	break; 
	 
	case 5: // &&
	  c->next = command_alloc();
	  c = c->next;
	  c->cond = 1;
	  break;

	case 6: // ||
	  c->next = command_alloc();
	  c = c->next;
	  c->cond = 2;
	  break;

	default: 
	  command_append_arg(c, token);
	  break;
	  }      
    }
    // at this point, the command group has been built. 

      while(cl_head)
      {
       // declare break_list flag.
	   int break_list = 0;
	  
	   // if this command list is in the background...
	   if(cl_head->background == 1) 
	   {
	     // fork it off as a new process.
	     pid_t pid = fork();
	     
	     // if we are in a child process
	      if(pid == 0) 
	       break_list = 1;
	      else 
	      {
	       // if not a background process, simply move on to next list of commands.
	       cl_head = cl_head->next;
	       continue;
	      }
       }
       
          if (cl_head->pipe_input == 0)   
	         run_list(cl_head->commands);
	      else
	         cl_head = pipe_command(cl_head);

    	  if (break_list == 1)
	      break;
	
	     cl_head = cl_head->next;
	     
      }
   
  

    command_list_free(cl_head);
}


int main(int argc, char* argv[]) {
    FILE* command_file = stdin;
    int quiet = 0;

    // Check for '-q' option: be quiet (print no prompts)
    if (argc > 1 && strcmp(argv[1], "-q") == 0) {
        quiet = 1;
        --argc, ++argv;
    }

    // Check for filename option: read commands from file
    if (argc > 1) {
        command_file = fopen(argv[1], "rb");
        if (!command_file) {
            perror(argv[1]);
            exit(1);
        }
    }

    // - Put the shell into the foreground
    // - Ignore the SIGTTOU signal, which is sent when the shell is put back
    //   into the foreground
    set_foreground(0);
    handle_signal(SIGTTOU, SIG_IGN);

    char buf[BUFSIZ];
    int bufpos = 0;
    int needprompt = 1;

    while (!feof(command_file)) {
        // Print the prompt at the beginning of the line
      if (needprompt && !quiet) {
	printf("sh61[%d]$ ", getpid());
	fflush(stdout);
	needprompt = 0;
      }

        // Read a string, checking for error or EOF
        if (fgets(&buf[bufpos], BUFSIZ - bufpos, command_file) == NULL) {
            if (ferror(command_file) && errno == EINTR) {
                // ignore EINTR errors
                clearerr(command_file);
                buf[bufpos] = 0;
            } else {
                if (ferror(command_file))
                    perror("sh61");
                break;
            }
        }

        // If a complete command line has been provided, run it
        bufpos = strlen(buf);
        if (bufpos == BUFSIZ - 1 || (bufpos > 0 && buf[bufpos - 1] == '\n')) {
            eval_line(buf);
            bufpos = 0;
            needprompt = 1;
        }

        // Handle zombie processes and/or interrupt requests
        // Your code here!
    }

    return 0;
}