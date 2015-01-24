#define M61_DISABLE 1
#include "m61.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
 
// end code for suffixes & garbage size
#define end_code 7912
#define garbage_size 100000

// how much you increment the number of searches a round can accept every time
#define round_inc 10

// range of counters for round; changed from 30 to 12
#define max_counters_per_round 15

// maximum number of rounds
#define max_rounds 1500

// limits on size and counters per round
#define min_round_searches 2

// for storage, in case we want to limit its # of accepted searches in a later build.
#define max_round_searches 10000

#define max_stored_counters 40

// if not taking data, the probability in each malloc of starting a round is 1/prob_const
#define prob_const 5
#define threshold 10.0

struct m61_statistics; 

static struct m61_statistics mystats = {
0, 0, 0, 0, 0, 0, NULL, NULL
};

// globals (store in a struct?) 

void* current_ptr = NULL;
static int* last_freed= NULL;
int active = 0; // are we actively collecting heavy hitter data? 
uint64_t totalBytes = 0;
uint64_t totalCalls = 0;

typedef struct 
{
  uint64_t my_size;
  uint64_t line;
  uint64_t allocated;
  const char* filename;
  void* next;
  void* prev;
  void* padding; 
} metadata;


// this will be used as a first round, then nw_storage. 
typedef struct List {
  // long long padding;
   void* first;
   void* last;
   int count;
} List;

typedef struct Counter {
   uint64_t calls;
   uint64_t bytes; 
   uint64_t line;
   const char* filename;
   int active;
} Counter;

// a stream is a linked list of rounds; a round is an array of counters. 

typedef struct Round {
   Counter** counters; //[max_counters_per_round];
   int id;
   uint64_t max_searches; // how many data points it will hold
   uint64_t count;
   int max_counters; // how many counters it will hold.
   uint64_t size;
   uint64_t searches;
  
   void* next; 
   void* prev;
} Round;

static struct List *metaList;

static struct List *stream; // linked list of rounds

static struct Round *storage;

/*
 *  Helper functions
 */
 
// adds a pointer to a linked list.
// Used for adding rounds to the stream, and adding metadata pointers to our metaList.
int add_to_list (void* ptr, struct List* myList) {
     
     myList->count++;
     
     if (myList->first == NULL)
     {
      myList->first = ptr;
     }
     else
     {
      // get the previous block address
      metadata* prev_m = (metadata*) myList->last;
      
      // make block point to this one
      prev_m->next = ptr;
      
      metadata* m = ptr;
      // make this block point to the last one
      m->prev = prev_m;
    } 
 
    myList->last = ptr;
    return 0;
}

/*
 *  Increments global stats. 
 */
int incrementStats(size_t sz)
{
   // increment the global size variables.
    mystats.ntotal+=1;
    mystats.nactive+=1;
    mystats.total_size += sz;
    mystats.active_size+= sz;
    return 0;
}

/*
 *  Initializes the properties of a round.
 */
int roundProps (struct Round* myRound, int max_s)
{
  myRound->id= stream->count+1; 
  myRound->max_searches = max_s;
  myRound->count = 0;
  myRound->searches = 0;
  myRound->counters = malloc(max_counters_per_round * sizeof(Counter));
  myRound->next = NULL;
  myRound->prev = NULL;  
  
  return 0;
} 

/*
 *  Initiates the counters in a round.
 */
int roundCounters (struct Round* myRound)
{
    for (int i = 0; i < max_counters_per_round; i++)
    {;
      Counter* myCounter = malloc(sizeof(Counter));
      myRound->counters[i] = myCounter;
      *myCounter = (struct Counter) {0, 0, 0, NULL, 0};
    }
  return 0;
}
/*
 * initialize the metaList (a linked list of pointers to metadata)
 *            the stream (a linked list of rounds for hhtest)  
 *            and storage (where all the counters end up)
 */
int initLists () {
    
       metaList = (struct List*) malloc(sizeof(struct List));       
      *metaList = (struct List) {NULL, NULL, 0};
      
       stream = (struct List*) malloc(sizeof(struct List));
      *stream = (struct List) {NULL, NULL, 0};
      
       storage = (struct Round*) malloc(sizeof(struct Round));
       
      // initialize round properties
      roundProps(storage, max_round_searches);
     
      // initialize the counters within a round
      roundCounters(storage);
    return 0;
}

