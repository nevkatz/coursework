#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include "serverinfo.h"
#include <math.h>


// NEXT UP: Simplify linked list and test it. 
// Make it a singly linked list that operates as a stack. You may not even need a struct for the list itself, just a list_start.  

// a conn gets added as the head when it gets done
// a conn gets removed when it is picked up for use again.

// constants for host, port, and user. 
static const char* pong_host = PONG_HOST;
static const char* pong_port = PONG_PORT;
static const char* pong_user = PONG_USER;
static struct addrinfo* pong_addr;



// NK: max number of threads at a time!
int n_connection_threads = 0;
int n_waiting_threads = 0;
int connection_down = 0;
int n_requests = 0;
int n_conns = 0;
int pushing = 0;
int popping = 0;
int pong_created = 0;
int pongs_sent = 0;
int index_sent = 0;
int sending = 0;

// TIME HELPERS
double elapsed_base = 0;

// timestamp()
// Return the current absolute time as a real number of seconds.
double timestamp(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec + (double) now.tv_usec / 1000000;
}

// elapsed()
//    Return the number of seconds that have elapsed since `elapsed_base`.
double elapsed(void) {
    return timestamp() - elapsed_base;
}


// HTTP CONNECTION MANAGEMENT

// http_connection
//    This object represents an open HTTP connection to a server.
typedef struct http_connection http_connection;
struct http_connection {
    int fd;                 // Socket file descriptor

    int state;              // Response parsing status (see below)
    int status_code;        // Response status code (e.g., 200, 402)
    size_t content_length;  // Content-Length value
    int has_content_length; // 1 iff Content-Length was provided
    int eof;                // 1 iff connection EOF has been reached

    char buf[BUFSIZ];       // Response buffer
    size_t len;             // Length of response buffer  
    
    http_connection* next;  // linked list properties
    http_connection* prev;
};


typedef struct conn_list conn_list;
struct conn_list {
  int count;
  http_connection* first;
  http_connection* last;
};

conn_list *my_conn_list;
// `http_connection::state` constants
#define HTTP_REQUEST 0      // Request not sent yet
#define HTTP_INITIAL 1      // Before first line of response
#define HTTP_HEADERS 2      // After first line of response, in headers
#define HTTP_BODY    3      // In body
#define HTTP_DONE    (-1)   // Body complete, available for a new request
#define HTTP_CLOSED  (-2)   // Body complete, connection closed
#define HTTP_BROKEN  (-3)   // Parse error

// thread constants
#define MAX_THREADS 30
#define MAX_CONNECTIONS 25
#define MAX_REQ 6

// sleep constants
#define DIS_CONST   10000 // 10000
#define DIS_MAX    100000 

// helper functions
char* http_truncate_response(http_connection* conn);
static int http_process_response_headers(http_connection* conn);
static int http_check_response_body(http_connection* conn);

static void usage(void);


// add to list

int push_to_list (http_connection* conn)
{
  pushing++;
  if (my_conn_list->count == 0)
  {
    my_conn_list->first = conn;
    my_conn_list->last = conn;
  }
  else
  {
    http_connection* temp = my_conn_list->last;
    temp->next = conn;
    conn->prev = temp;
  }
  
  my_conn_list->last = conn;
  my_conn_list->count++;
  
  pushing--;
  
  return 0;
}

// remove from list
int pop_from_list (http_connection* conn)
{

  // if not in list, leave. 
  if ((conn->prev == NULL
   && conn->next == NULL 
   && my_conn_list->first != conn) || (my_conn_list->count == 0))
    return 0;
  
  // lock out other threads by making "popping" variable over zero.
   popping++;

  if (my_conn_list->count == 1) {
    my_conn_list->first = NULL;
    my_conn_list->last = NULL;
  }
  else if (my_conn_list->last == conn && my_conn_list) // if last in list
  {
    http_connection* new_last = conn->prev;
    new_last->next = NULL;
    conn->prev = NULL;
    my_conn_list->last = new_last;
  }
  else if (my_conn_list->first == conn) // if first in list
  {
    http_connection* new_first = conn->next;
    new_first->prev = NULL;
    conn->next = NULL;
    my_conn_list->first = new_first;
  }
  else // if somewhere in the middle
  {
   http_connection* prev_conn = conn->prev;
   http_connection* next_conn = conn->next;

   // not sure if we should need the if's here. 
   if (next_conn && prev_conn) prev_conn->next = next_conn;
   if (prev_conn && prev_conn) next_conn->prev = prev_conn;
   
   conn->next = NULL;
   conn->prev = NULL;
  }
  my_conn_list->count--;
  popping--;
  return 0;
}

