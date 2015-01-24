#include "sh61.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*

Questions: 





*/
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
  int background;
};


// command_alloc()
//    Allocate and return a new command structure.

static command* command_alloc(void) {
    command* c = (command*) malloc(sizeof(command));
    c->argc = 0;
    c->argv = NULL;
    c->pid = -1;
    
    // starts out as NOT a background process, so it waits by default. 
    c->wait = 1;
    c->background = 0;
    c->next = NULL;
    c->cond = 0;
    return c;
}


// command_free(c)
//    Free command structure `c`, including all its words.

static void command_free(command* c) {
    for (int i = 0; i != c->argc; ++i)
        free(c->argv[i]);
    free(c->argv);
    free(c);
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
  
  // if pid is a child
  if(pid == 0) {
    // declare file
    const char* file = (const char *) c->argv[0];
    
    // get the address of c->argv[0]
    char **argv = &(c->argv[0]);
    
    // execvp
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
  
  // initialize exit status. 
  int exit_status = 0;
  
  // iterate through linked list until c is NULL
  while(c)
    {
      // handles the conditionals. 1 is for || and 2 is for &&
      if(((c->cond == 1) && exit_status) || ((c->cond == 2) && !exit_status))
	  {
	    c = c->next;
	    continue;
	  }
	  
	  // declare pid. 
      pid_t pid = start_command(c, 0);
      
      // if pid is non-zero....
      if(pid != 0) { 
	
	 
	  if(c->wait) {
	  
      
	    int status;
	    
	    // wait for child process to stop. 
	    waitpid(pid, &status, 0);
	  
	    // we exited, get the status.
	    if(WIFEXITED(status)) 
	    exit_status = WEXITSTATUS(status); 
	    }	
      }
      
      // go to the next item in the list
      c = c->next;
    }
}


// eval_line(c)
//    Parse the command list in `s` and run it via `run_list`.

void eval_line(const char* s) {
    int type;
    char* token;
    // Your code here!

    // build the command
    command* c = command_alloc();
    command* head = c;
    command* temp;
    int child_pid;    
    while ((s = parse_shell_token(s, &type, &token)) != NULL) {

    switch (type)
    {
      
      
      // ;
      case 2:
      
	  temp = command_alloc();
	  c->next = temp;
	  c = temp;
	  break;
	  
	  // & 
      case 3: 
      // this is a child process, so set wait to 0.
	  c->wait = 0;
	  
	  // allocate new command
	  temp = command_alloc();
	  c->next = temp;
	  c = temp;
	  c->background = 1;
	  
	  // I think we need to fork a process here...
	 // child_pid = fork();
	  break;
	  
	  // &&
      case 5:
	  temp = command_alloc();
	  c->next = temp;
	  c = temp;
	  
	  // set condition
	  c->cond = 1;
      break;
      
      // || 
      case 6:
	  temp = command_alloc();
	  c->next = temp;
	  c = temp;
	  
	  // set condition
	  c->cond = 2;
	  break;
	  
	  default: 
	  command_append_arg(c, token);
	  break;
	 }

   }
    // execute it; all traversal functions are in run_list. 
    if (head->argc)
        run_list(head);
    
    command_free(c);
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