// initiates a new round when taking heavy hitter data. 
int initRound () {
     
      // we are actively collecting heavy hitter data
      active = 1;
      
      // start round & allocate memory
      struct Round* myRound =  malloc(max_counters_per_round*sizeof(Counter));
       
      // initialize round properties
      uint64_t max_s = min_round_searches + (stream->count)*round_inc; 
      
      roundProps(myRound, max_s);
     
      roundCounters(myRound);
     
      if (stream->count > 0)
      {
        Round* prevRound = stream->last;
        myRound->prev = prevRound;
        prevRound->next = myRound;
      }
      else
        stream->first = myRound;
        
      stream->last = myRound;
      stream->count++;
      return 0;
}

// adds the bytes & calls of a counter with the same file-line pair.
int addToCounter(struct Round *myRound,
                 struct Counter *myCounter,
                 size_t sz, 
               uint64_t calls)
{
     myRound->size += (uint64_t) sz;
     
     myCounter->bytes += (uint64_t) sz;
     myCounter->calls +=calls;
    
     return 0;
}
// transfers the counters in a round to storage.

// end a given round, set data recording to false (active = 0)

int searchRound(struct Round* myRound,
                   int line, 
           const char* filename,
              uint64_t size, 
              uint64_t calls) 
{
  myRound->searches++;
  uint64_t n = max_counters_per_round;
  
  if (myRound!=storage)
  {  
    totalBytes += size;
    totalCalls += calls;
  }
  
  for (uint64_t i = 0; i < n; i++)
  {
    struct Counter *myCounter = myRound->counters[i];
    
    if (myCounter->line == (uint64_t) line
    &&  myCounter->filename == (char*) filename) 
    {
   
        addToCounter(myRound, myCounter, size, calls);
     
      return 0;
    } 
    // if there is an empty counter available, initialize it with m's info.
    else if (myCounter->active ==0
             && myRound->count < n)
    {
      myCounter->active = 1;
      myCounter->filename = filename;
      myCounter->line = line;
      addToCounter(myRound, myCounter, size, calls);
      myRound->count += 1;
      return 0;
    }
  }
  
  return 1;
}
int to_storage(struct Round* myRound)
{
  int n = myRound->count;

  for (int k = 0; k < n; k++)   
  {
    struct Counter* myCounter = myRound->counters[k];

    if (myCounter->active > 0)
    {
    searchRound(storage, 
                myCounter->line,
                myCounter->filename,
                myCounter->bytes, 
                myCounter->calls);
    }
  }
  return 0;
}
int endRound (struct Round* myRound)
{  
   if (storage->count < max_stored_counters) to_storage(myRound);
   active = 0;
  return 0;
}

// sorting algorithm for final storage before printing results. 
int bubbleSort(struct Round* myRound)
{  

  int n = (int) myRound->count;
    
  for (int x = 0; x < n; x++)
  {
     for (int y = 0; y < n-1; y++)
     {
       struct Counter* c1 = myRound->counters[y];
       struct Counter* c2 = myRound->counters[y+1];
       uint64_t t1 = c1->bytes;
       uint64_t t2 = c2->bytes;
       if (t1 > t2) 
       {
        
        struct Counter* temp = malloc(sizeof(Counter));
         *temp = *c2;
         *c2 = *c1;
         *c1 = *temp;
       }
     }  
  }
  return 0;
}


// checks to see if we should start a new round (if one isn't running.) 
int checkStream(size_t sz)
{
   int r = rand () % prob_const;
    
   if (r == prob_const-1 && active == 0
       && stream->count < max_rounds
       && storage->count < max_stored_counters)
       initRound();
   
   return 0;
}



// update a round based on new metadata.
int updateRound(metadata* m)
{
  // get the current round, which is stream->last
  struct Round *myRound = stream->last; 

  if (myRound->searches < myRound->max_searches
   && myRound->count < max_counters_per_round) 
   {
     if (searchRound(myRound, m->line, m->filename, (int) m->my_size, 1) == 1)
     {
       endRound(myRound);
       initRound();
       updateRound(m);
       
     }
     
   }
  else
  {
     endRound(myRound);
     }
  return 0;
}