// remove from list
http_connection* recycle_connection ()
{
   if (my_conn_list->count > 0)
   {
     http_connection* myconn = my_conn_list->first;
     
     if (my_conn_list->count > 1)
     {
       http_connection* mynext = myconn->next;
       my_conn_list->first = mynext;
       mynext->prev = NULL;
     }
     else
       my_conn_list->first = NULL; //malloc(sizeof(http_connection));
       
     my_conn_list->count--;
     return myconn;   
   }
   else 
     return NULL;
}

// http_connect(ai)
//    Open a new connection to the server described by `ai`. Returns a new
//    `http_connection` object for that server connection. Exits with an
//    error message if the connection fails.
http_connection* http_connect(const struct addrinfo* ai) {

    // connect to the server
    http_connection* conn;
    
    // if we can, let's recycle a new connection
    if (my_conn_list->count > 0 || n_conns >= MAX_CONNECTIONS)
    {
      while (my_conn_list->count == 0) usleep(1);
      
      conn = recycle_connection();
      return conn;
    }
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    int yes = 1;
    (void) setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    int r = connect(fd, ai->ai_addr, ai->ai_addrlen);
    if (r < 0) {
        perror("connect");
        exit(1);
    }

    // construct an http_connection object for this connection
    conn = (http_connection*) malloc(sizeof(http_connection));

    conn->fd = fd;
    conn->state = HTTP_REQUEST;
    conn->eof = 0;
    
    // new
    conn->next = NULL;
    conn->prev = NULL;
   
    n_conns++;
    
    return conn;
}

// when I call close here, do I close a thread as well? 

// http_close(conn)
//    Close the HTTP connection `conn` and free its resources.
void http_close(http_connection* conn) {
    close(conn->fd);
    free(conn);
    
    // decrement current number of connections.
    n_conns--;
}


// http_send_request(conn, uri)
//    Send an HTTP POST request for `uri` to connection `conn`.
//    Exit on error.
void http_send_request(http_connection* conn, const char* uri) {
    assert(conn->state == HTTP_REQUEST || conn->state == HTTP_DONE);

   
    // prepare and write the request
    char reqbuf[BUFSIZ];
    size_t reqsz = sprintf(reqbuf,
                           "POST /%s/%s HTTP/1.0\r\n"
                           "Host: %s\r\n"
                           "Connection: keep-alive\r\n"
                           "\r\n",
                           pong_user, uri, pong_host);
    size_t pos = 0;
    while (pos < reqsz) {
        // write to a spot in the buffer using connection's file descriptor
        ssize_t nw = write(conn->fd, &reqbuf[pos], reqsz - pos);
        if (nw == 0)
            break;
        else if (nw == -1 && errno != EINTR && errno != EAGAIN) {
            perror("write");
            exit(1);
        } else if (nw != -1)
            pos += nw;
    }

    if (pos != reqsz) {
        fprintf(stderr, "%.3f sec: connection closed prematurely\n",
                elapsed());
        exit(1);
    }

    // clear response information
    conn->state = HTTP_INITIAL;
    conn->status_code = -1;
    conn->content_length = 0;
    conn->has_content_length = 0;
    conn->len = 0;
}


