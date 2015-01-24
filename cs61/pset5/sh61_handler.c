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
  
  // background
  int wait;
  
  //command connectivity -  connected to last command with 0 - pipe, 1 - ||, 2 - && 
  int connectivity;
  
  // pipes
  int in_fd;
  int out_fd;

  //redirection
  int num_redirect;
  int *redirect_fd;
  char** file_name;
};

int global_var;

// command_alloc()
//    Allocate and return a new command structure.

static command* command_alloc(void) {
    command* c = (command*) malloc(sizeof(command));
    c->argc = 0;
    c->argv = NULL;
    c->pid = -1;
    c->next = NULL;
    c->wait = 1;
    c->connectivity = 0;

    // pipe flags
    c->in_fd = -1;
    c->out_fd = -1;

    //redirection
    c->num_redirect = 0;
    c->redirect_fd = NULL;
    c->file_name = NULL;
    return c;
}


// command_free(c)
//    Free command structure `c`, including all its words.

static void command_free(command* c) {
    for (int i = 0; i != c->argc; ++i)
        free(c->argv[i]);
    free(c->argv);
    for(int i = 0; i != c->num_redirect; ++i)
      free(c->file_name[i]);
    free(c->file_name);
    free(c->redirect_fd);
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

static void command_append_redirect(command* c, char* file, int fd)  {
  c->file_name = (char**) realloc(c->file_name, sizeof(char*) *(c->num_redirect + 1));
  c->file_name[c->num_redirect] = file;
  c->redirect_fd = (int *) realloc(c->redirect_fd, sizeof(int) * (c->num_redirect + 1));
  c->redirect_fd[c->num_redirect] = fd;
  ++c->num_redirect;
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

    
  command* next = c->next;

  // loop
  if(next) {
    // if not a conditional, use pipe logic
    if(next->connectivity == 0) {
  
      
      int pipe_val[2];
      // set up this commmand's pipe array
      pipe(pipe_val);
      // set up the out portal of this pipe
      c->out_fd = pipe_val[1];
      // set up the in portal of this pipe
      next->in_fd = pipe_val[0];
    }
  }

  int pid = -1;
  
  // fork this process
  pid = fork();
  
    /* 
     *  New logic for setting up process group id of child (per instructions above.) 
     */

    // if we are in the child
    if (pid == 0)
    {
       // here the child is calling setpid on itself.
       // set a new pgid for the FIRST child. 
       // for all other children, the pgid of start_command must be the same as 
       // as those in the group. 
       if (pgid == 0) 
       {
        // printf("new pgid is zero\n");
         setpgid(0, 0);
       }
       else
         setpgid(pid, pgid);
    }
 
    // set parent is calling setpid on the child.
    // Hey, HB: doing this in right place? are we back in the parent now? 
    setpgid(pid, pid);
       
  // if not in child process, close any pipes we are using                                                                                                                                  
  if(pid != 0) {
    if(c->out_fd != -1)
      close(c->out_fd);
    if(c->in_fd != -1)
      close(c->in_fd);
  }
  else {
    //redirection
    int file_fd =-1;
    for(int i = 0; i < c->num_redirect; ++i)  {
      if(c->redirect_fd[i] == 0)
	file_fd =open(c->file_name[i], O_RDONLY);
      else
	file_fd =open(c->file_name[i], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);

      if(file_fd == -1) {
	printf("%s ", strerror(errno));
	exit(1);
      }
      else  {
	dup2(file_fd, c->redirect_fd[i]);
	close(file_fd);
      }
    }
     
    // if write end is to be piped, run the dup2 
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
  
 
  // i think this is actually returning the pid. 
  return pid;
}

/*
You should never need to get the group id. 
 
 

In your code, you'll want to put set_foreground in run_list—
   a) once before start_command,
   b) and once after waitpid to restore the foreground after completion of a pipeline.
 
No need to call kill! Once you set_foreground with the right process group ID, every variable in that process group will receive a SIGINT upon a Ctrl-C, courtesy of the terminal running sh61.
 
how will shell observe that pipeline dies because of SIGINT?

WIFSIGNALED(status) && WTERMSIG(status) == SIGINT

to catch if SIGINT happened, see handle_signal in sh61.h. 

to print another prompt, see the needprompt variable.
 
Immediately after calling fork, you want to make sure the child's process group ID is set appropriately. 

You don't know whether control will return to the child or the parent first, so you need to call setpgid in BOTH the child and the parent. The parent calls setpgid on the child; the child calls setpgid on itself. That way, you're covered no matter which starts executing first.

**********************************************

Notice how start_command takes a second argument, pgid? Basically, given a pipeline like the following

    echo 1 | echo 2 | echo 3

call start_command("echo 1", 0).

In the child, call setpgid(0, 0) to give the child process a new group, with the same ID as its PID.
 
In the parent, call setpgid(child_pid, child_pid) to do the same.
  
Then, call start_command("echo 2", echo_1_pid) and start_command("echo 3", echo_1_pid). Make sure start_command properly sets the process group ID of those commands to echo_1_pid!

Finally, after waitpid returns, call set_foreground(0) to make sh61 the foreground process group again. Also you should check WTERMSIG(status) == SIGINT and abort execution of the remainder of the command line if so. 

***********************************************

 
*/
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
//
//    PART 8: - Choose a process group for each pipeline.
//       - Call `set_foreground(pgid)` before waiting for the pipeline.
//       - Call `set_foreground(0)` once the pipeline is complete.
//       - Cancel the list when you detect interruption.

void run_list(command* c) {

  //exit status
  int exit_status = 0;
  
  // i am setting an initial pid of zero. 
  pid_t pid = 0;

  //iterating over all commands
  while(c)
    {
      //getting next command
      command* next = c->next;

      //skipping command based on connectivity
      if(((c->connectivity == 1) && exit_status) || ((c->connectivity == 2) && !exit_status))
	{
	  c = c->next;
	  continue;
	}

      //cd logic                                                                                                                                                                      
      if(strcmp(c->argv[0], "cd") == 0) {                                                                                                                                                  
        int status = chdir(c->argv[1]);                                                                                                                                                      
        if(status == -1)                                                                                                                                                                     
          exit_status = 1;                                                                                                                                                                   
        else if(status == 0)                                                                                                                                                                 
          exit_status = 0;                                                                                                                                                                   
        c = c->next;                                                                                                                                                                         

       continue;   

                                                                                                                                                                                
      }
      
      if (strcmp(c->argv[0], "exit") == 0)
       exit(0);
      
      
      //commenting this out because I think pid should start out zero (right?) 
     // pid_t pid = -1;
      
      // setting foreground with current pid. this is 0 first, then
      // becomes the child pid after the first run through a pipe. 
      
  //    printf("pid: %i\n", pid);
      
      set_foreground(pid);
      
      
      // maybe setpgid here? 
      
      // I'm assuming that we pass pid into start_command. 
      // it is first 0, and then it is the pid of the child. 
      pid = start_command(c, pid);

      // calling waitpid only for conditionals and last command
      // the wait variable is used for making sure it is not a background process
      
      if(((next && (next->connectivity != 0)) || (next == NULL)) && c->wait)
	  {
	    int status;
	    waitpid(pid, &status, 0);
	    if(WIFEXITED(status)) 
	    {
	      exit_status = WEXITSTATUS(status);
	    }
	    
	    // we are checking on waitpid status, so I think it goes here. 
	    // but is it working???
	    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)
	    {
	      
	      global_var = 1;
	      printf("global: %i\n", global_var);
          break;
         
	    }
	    else
	      global_var = 0;
	    // pipeline complete (right?) restore foreground.
	    set_foreground(0);
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
    
    // establish the head of our command chain.
    command* head = c;
    
    while ((s = parse_shell_token(s, &type, &token)) != NULL) {

    // token redirection logic.
    if(type == TOKEN_REDIRECTION) 
    {
	  char* file;
	  int fd;
	  if(strlen(token) > 1) {
	  fd = 2;
	}
	else {
	  if(strcmp(token, ">") == 0)
	    fd = 1;
	  else if(strcmp(token, "<") == 0)
	    fd = 0;
	}
	s = parse_shell_token(s, &type, &file);
	command_append_redirect(c, file, fd);
	continue;
      }
      int pid = 0;
      command* temp;
      switch (type) 
      {
        // & - background this process.
        case 3:
         // fork a background process. 
	     pid = fork();
	  
	     // if head exists, call run_list(head). 
	     if(pid == 0)
	     {
	       c->wait = 0;
	       if(head->argc)
	         run_list(head);
	         exit(0);	    
	     }
	     // allocate new pointer; set head and c to this new pointer. (needed?)
	    
	        temp = command_alloc();
	        head = temp;
	        c = temp;
	     
        break;
        // at semicolon, run the previous command(s) and then initiate a new command.
        case 2: 
          if(head->argc)
	        run_list(head);
	        temp = command_alloc();
	        head = temp;
	        c = temp;
        break;
   
        /*
         *  Connectivity Cases
         */
         
        // pipe
        case 4: 
          temp = command_alloc();
	      c->next = temp;
	      c = temp;
        break;
        // &&
        case 5: 
           temp = command_alloc();
	       c->next = temp;
	       c = temp;
	       c->connectivity = 1;
        break;
        // || 
        case 6: 
           temp = command_alloc();
	       c->next = temp;
	       c = temp;
	       c->connectivity = 2;
        break;
        case 'x':
        exit(0);
        break;
        default: 
           command_append_arg(c, token);
        break;

      }
   
	}

    // execute it
    if (head->argc)
        run_list(head);
    
    //command_free(c);
    command_chain_free(c);
}

/* 
 *  Signal handler that clears the space and prints a new prompt. 
 *  
 */


void interrupt_handler (int sig) 
{
  (void) sig;
  global_var = 1;
  printf("global var from handler: %i\n", global_var);
  printf("\rsh61[%d]$                            \n", getpid());
  printf("sh61[%d]$ ", getpid());
  fflush(stdout);
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
    
    // This triggers the interrupt handler!!!!!!!
    
    int h = handle_signal(SIGINT, interrupt_handler);
    assert (h >=0);
   
    char buf[BUFSIZ];
    int bufpos = 0;
    
    // print another prompt
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
      
       // zombies
       while ((pid = waitpid(-1, &status, 0)) > 0)
         if (WIFEXITED(status)) WEXITSTATUS(status);
         
          
          // what type of interrupt logic do we put here? 
       
         
      
       
    }

    return 0;
}


/*INTERRUPT STEPS:
All processes in each pipeline must have the same process group (see below).

setpgid(pid, pgid) sets process pid’s process group to pgid. Process groups use the same ID space as process IDs, so you’ll often see code like setpgid(pid, pid). 

NK: This is being done in start_command currently. 

Your shell should use the set_foreground function to inform the OS about the currently active foreground pipeline.

NK: Added some initial logic for this in start_command as well.

If the user presses Control-C while the shell is executing a foreground pipeline, every process in that pipeline will receive the SIGINT signal. This will kill them.

If the shell observes that a pipeline died because of SIGINT, it should stop evaluating the command line.

If SIGINT is received at another time, your shell should clear the current partial command line and print another prompt.
*/