int free_counters(struct Round* myRound)
{
   for (int i = 0; i< max_counters_per_round; i++)
   {
      Counter* myCounter = myRound->counters[i];
      free(myCounter);
   }
   return 0;
}
int deleteRound()
{
   Round* myRound = stream->last;
   Round* prevRound = myRound->prev;
   prevRound->next = NULL;
   stream->last = prevRound;
  
   free_counters(myRound);
    
   free(myRound->counters);
    
   free(myRound);
   stream->count--;
  return 0;
}
int free_all()
{
  while (stream->count > 1)
    deleteRound();
  
  free(stream);
  free_counters(storage);
  free(storage);
  free(metaList);
  return 0;
  
}
// report the heavy hitters with bytes or malloc calls above the given threshold.
int hhreport()
{
  if (active == 1) endRound(stream->last);
  
  
  
  uint64_t myCount = storage->count;
   bubbleSort(storage);
  
  printf("\n***********************HEAVY HITTER REPORT*************************\n\n");
  for (uint64_t i = 0; i < 15; i++)
  {
    struct Counter* myCounter = storage->counters[i];       
    float bytes_percent = ((float) myCounter->bytes / (float) totalBytes)*100;
    float calls_percent = ((float) myCounter->calls / (float) totalCalls)*100;
  
   if (bytes_percent > threshold || calls_percent > threshold)
    printf("HEAVY HITTER: %s:%i: %" PRIu64 " allocations (~%.02f%%), %" 
                                    PRIu64 " bytes (~%.02f%%)\n",
           myCounter->filename, 
           (int) myCounter->line, 
           myCounter->calls,
           calls_percent,
           myCounter->bytes,
           bytes_percent);
    
   }
  free_all();
  return 0;
}

// initialie metadata block.
int initMeta (metadata* m, uint64_t sz, const char* file, int line)
{
    m->my_size = sz;
    m->allocated = 1;
    m->filename = file;
    m->next = NULL;
    m->prev = NULL;
    m->line = line;
    m->padding = NULL;
    return 0;
}
/*
 *  m61_malloc
 */ 
void* m61_malloc(size_t sz, const char* file, int line) {
    
    (void) file, (void) line;   // avoid uninitialized variable warnings

    // checking for garbage
    if (sz > (size_t) garbage_size)
    {
       mystats.nfail +=1;
       mystats.fail_size += sz; 
       return NULL;
    }
   
    incrementStats(sz);

    // set aside space for the new pointer 
    void *ptr = malloc(sizeof(metadata) + sz + sizeof(int) + 16*sizeof(char*));
    
    // initialize heap_min if it is null.
    if (mystats.heap_min == NULL)
    {
       initLists();
       mystats.heap_min = ptr; 
    }
    
    // initialize metadata block
    metadata* m = ptr;
   
    // if we are using the address that was freed last, then remove its "freed" status. 
    if ((int*) &m == last_freed) last_freed = NULL;
    
    initMeta(m, sz, file, line);
    
    checkStream(sz);
    
    if (active == 1) updateRound(m);
    
    void *addr = (metadata*) ptr + 1;
    
    // get something to compare heap_max to.
    void *end = addr + sz + sizeof(int);
    
    // add this data block to our linked list of metadata blocks.
    add_to_list(m, metaList);
    
    /*
     *  Suffix Logic
     */
     
    int* suffix = addr + sz;
    
    // is this the final address or the address after the block?
    suffix[0] = end_code;
    
    // modularize   
    if ( (char*) ptr < mystats.heap_min || mystats.heap_min == NULL) 
      mystats.heap_min = ptr;
       
    if ( (char*) end > mystats.heap_max || mystats.heap_max == NULL) 
      mystats.heap_max = end;
 
    return addr;   
}

// looks for a match in the existing metadata to see if a pointer has been allocated.
int find_match (metadata* ptr)
{  
  int count = metaList->count;
  metadata* cur = NULL;
  
  while (count > 0)
  {
    if (cur == NULL)
      cur = metaList->first; 
    else
      cur = cur->next;

    if (ptr == cur) return 1;

    count--;
  }
  return 0;
}

