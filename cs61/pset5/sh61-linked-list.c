#include "sh61.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*
STEP 2: Support background commands, such as sleep 1 &, which the shell runs without waiting for them to complete. This will require changes to eval_line (to detect control operators) and run_list, as well as, most likely, to struct command.
*/
// struct command
//    Data structure describing a command. Add your own stuff.


typedef struct command command;
struct command {
    int argc;      // number of arguments
    int added;
    char** argv;   // arguments, terminated by NULL
    pid_t pid;     // process ID running this command, -1 if none
    void* prev;
    void* next;
    
    // might come in handy
    int background;
};



typedef struct command_list command_list;
struct command_list {
    void* first;
    void* last;
    int count;
};


// command_alloc()
//    Allocate and return a new command structure.

static command* command_alloc(void) {
    command* c = (command*) malloc(sizeof(command));
    c->argc = 0;
    c->argv = NULL;
    c->pid = -1;
    c->background = 0;
    c->added = 0;
    c->prev = NULL;
    c->next = NULL;
    return c;
}
static command_list* command_list_alloc(void) {
   // initializers go here
   command_list* cl = (command_list*) malloc(sizeof(command_list));
   cl->last = NULL;
   cl->first = NULL;
   cl->count = 0;
   return cl;
   
}
int add_to_list (command_list* list, command* com)
{
  printf("COMMAND: ");
  for (int i = 0; i < com->argc; i++)
  {
    printf("**%s\n", com->argv[i]);
  }
  if (list->count == 0)
    list->first = com;    
  else
  {
    command* my_last = list->last;
    my_last->next = com;
    com->prev = my_last;
  }
  list->last = com;
  com->added = 1;
  list->count++;

  return 0;
}
int reset_command (command* c)
{
  c->argc = 0;
  c->argv = NULL;
  c->pid = -1;
  return 0;
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

    pid_t my_child = fork();
    
    if (my_child == 0)
    {
      execvp(c->argv[0], c->argv);
    }
    
    //fprintf(stderr, "start_command not done yet\n");
    return c->pid;
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
//    ***
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
    pid_t p = start_command(c, 0);
    //fprintf(stderr, "run_command not done yet\n");
    
    int status;
    
    waitpid(p, &status, c->background);
   

    
    // how to treat background commands differently? 
}


// eval_line(c)
//    Parse the command list in `s` and run it via `run_list`.

void eval_line(const char* s) {
    int type;
    char* token;
    // Your code here!

    // build the command
    // instantiate a new command
    command* c = command_alloc();
    command_list* my_list = command_list_alloc();
 
    while ((s = parse_shell_token(s, &type, &token)) != NULL)
    {
            
              pid_t child_pid = -1;
              (void) child_pid;
              switch (type) 
              {
                // < > 2>
                case 1: 
                break;
                
      
                
                // ; 
                case 2:  
                
                // do we need this? 
                
                add_to_list(my_list, c);
                c = command_alloc();

                break;
                
                // & 
                case 3:
                c->background = 1;
                run_list(c);
                reset_command(c);
                 
                child_pid = fork(); 

                break;
                
                // | 
                case 4:
                break;
                
                // && parallel
                case 5:
                child_pid = fork();
                c->background = 1;
                // what to do with fork() now that we have a parallel process?
                break;
                
                // ||
                case 6: 
                break;
                
                default:
                command_append_arg(c, token);
                if (c->added == 0)
                  add_to_list(my_list, c);
                break;
              }
        
        }
        
   
     
      command* current_command = my_list->first;
      if (my_list->count == 1)
      {
        printf("single command: %s\n", c->argv[0]);
        
        if (c->argc)
          run_list(c);
      }
      else
      {
       printf("multiple items in list\n");
       for (int i = 0; i < my_list->count; ++i)
       {
          if (current_command->argc)
            run_list(current_command);
        
          if (my_list->last != current_command)
          {
            command* mynext = current_command->next;
        
            current_command = mynext;
          }
          // start at first initially;
          // if not at first, set it equal to command_prev
        }
      
      
      
      //run_list(c);
    }
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

    return 1;
}
