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
  int wait;
  command * next;
  int cond;
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
    return c;
}

// struct command_list
//   A list of command lists. In essence, a 2D array except with linked lists. 
 
typedef struct command_list command_list;
struct command_list  {
  command_list * next;
  int background;
  command* commands;
};

static command_list* command_list_alloc(void) {
  command_list* cl = (command_list *) malloc(sizeof(command_list));
  cl->background = 0;
  cl->next = NULL;
  cl->commands = NULL;
  return cl;
}


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
  
  // fork process here.
  pid = fork();
  if(pid == 0) {
    //printf("in child process\n");
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
//    PART 5: Change the loop to handle pipelines. Start all processes in
//       the pipeline in parallel. The status of a pipeline is the status of
//       its LAST command.
//    PART 8: - Choose a process group for each pipeline.
//       - Call `set_foreground(pgid)` before waiting for the pipeline.
//       - Call `set_foreground(0)` once the pipeline is complete.
//       - Cancel the list when you detect interruption.

void run_list(command* c) {
  int exit_status = 0;
  while(c)
    {
       if(((c->cond == 1) && exit_status) || ((c->cond == 2) && !exit_status))
	   {
	     c = c->next;
	     continue;
	   }
       pid_t pid = -1;
       pid = start_command(c, 0);
      
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


// eval_line(c)
//    Parse the command list in `s` and run it via `run_list`.

void eval_line(const char* s) {
    int type;
    char* token;
    // allocate space for a list of command chains.
    command_list* cl = command_list_alloc();
    
    // declare the cl_head as the start of our "command chain" list. 
    command_list* cl_head = cl;

    // build the first command
    command* c = command_alloc();
    
    // c will be the first command in our first command chain.
    cl->commands =  c;

    while ((s = parse_shell_token(s, &type, &token)) != NULL) {

      switch(type)
	{
	case 3: // &
	  cl->background = 1;
	  
	case 2: // ; 
	  cl->next = command_list_alloc();
	  cl = cl->next;
	  c = command_alloc();
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
	  c->cond =2;
	  break;

	default: 
	  command_append_arg(c, token);
	  break;
	}      
    }
    
    while(cl_head)
    {
      // declare break_list flag.
	  int break_list = 0;
	  
	  // if this command list is in the background...
	  if(cl_head->background == 1)  {
	  
	     // fork it off as a new process.
	     pid_t pid = fork();
	     
	     // if we are in a child process
	     if(pid == 0) {
	     
	       // set break_list to 1.
	       break_list = 1;
	  }
	  else 
	  {
	    // if not a background process, simply move on to next list of commands.
	    cl_head = cl_head->next;
	    continue;
	  }
	}
	
	
	// execute it
	//if (head->argc)
	run_list(cl_head->commands);

	if(break_list == 1)
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