void m61_free(void *ptr, const char *file, int line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
 
         metadata* m = (metadata*) ptr - 1;
       
    if ( (int) mystats.heap_max < 1 || (int) mystats.heap_min < 1)
    {  

      printf("MEMORY BUG???: invalid free of pointer %p, not in heap\n", ptr);
    }
    // it is listed as allocated, AND it is in the linked list of meta-pointers.
 
    else if (m->allocated==1 && find_match(m)==1) 
    {
      
       size_t sz = m->my_size;
       mystats.nactive -=1;
       mystats.active_size -= sz;

       // guard against double frees.
      m->allocated = 2;
             
      // locate suffix
      int* epilogue_ptr = ptr + m->my_size;
       
      // if lacking an end code, then there was a wild write!
      if (epilogue_ptr[0] != end_code)
         printf("MEMORY BUG??: detected wild write during free of pointer ???\n");
      
      metaList->count--;
      
      if (metaList->first == (void*) m)
      {
        metaList->first = m->next;
      }
      else if (metaList->last == (void*) m)
      {
        metaList->last = m->prev;
      }
      else
      {
        // define prev & next blocks.
        metadata* prev_m = (metadata*) m->prev;
        metadata* next_m = (metadata*) m->next;
        
        // connect prev & next blocks to each other.
        prev_m->next = next_m;
        next_m->prev = prev_m;
      }
       last_freed = (int*) &m[0];
       free(m);
    }
   
    else if (m->allocated==2 || last_freed == (int*) &m[0])
    {
      printf("MEMORY BUG???: invalid free of pointer %p\n", (void*) ptr);
    }
    // an unallocated pointer NOT resulting from double free. 
    else
    {
      printf("MEMORY BUG: %s:%i: invalid free of pointer %p, not allocated\n", 
             file, line, (void*) ptr);
   
      int count = metaList->count;
      int found = 0;
      metadata* cur_m = NULL;
      
      while (count > 0 && found == 0)
      {
         // traversing the list of meta pointers
         if (cur_m == NULL) 
            cur_m = metaList->first;
         else
            cur_m = cur_m->next;
            
            // get to the content pointer
            void* cur_p = (metadata*) cur_m + 1;
            
            // find dist between freed pointer & content pointer
            uint64_t distance = ptr - cur_p;
         
         // if this point is within a memory block...
         if (distance > 0 && distance < cur_m->my_size)
         {
           printf("  %s:%i: %p is %i bytes inside a %i byte region allocated here\n", 
           file, (int) cur_m->line, (void*) ptr, (int) distance, (int) cur_m->my_size);
           printf("  %p\n", (void*) cur_p);
           found = 1;
         } 
         count--;
       }
    }   
}

void* m61_realloc(void* ptr, size_t sz, const char* file, int line) {
    void* new_ptr = NULL;
    if (sz)
        new_ptr = m61_malloc(sz, file, line);
    if (ptr && new_ptr) {
        // Copy the data from `ptr` into `new_ptr`.
        // To do that, we must figure out the size of allocation `ptr`.
        metadata* old_m = ptr - sizeof(metadata);
        metadata* new_m = new_ptr - sizeof(metadata);
        
        
        uint64_t old_size = old_m->my_size;
        uint64_t new_size = new_m->my_size;
        uint64_t num = (old_size > new_size) ? new_size : old_size;
        memcpy(new_ptr, ptr, new_size);
    
    }
    // frees the old pointer
    if (ptr) m61_free(ptr, file, line);
    return new_ptr;
}

void* m61_calloc(size_t nmemb, size_t sz, const char* file, int line) {
    
    // fixes test014.
    void* ptr = m61_malloc(nmemb * sz, file, line);
    if (ptr)
        memset(ptr, 0, nmemb * sz);
    return ptr;
}

void m61_getstatistics(struct m61_statistics* stats) {
     *stats = mystats;
}

void m61_printstatistics(void) {
    struct m61_statistics stats;
    m61_getstatistics(&stats);

    printf("malloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("malloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}

/* prints the leak report.*/
void m61_printleakreport(void) {
    if (metaList->count != 0)
    { 
       int count = metaList->count;
       metadata* cur = NULL;
       while (count > 0)
       {
         if (cur == NULL) 
            cur = metaList->first;
         else
            cur = cur->next;
            
            void* ptr = (metadata*) cur + 1;
            
         printf("LEAK CHECK: %s:%i: allocated object %p with size %i\n", 
             cur->filename, (int) cur->line, (void*) ptr, (int) cur->my_size);
         count--;
       }
    }
}
 
