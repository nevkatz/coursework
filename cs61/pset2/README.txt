README for CS 61 Problem Set 2
------------------------------
YOU MUST FILL OUT THIS FILE BEFORE SUBMITTING!

YOUR NAME: Nevin Katz
YOUR HUID: 80519354

(Optional, for partner)
YOUR NAME:
YOUR HUID:

OTHER COLLABORATORS AND CITATIONS (if any):
io6_read() and io6_seek() are both adapted from Exercise IO, but have 
been modified. 
The mmap logic was adapted from lecture. 

KNOWN BUGS (if any):
No bugs as far as I can tell. All tests are either blue or green.

NOTES FOR THE GRADER (if any):

**REVISIONS**

**Below are the revisions I was able to accomplish per my grader's advice. The one item I had trouble figuring out was how to use the write cache for files that are being seeked - this is still a puzzle to me. That said, I did my best to cover everything else. 

1) When initializing a file, I set f->mode equal to O_RDONLY and O_WRONLY instead of 0 and 1, respectively

2) When preparing the cache, the size argument in calloc no longer depends on "sz" passed into the io61_read and io61_write functions. 

3) Made size variable an unsigned int

4) After attempting mmap, we no longer exit if mmap fails

5) Eliminated arbitrary constants, such as max_size

6) Pointless lines that were flagged by the grader have been removed

7) When writing a seekable file, we now return -1 if write fails and do not update f->pos_tag.

8) When memcopying during io61_write, I now check the size we are writing to make sure we don't overflow the write buffer

9) Eliminated init_cache function to avoid an unneeded function jump

10) Now using the io61_filesize function instead of the filesize function from lecture.

11) Fixed quite a few indents

12) Simplified io61_file struct

EXTRA CREDIT ATTEMPTED (if any): N/A - I focused solely on revisions and on better understanding the code. 
