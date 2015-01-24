
#include "io61.h"
#include <sys/types.h>
#include <sys/stat.h>

// sys/man is for mmap 
#include <sys/mman.h> 
#include <limits.h>
#include <errno.h>

// cache sizes; the larger one is used for reordercat61

#define CACHE_SIZE 131072

struct io61_file { 

    // read/write, initialized, used in seeks
    int mode;
    int first_pass; 
    int seekable;
    
    // the file's cache
    unsigned char* cbuf;
    
    // tracking position during seeks
    size_t cpos; 
    size_t csz;
    
    // current position & frontier in the cache
    off_t tag;
    off_t end_tag;
    
    // our current position in the file
    off_t pos_tag;
    
    // file size (-1 for pipes)
    ssize_t sz;
    
    // the file ID
    unsigned int fd;
};

// io61_fdopen(fd, mode)
//    Return a new io61_file that reads from and/or writes to the given
//    file descriptor `fd`. `mode` is either O_RDONLY for a read-only file
//    or O_WRONLY for a write-only file. You need not support read/write
//    files.

io61_file* io61_fdopen(int fd, int mode) {

    io61_file* f = (io61_file*) malloc(sizeof(io61_file));
    // assign fd to a global variable
    f->fd = fd; 
    
    // designate this as a read file (0) or a write file (1). 
    f->mode = mode;

    f->first_pass = 0;
    
    if (f->mode == O_RDONLY)
       f->sz = io61_filesize(f);
    else f->sz = -1;
      
    if (f->sz > 0) {
       // here, cbuf is a memory map rather than a cache. 
       f->cbuf= mmap(NULL,
                     f->sz,
                     PROT_READ,
                     MAP_SHARED,
                     fd,
                     0);
                       
       if (f->cbuf == MAP_FAILED) perror("mmap");
  
       // csz is decided in read() for pipes.
       f->csz = f->sz;
       f->end_tag = f->sz;
    }
    // for all files
    f->cpos = 0;
    f->pos_tag = 0;
    f->tag = 0;
    f->seekable = 0;
    return f;
}


// io61_close(f)
//    Close the io61_file `f` and release all its resources, including
//    any buffers.

int io61_close(io61_file* f) {
    
     if (f->mode==O_WRONLY) io61_flush(f);
    
     // officially closing the file
     int r = close(f->fd);
     
     // if we used the memory map, free it; otherwise, free the buffer.
     if (f->mode == O_RDONLY && f->sz > 0)
       munmap(f->cbuf, f->sz);
     else
       free(f->cbuf);  
       
     free(f);
     return r;
}

// io61_readc(f)
//    Read a single (unsigned) character from `f` and return it. Returns EOF
//    (which is -1) on error or end-of-file.

int io61_readc(io61_file* f) {
     unsigned char buf[1];
     if (io61_read(f, (char*) buf, 1) == 1)
       return buf[0];
     else
        return EOF;
}

// io61_read(f, buf, sz)
//    Read up to `sz` characters fom `f` into `buf`. Returns the number of
//    characters read on success; normally this is `sz`. Returns a short
//    count if the file ended before `sz` characters could be read. Returns
//    -1 an error occurred before any characters were read.

 ssize_t io61_read(io61_file* f, char* buf, size_t sz) {

    ssize_t pos = 0;
    
    // for pipes
    if (f->first_pass == 0 && f->sz < 0) 
    {
      f->end_tag = 0;
      f->cpos = 0;
      f->csz = 0;
      f->cbuf = calloc(CACHE_SIZE, sizeof(char*)); 
      if (f->cbuf == NULL) perror("calloc in read failed");  
    }
    f->first_pass = 1; 

    // while temp position does not equal block size
    while ( pos != (off_t) sz)
    {
      // if a pipe, use cache; read from it if it is not useful. 
      if (f->pos_tag >= f->end_tag && f->sz < 0) 
      {
          // set tag of cache equal to end-tag of cache,
          // since this is a linear arrangement of data. 
          f->tag = f->end_tag;
       
          // prefetch the data. 
          ssize_t data_fetched = 0;
          
          // fetch new data into this cache. 
          data_fetched = read(f->fd, f->cbuf, BUFSIZ);
       
          // if we are at the end
          // return pos, if it exists.
          // if pos does not exist, return data_fetched.
          if (data_fetched <= 0 || data_fetched == EOF)
          return pos ? pos : data_fetched;   
       
          f->csz += data_fetched;
          f->end_tag+=data_fetched;
     }
     else
     {
       ssize_t n = sz - pos;
       ssize_t diff = f->end_tag - f->pos_tag;
       if (n > diff) n = diff;
       off_t offset = f->pos_tag - f->tag;
       
       // copy from the cache into the buffer
       memcpy(buf + pos, f->cbuf + offset, n);
       f->cpos +=n;
       pos +=n;
       f->pos_tag +=n; 
       if (diff == 0 && f->sz > 0) return pos; 
     }
    }
    return pos;
}

 

// io61_writec(f)
//    Write a single character `ch` to `f`. Returns 0 on success or
//    -1 on error.