// http_receive_response_headers(conn)
//    Read the server's response headers. On return, `conn->status_code`
//    holds the server's status code. If the connection terminates
//    prematurely, `conn->status_code` is -1.
void http_receive_response_headers(http_connection* conn) {
    assert(conn->state != HTTP_REQUEST);
    if (conn->state < 0)
        return;

    // read & parse data until told `http_process_response_headers`
    // tells us to stop
    while (http_process_response_headers(conn)) {
        ssize_t nr = read(conn->fd, &conn->buf[conn->len], BUFSIZ);
        if (nr == 0)
            conn->eof = 1;
        else if (nr == -1 && errno != EINTR && errno != EAGAIN) {
            perror("read");
            exit(1);
        } else if (nr != -1)
            conn->len += nr;
    }

    // Status codes >= 500 mean we are overloading the server
    // and should exit.
    if (conn->status_code >= 500) {
        fprintf(stderr, "%.3f sec: exiting because of "
                "server status %d (%s)\n", elapsed(),
                conn->status_code, http_truncate_response(conn));
        exit(1);
    }
}


// http_receive_response_body(conn)
//    Read the server's response body. On return, `conn->buf` holds the
//    response body, which is `conn->len` bytes long and has been
//    null-terminated.
void http_receive_response_body(http_connection* conn) {
    assert(conn->state < 0 || conn->state == HTTP_BODY);
    if (conn->state < 0)
        return;

    // read response body (http_check_response_body tells us when to stop)
    while (http_check_response_body(conn)) {
        // read info into a buffer
        ssize_t nr = read(conn->fd, &conn->buf[conn->len], BUFSIZ);
        
        // if at end of file
        if (nr == 0)
            conn->eof = 1;
            
        // handle errors
        else if (nr == -1 && errno != EINTR && errno != EAGAIN) {
            perror("read");
            exit(1);
        // if amt read is positive, keep reading
        } else if (nr != -1)
            conn->len += nr;
    }
    
    // should I somehow see how long this takes and spawn a new thread if
    // it takes too long? 
    
    // null-terminate body
    conn->buf[conn->len] = 0;
}


// http_truncate_response(conn)
//    Truncate the `conn` response text to a manageable length and return
//    that truncated text. Useful for error messages.
char* http_truncate_response(http_connection* conn) {
    char *eol = strchr(conn->buf, '\n');
    if (eol)
        *eol = 0;
    if (strnlen(conn->buf, 100) >= 100)
        conn->buf[100] = 0;
    return conn->buf;
}


// MAIN PROGRAM

// add a property to pong_args for the sequence!
typedef struct pong_args {
    int x;
    int y;
    int index;
    int sent;
} pong_args;

pthread_mutex_t mutex;
pthread_cond_t condvar;

