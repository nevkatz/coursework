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
  int cond;
  
  // pipes
  int in_fd;
  int out_fd;
  int pipedio[2];
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
    // pipe flags
    c->in_fd = -1;
    c->out_fd = -1;
    // array for file descriptors
    c->pipedio[0] = -1;
    c->pipedio[1] = -1;
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

static void command_chain_free  (command* c) {
    while (c) 
    {
      command* temp = c->next;
      command_free(c);
      c = temp;
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

  command* next = c->next;
  
  // we would **not** use pipes if this next were NULL
  if(next) {
    // we are in this loop if we are dealing with a series of commands.
    // if not a conditional, use pipe logic
    if(next->cond == 0) {
      // set up this commmand's pipe array
      pipe(c->pipedio);
      // set up the out portal of this pipe
      c->out_fd = c->pipedio[1];
      // set up the in portal of this pipe
      next->in_fd = c->pipedio[0];
    }
  }

  int pid = -1;
  
  // fork this process
  pid = fork();
  
  // if we are in a child process
  if(pid == 0) {
     
    // if write end is to be piped, run the dup2 (are we writing to STDOUT?)
    if(c->out_fd != -1)
      dup2(c->out_fd, 1);
      
    // if read end is to be piped, run dup2
    if(c->in_fd != -1)
      dup2(c->in_fd, 0);

    // if pipe, close write end.
    if(c->out_fd != -1) 
    close(c->out_fd);
   
    // if pipe, close read end.
    if(c->in_fd != -1)
    close(c->in_fd);
    
    // call execvp
    const char* file = (const char *) c->argv[0];
    char **argv = &(c->argv[0]);
    execvp(file, argv);
  }
  // if not in child process, close any pipes we are using
  else {
    if(c->out_fd != -1)
      close(c->out_fd);
    if(c->in_fd != -1)
       close(c->in_fd);
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

/* 
 *  Here we are executing whatever commands have been stored up. 
 */
void run_list(command* c) {
  int exit_status = 0;

  // we will go through all the commands until we reach NULL.
  while(c)
    {
      command* next = c->next;
      if((c->cond == 1) && exit_status)
	{
	  c = c->next;
	  continue;
	}
      else if((c->cond == 2) && !exit_status)
	{
	  c = c->next;
	  continue;
	}
      
      pid_t pid = -1;
      
      // call start_command, which in turn execvp
      pid = start_command(c, 0);
      if(next){
	if(next->cond != 0)
	  {
	    int status;
	    waitpid(pid, &status, 0);
	    if(WIFEXITED(status)) {
	      exit_status = WEXITSTATUS(status);
	    }
	  }
      }
      if(next == NULL){
	int status;
	waitpid(pid, &status, 0);
	if(WIFEXITED(status)) {
	  exit_status = WEXITSTATUS(status);
	}
      }

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
    while ((s = parse_shell_token(s, &type, &token)) != NULL) {

      if(strcmp("&", token) == 0)
	 {
	  // fork a background process. 
	  int pid = fork();
	  
	  // if head exists, call run_list(head). 
	  if(pid == 0) {
	    if(head->argc)
	      run_list(head);
	    exit(0);	    
	  }
	  // allocate new pointer; set head and c to this new pointer.
	  else {
	    command* temp = command_alloc();
	    head = temp;
	    c = temp;
	    continue;
	  }
	}
	// at semicolon, run the previous command(s) and then initiate a new command.
    else if(strcmp(";", token) == 0)
	{
	  if(head->argc)
	    run_list(head);
	  command* temp = command_alloc();
	  head = temp;
	  c = temp;
	  continue;
	}
	// set positive condition
      else if(strcmp("&&", token) == 0)
	{
	  command* temp = command_alloc();
	  c->next = temp;
	  c = temp;
	  c->cond = 1;
	  continue;
	}
	// set negation condition
      else if(strcmp("||", token) == 0)
	{
	  command* temp = command_alloc();
	  c->next = temp;
	  c = temp;
	  c->cond = 2;
	  continue;
	}
	// move on to next command in linked list. 
      else if(strcmp("|", token) == 0)
	{
	  command* temp = command_alloc();
	  c->next = temp;
	  c = temp;
	  continue;
	}
	// add this element to our comand
      command_append_arg(c, token);
    }

    // execute it, call run_list.
    if (head->argc)
        run_list(head);
    
    //command_free(c);
    command_chain_free(c);
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
       int status = 0;
       pid_t pid;
      
       while ((pid = waitpid(-1, &status, 0)) > 0)
         if (WIFEXITED(status)) WEXITSTATUS(status);
    
    }

    return 0;
}
