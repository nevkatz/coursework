#include "sh61.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

//variable to kill process at ctrl c
sig_atomic_t kill_process = 0;

// struct command
//    Data structure describing a command. Add your own stuff.

// history functionality

typedef struct string_node string_node;
struct string_node {
  char* str;
  string_node* next;
};


typedef struct string_list string_list;
struct string_list {
  int max;
  int count;
  string_node* first;
  string_node* last;
};

string_list* history_list;

// prints out full history
int print_history()
{
  string_node* cur_node = history_list->first;
  for (int i = 0; i < history_list->count; ++i)
  { 
    int n = i+1;
    printf("%i %s", n, cur_node->str); 
    string_node* temp = cur_node->next;
    cur_node = temp;
  }
 return 0;
}
// add to history (maybe add a clear history?)
int push_to_history (char buf[BUFSIZ])
{
  
  // allocate new string node
  string_node* new_node = malloc(sizeof(string_node));
 
  // alloc string
  new_node->str = malloc(BUFSIZ);
  
  memcpy(new_node->str, buf, BUFSIZ);
    
  if (history_list->count == history_list->max)
  {
    // set head equal to first el in list
    string_node* head = history_list->first; 
    
    // forget about original head
    // make new list start the next one
    history_list->first = head->next;
    
    free (head);
    
  }
  else 
    ++history_list->count;
  
  // if list was empty
  if (history_list->count == 1) {
    history_list->first = new_node;
    history_list->last = new_node;
  }
  else {
    
    
    // set tail equal to last el
    string_node* tail = history_list->last;

    // set next pointer equal to new node
    tail->next = new_node;

    // set last el in hist list to new node
    history_list->last = new_node;

  }

  return 0;
}
// end history functionality



typedef struct command command;
struct command {
  int argc;      // number of arguments
  char** argv;   // arguments, terminated by NULL
  pid_t pid;     // process ID running this command, -1 if none
  command * next;
  
  // background
  int wait;
  
  //command connectivity
  // 0 - pipe, 1 - ||, 2 - && 
  int connectivity;
  
  // pipes
  int in_fd;
  int out_fd;

  //redirection
  int num_redirect;
  int *redirect_fd;
  char** file_name;
};


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

// frees an entire command chain.
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

// modifies a command's redirect properties. 
static void command_append_redirect(command* c, char* file, int fd)  
{
  // expand the file name array and put value in next index.
  c->file_name = (char**) realloc(c->file_name, sizeof(char*) *(c->num_redirect + 1));
  c->file_name[c->num_redirect] = file;
  
  // expand the file descriptor array and put file descriptor in next index.
  c->redirect_fd = (int *) realloc(c->redirect_fd, sizeof(int) * (c->num_redirect + 1));
  c->redirect_fd[c->num_redirect] = fd;
  
  // augment the number 
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
      
      //if there is < then no need to set pipe
      int use_redirect = 0;
      for(int i = 0; i < next->num_redirect; ++i) {
	if(next->redirect_fd[i] == 0) {
	  use_redirect = 1;
	  break;
	}
      }
      
      // initialize pipe file descriptor array     
      int pipe_val[2];
      // set up this commmand's pipe array
      pipe(pipe_val);
      // set up the out portal of this pipe
      c->out_fd = pipe_val[1];
      
      // if there is not a "read" redirect,
      // set up the in portal of this pipe; else, close pipe
      if(!use_redirect)
	next->in_fd = pipe_val[0];
      else
	close(pipe_val[0]);      
    }
  }

  int pid = -1;
  
  // fork this process
  pid = fork();
      
  // New logic for setting up process group id of child (per instructions above.)      
  if((pgid != 0) && (pid != 0)){
    setpgid(pid, pgid);
  }
  if(pgid == 0) {  
    if (pid == 0)
      setpgid(0, 0);
    else
      setpgid(pid, pid);
  }
    
  // if not in child process, close any pipes we are using                                                                                                                                 
  if(pid != 0) 
  {
    if(c->out_fd != -1)
      close(c->out_fd);
    if(c->in_fd != -1)
      close(c->in_fd);
  }
  else 
  {
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
      else {
	dup2(file_fd, c->redirect_fd[i]);
	close(file_fd);
      }
    }
    
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
//
//    PART 8: - Choose a process group for each pipeline.
//       - Call `set_foreground(pgid)` before waiting for the pipeline.
//       - Call `set_foreground(0)` once the pipeline is complete.
//       - Cancel the list when you detect interruption.