// pong_thread(threadarg)
//    Connect to the server at the position indicated by `threadarg`
//    (which is a pointer to a `pong_args` structure).
void* pong_thread(void* threadarg) {
    
    // no point in proceeding if the connection is already down.
   
    
    int disconnects = 0;
    int success = 0;
   
    pthread_detach(pthread_self());

    // Copy thread arguments onto our stack.
    pong_args pa = *((pong_args*) threadarg);
    
    char url[256];
    snprintf(url, sizeof(url), "move?x=%d&y=%d&style=on",
             pa.x, pa.y);
    
    while (connection_down == 1) 
      usleep(1);
      
    http_connection* conn = http_connect(pong_addr);
    
    while ((pa.index - index_sent) > 1 || sending == 1)
     {
                 printf("*");
                 usleep(1);
     } 
    sending = 1;
    http_send_request(conn, url);
    sending = 0;
    n_requests++;
    
    http_receive_response_headers(conn);
    if (conn->status_code != 200)
    {
        fprintf(stderr, "%.3f sec: warning: %d,%d: "
                "server returned status %d (expected 200)\n",
                elapsed(), pa.x, pa.y, conn->status_code);
                
      // detect loss of connection
      while (conn->state == HTTP_BROKEN || conn->status_code == -1) 
      {
        printf("failed\n");
        connection_down = 1;
 
        // close that connection (to free its resources), but 
        // delay closure until no pops are taking place. 
        while (popping > 0) usleep (1);
        
        pop_from_list(conn);
        
        http_close(conn);
        
        // exponential backoff
        int multiplier = pow(2,n_requests);
      
        // multiply by our constant
        int sleeptime = multiplier*DIS_CONST;

        if (sleeptime > DIS_MAX) sleeptime = DIS_MAX;
      
        // sleep for that amount of time
        usleep(sleeptime);
      
        // send over the coordinates as a query string. 
        snprintf(url, sizeof(url), "move?x=%d&y=%d&style=on",
             pa.x, pa.y);
         
        // and make a new connection attempt at the same position.
        conn = http_connect(pong_addr);
        
        int n = pa.index - index_sent;
        while ((pa.index - index_sent) > 1 || sending == 1)
               {
                 printf("*");
                 usleep(1);
               } 
        printf("!\n");
        sending = 1;
        http_send_request(conn, url);
        sending = 0;
        n_requests++;
        
  //      printf("%.3f sec: %i requests; this one from %d,%d\n",elapsed(), n_requests, pa.x, pa.y);
      
        // receive the response
        http_receive_response_headers(conn);
       
        disconnects++;
        
       
  
      }
 //   printf("%.3f sec: back online with %d,%d\n",elapsed(), pa.x, pa.y);
    connection_down = 0;
  
    }

    printf("sent pong %i", pa.index);
    if (pa.sent == 1) printf("REPEAT!\n");
    pa.sent = 1;
    if (pa.index != index_sent + 1) printf("--RACE CONDITION!\n");
       else printf("\n");
    pongs_sent++;
    index_sent = pa.index;
    n_requests = 0;

    http_receive_response_body(conn);
    
    // printf("returned from receive response body\n");
    double result = strtod(conn->buf, NULL);
    if (result < 0) {
        fprintf(stderr, "%.3f sec: server returned error: %s\n",
                elapsed(), http_truncate_response(conn));
        exit(1);
    }

    // not closing the connection because we want to store DONE connections.

    // signal the main thread to continue
    pthread_cond_signal(&condvar);
    // and exit!
    
    // decrement the number of connection threads.
    n_connection_threads--;
  
    pthread_exit(NULL);
}


// usage()
//    Explain how pong61 should be run.
static void usage(void) {
    fprintf(stderr, "Usage: ./pong61 [-h HOST] [-p PORT] [USER]\n");
    exit(1);
}


// main(argc, argv)
//    The main loop.
int main(int argc, char** argv) {
    // parse arguments
    int ch, nocheck = 0;
    
    // NEW: Declare connections list
    my_conn_list = malloc(sizeof(conn_list));
    my_conn_list->first = NULL; //malloc(sizeof(http_connection));
    my_conn_list->last = NULL;
    my_conn_list->count = 0;
    
    // what is getopt? 
    while ((ch = getopt(argc, argv, "nh:p:u:")) != -1) {
        if (ch == 'h')
            pong_host = optarg;
        else if (ch == 'p')
            pong_port = optarg;
        else if (ch == 'u')
            pong_user = optarg;
        else if (ch == 'n')
            nocheck = 1;
        else
            usage();
    }
    if (optind == argc - 1)
        pong_user = argv[optind];
    else if (optind != argc)
        usage();

    // look up network address of pong server
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    int r = getaddrinfo(pong_host, pong_port, &hints, &pong_addr);
    if (r != 0) {
        fprintf(stderr, "problem looking up %s: %s\n",
                pong_host, gai_strerror(r));
        exit(1);
    }

    // reset pong board and get its dimensions
    int width, height;
    {
        http_connection* conn = http_connect(pong_addr);
        http_send_request(conn, nocheck ? "reset?nocheck=1" : "reset");
        http_receive_response_headers(conn);
        http_receive_response_body(conn);
        if (conn->status_code != 200
            || sscanf(conn->buf, "%d %d\n", &width, &height) != 2
            || width <= 0 || height <= 0) {
            fprintf(stderr, "bad response to \"reset\" RPC: %d %s\n",
                    conn->status_code, http_truncate_response(conn));
            exit(1);
        }
        
        // added by NK
        pop_from_list(conn);
        
        http_close(conn);
    }
    // measure future times relative to this moment
    elapsed_base = timestamp();

    // print display URL
    printf("Display: http://%s:%s/%s/%s\n",
           pong_host, pong_port, pong_user,
           nocheck ? " (NOCHECK mode)" : "");

    // initialize global synchronization objects
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condvar, NULL);

    // play game
    int x = 0, y = 0, dx = 1, dy = 1;
    char url[BUFSIZ];
    while (1) {
        // create a new thread to handle the next position
        pong_args pa;
        pa.x = x;
        pa.y = y;
        pa.sent = 0;
        
        // new
        pong_created++;
        pa.index = pong_created;
        printf("created pong %i\n", pong_created);
        pthread_t pt;
        
        // pong_thread is called in the pthread_create function.
        r = pthread_create(&pt, NULL, pong_thread, &pa);
        if (r != 0) {
            fprintf(stderr, "%.3f sec: pthread_create: %s\n",
                    elapsed(), strerror(r));
            exit(1);
        }
        else
          n_connection_threads++;
  
        
        // ASSUMPTION: All commands between the two mutex lines are atomic.
        // wait until that thread signals us to continue
        pthread_mutex_lock(&mutex);
        
     //   printf("index: %i sent: %i\n", pa.index, pongs_sent);
        // added condition that checks for our current # of threads
    //    while (n_connection_threads >= MAX_THREADS || n_requests > 0 || connection_down == 1 || pa.index - pongs_sent > 1 )
          if (n_connection_threads >= MAX_THREADS || n_requests > 0 || connection_down == 1 || pa.index - pongs_sent > 1)
            pthread_cond_wait(&condvar, &mutex);
        
        // release the lock
        pthread_mutex_unlock(&mutex);
        

        // update position
        x += dx;
        y += dy;
        
        // bounce if horizontally at edge
        if (x < 0 || x >= width) {
            dx = -dx;
            x += 2 * dx;
        }
        // bounce if vertically at edge
        if (y < 0 || y >= height) {
            dy = -dy;
            y += 2 * dy;
        }
        // wait 0.1sec
        usleep(100000);
    }
}