int io61_writec(io61_file* f, int ch) {
      unsigned char buf[1]; 
      buf[0] = ch;
      if (io61_write(f, (char*) buf, 1) == 1)
        return 0;
      else
        return -1;
    return 0;
}
// 
// io61_write(f, buf, sz)
//    Write `sz` characters from `buf` to `f`. Returns the number of
//    characters written on success; normally this is `sz`. Returns -1 if
//    an error occurred before any characters were written.



ssize_t io61_write(io61_file* f, const char* buf, size_t sz) {
 
    if (f->first_pass == 0)
    { 
       f->cpos = 0;
       f->csz = 0; 
       f->cbuf = calloc(CACHE_SIZE, sizeof(char*)); 
    }
    // if we are seeking, write immediately. 
    if (f->seekable == 2) 
    {
      f->csz = sz; 
      int rs = write(f->fd, buf, f->csz);
      if (rs < 0) return -1;
      f->pos_tag+=f->csz;
    }
    else
    {
     f->first_pass = 1;
     size_t amt = ((f->csz + sz) <= CACHE_SIZE*sizeof(char*)) ? sz : (CACHE_SIZE - f->csz);
     memcpy(f->cbuf + f->csz, buf, amt);
     f->csz +=amt;

     // if we have reached the limit of the cache or have run out of data, empty cache
     if (f->csz >= CACHE_SIZE || f->csz < (size_t) sz)
     {
       int rs = write(f->fd, f->cbuf, f->csz); 
       if (rs == -1) return -1;
       f->cpos = 0;
       f->csz = 0;
     }
    }
    return 0;
}

// io61_flush(f)
//    Forces a write of all buffered data written to `f`.
//    If `f` was opened read-only, io61_flush(f) may either drop all
//    data buffered for reading, or do nothing.
     
int io61_flush(io61_file* f) {
    if (f->seekable == 0) write(f->fd, f->cbuf, f->csz);     
    return 0;
}

// io61_seek(f, pos)
//    Change the file pointer for file `f` to `pos` bytes into the file.
//    Returns 0 on success and -1 on failure.


// adapted from exercise io61. 
int io61_seek(io61_file* f, off_t pos) { 

  if (f->first_pass == 0 && f->sz < 0 && f->mode == O_RDONLY)
  { 
    f->csz = 0;
    f->end_tag = 0;
  }
  off_t r;
  switch (f->mode) 
  {
     case O_WRONLY: 
     // increment seekable number of seeks until we get to 2
     if (f->seekable < 2) f->seekable++;
     
     // go to pos bytes from origin
     r = lseek(f->fd, pos, SEEK_SET);    
     if (r != pos) return -1;
     f->cpos = 0; 
     return 0;
     break;
     
     case O_RDONLY: 
     if (f->sz > 0)
     {
        // for regular files
       if (pos >= f->tag && pos <= f->tag + (off_t) f->csz) 
       {
         f->pos_tag = pos;
         f->cpos = f->pos_tag - f->tag;
         return 0;
       }
       // take care of alignment
       else
       {
         off_t aligned_pos = pos - (pos%BUFSIZ);
         off_t r = lseek(f->fd, (off_t) aligned_pos, SEEK_SET);
         if (r != aligned_pos) return -1;
         
         // reset tag and end_tags; buffer is empty now.
         f->end_tag = f->sz; 
         f->pos_tag = pos;
         f->cpos = f->pos_tag - f->tag; 
         return 0;
       }
     }
     break;
  }
  return 0;
}


// You shouldn't need to change these functions.

// io61_open_check(filename, mode)
//    Open the file corresponding to `filename` and return its io61_file.
//    If `filename == NULL`, returns either the standard input or the
//    standard output, depending on `mode`. Exits with an error message if
//    `filename != NULL` and the named file cannot be opened.

io61_file* io61_open_check(const char* filename, int mode) {
    int fd;
    
    // if a filename exists....
    if (filename)
    
        // system call -- but what could mode be and why 0666?
        fd = open(filename, mode, 0666);
        
        // what is 
    else if ((mode & O_ACCMODE) == O_RDONLY)
        fd = STDIN_FILENO;
    else
        fd = STDOUT_FILENO;
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", filename, strerror(errno));
        exit(1);
    }
    return io61_fdopen(fd, mode & O_ACCMODE);
}
// io61_filesize(f)
//    Return the size of `f` in bytes. Returns -1 if `f` does not have a
//    well-defined size (for instance, if it is a pipe).


// fstat is a system call that is used to obtain information about a file based on its
// file descriptor. 

off_t io61_filesize(io61_file* f) {
 
    struct stat s;
    int r = fstat(f->fd, &s);
    if (r >= 0 && S_ISREG(s.st_mode))
        return s.st_size;
    else
        return -1;
}

// read (int handle, void *buffer, int nbyte)

// io61_eof(f)
//    Test if readable file `f` is at end-of-file. Should only be called
//    immediately after a `read` call that returned 0 or -1.

int io61_eof(io61_file* f) {
    char x;
    ssize_t nread = read(f->fd, &x, 1);
    if (nread == 1) {
        fprintf(stderr, "Error: io61_eof called improperly\n\
  (Only call immediately after a read() that returned 0 or -1.)\n");
        abort();
    }
    return nread == 0;
}