void run_list(command* c) {

  //exit status
  int exit_status = 0;
  int pgid = 0;
  
  //iterating over all commands
  while(c)
    {
      //getting next command
      command* next = c->next;
      
      //skipping command based on connectivity
      if(((c->connectivity == 1) && exit_status) 
	 || ((c->connectivity == 2) && !exit_status)) {
	c = c->next;
	continue;
      }

      // cd logic 
      if (strcmp(c->argv[0], "cd") == 0) {
	int status = chdir(c->argv[1]);
	if (status == -1)
	  exit_status = 1;
	else if (status == 0)
	  exit_status = 0;
	c = c->next;
	continue;
      }
      
     // debugging
     // if (strcmp(c->argv[0], "exit") == 0)
     // exit(0);
      
      //running command
      int pid = -1;
      pid = start_command(c, pgid);
      
      if(pgid == 0) {
	set_foreground(pid);
	pgid = pid;
      }
      
      // calling waitpid only for conditionals and last command - not for backgrounded p's
      if(((next && (next->connectivity != 0)) || (next == NULL)) && c->wait) {
	int status;
	waitpid(pid, &status, 0);
	if(WIFEXITED(status)) 
	  exit_status = WEXITSTATUS(status);
	
	// checking for interrupts
	if(WIFSIGNALED(status)) {
	  if(WTERMSIG(status) == SIGINT) {
	    kill_process = 1;
	    set_foreground(0);
	    break;  
	  }	  
	}
      }
      
      // return control to foreground
      if((next && (next->connectivity != 0)) || (next == NULL)) {
	pgid = 0;
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
 
    // build the command
    command* c = command_alloc();
    
    // establish the head of our command chain.
    command* head = c;
    
  
    while ((s = parse_shell_token(s, &type, &token)) != NULL) {
      if(kill_process) 
	break;
      
      // token redirection logic.
      if(type == TOKEN_REDIRECTION) {
	char* file;
	int fd = -1;
      
	if(strlen(token) > 1) 
	  fd = 2;
	else 
	  fd = (strcmp(token, ">") == 0) ? 1: 0;  
	s = parse_shell_token(s, &type, &file);
	command_append_redirect(c, file, fd);
	continue;
      }

      int pid = 0;
      command* temp;
      
      // parsing tokens
      switch (type) 
	{
	  // & - background this process.
        case 3:
	  pid = fork();
	  if(pid == 0)
	    {
	      c->wait = 0;
	      if(head->argc)
		run_list(head);
	      exit(0);	    
	    }
	  // since c just got backgrounded, allocate new command
	  temp = command_alloc();
	  head = temp;
	  c = temp;
	  break;
	  
	  // ; - run the previous command(s) and then initiate a new command.
        case 2: 
	  if(head->argc)run_list(head);
	  temp = command_alloc();
	  head = temp;
	  c = temp;
	  break;
	  
	  // Connectivity Cases	  
	  // | (pipe)
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
	  
        default: 
	  command_append_arg(c, token);
	  break;
	 }      
    }
    
    if (head->argv)
    {
      if (strcmp(head->argv[0],"history")==0)
          print_history();
          
       // execute it
      else if (head->argc && !kill_process)
      run_list(head);
    }
          
   
    
    //command_free(c);
    command_chain_free(c);
}


// handler for interrupts
void interrupt_handler (int sig) 
{
  (void) sig;
  printf("\rsh61[%d]$                            \n", getpid());
  printf("\rsh61[%d]$ ", getpid());
  fflush(stdout);
}

int main(int argc, char* argv[]) {
    FILE* command_file = stdin;
    int quiet = 0;
    
    // allocate space for first node of history list
    history_list = malloc(sizeof(string_list));
    history_list->max = 5;
    history_list->count = 0;
    history_list->first = NULL;
    history_list->last = NULL;
  
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
    
    // set handlers
    handle_signal(SIGTTOU, SIG_IGN);
    handle_signal(SIGINT, interrupt_handler);
    char buf[BUFSIZ];
    int bufpos = 0;
    int needprompt = 1;

    while (!feof(command_file)) {
        // Print the prompt at the beginning of the line
      if (needprompt && !quiet) {
      kill_process = 0;
	  printf("\rsh61[%d]$ ", getpid());
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
      
        push_to_history(buf);
        
      
	    eval_line(buf);
	    bufpos = 0;
	    needprompt = 1;
      }
      
      int status = 0;
      pid_t pid;
      
      // zombies
      while ((pid = waitpid(-1, &status, 0)) > 0)
	    if (WIFEXITED(status)) WEXITSTATUS(status);
      
    }

    return 0;
}