// HTTP PARSING

// http_process_response_headers(conn)
//    Parse the response represented by `conn->buf`. Returns 1
//    if more header data remains to be read, 0 if all headers
//    have been consumed.
static int http_process_response_headers(http_connection* conn) {
    size_t i = 0;
    while ((conn->state == HTTP_INITIAL || conn->state == HTTP_HEADERS)
           && i + 2 <= conn->len) {
        if (conn->buf[i] == '\r' && conn->buf[i+1] == '\n') {
            conn->buf[i] = 0;
            if (conn->state == HTTP_INITIAL) {
                int minor;
                if (sscanf(conn->buf, "HTTP/1.%d %d",
                           &minor, &conn->status_code) == 2)
                    conn->state = HTTP_HEADERS;
                else
                    conn->state = HTTP_BROKEN;
            } else if (i == 0)
                conn->state = HTTP_BODY;
            else if (strncmp(conn->buf, "Content-Length: ", 16) == 0) {
                conn->content_length = strtoul(conn->buf + 16, NULL, 0);
                conn->has_content_length = 1;
            }
            memmove(conn->buf, conn->buf + i + 2, conn->len - (i + 2));
            conn->len -= i + 2;
            i = 0;
        } else
            ++i;
    }
    if (conn->eof)
        conn->state = HTTP_BROKEN;
    return conn->state == HTTP_INITIAL || conn->state == HTTP_HEADERS;
}


// http_check_response_body(conn)
//    Returns 1 if more response data should be read into `conn->buf`,
//    0 if the connection is broken or the response is complete.
static int http_check_response_body(http_connection* conn) {
  //  printf("check response body pushing: %i conn: %p state: %i\n", pushing, (void*) conn, conn->state);
    if (conn->state == HTTP_BODY
        && (conn->has_content_length || conn->eof)
        && conn->len >= conn->content_length)
    {
        conn->state = HTTP_DONE; 
        while (pushing > 0) usleep(1);

        if (pushing == 0) push_to_list(conn);
    }
    if (conn->eof && conn->state == HTTP_DONE)
        conn->state = HTTP_CLOSED;
    else if (conn->eof)
        conn->state = HTTP_BROKEN;
    return conn->state == HTTP_BODY;
}
