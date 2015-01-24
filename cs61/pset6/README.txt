README for CS 61 Problem Set 6
------------------------------
YOU MUST FILL OUT THIS FILE BEFORE SUBMITTING!

YOUR NAME: Nevin Katz
YOUR HUID: 80519354

(Optional, for partner)
YOUR NAME: Hemant Bajpai
YOUR HUID: 80887507

RACE CONDITIONS
---------------
Write a SHORT paragraph here explaining your strategy for avoiding
race conditions. No more than 400 words please.

One race condition we avoided involved pong arguments being sent to the server in the wrong order when multiple threads were active at once. This scenario was avoided in the handout code, because the main thread could not move onto its next position and create a new child thread until the current child thread had finished all its work and was about to exit.

However, we then called cond_signal *before* the current child thread received the body, so that the main thread could continue to the next position before the server completed its response to the current child thread. Because this resulted in more than one thread being active at once, it was possible for threads to send signals to the server in the wrong order. To avoid this, we placed cond_signal right after our calls to http_connect (both the normal call and the exponential backoff call). This ensured that the server received the most recent position before allowing the main thread to move onto the next position. As a result, position signals were sent to the server in the proper order with multiple child threads active at once.

To avoid other race conditions, we used three mutex locks. Each of these were used to protect critical regions of code that we didn't want multiple threads operating on at once. We used three different mutex locks so that a given thread could operate on one critical region (ex. incrementing a global variable) while another thread operated on another (ex. pushing a connection to a linked list).

Our regular mutex lock was used for waiting on the cond_signal to kick off, as well for incrementing and decrementing global variables, so we didn't have more than one thread operating on them at a time.  List_mutex was used for adding and removing connections from our linked list. This ensured that we did not have more than one thread operating on this data structure at a time. 

Our connection mutex was used both for responding to a STOP signal and for our helper function wait_check(), which delays the establishment of an http connection if it is called before the ensuing down time concludes. This prevented our global variable of resume_time (which specifies when connectivity should resume) from being altered improperly. Between the strategic placement of placement of cond_wait() and the use of our mutex locks, we successfully avoided race conditions.

OTHER COLLABORATORS AND CITATIONS (if any): Nikhil as usual was tremendously helpful, especially during phases 4 and 5. We used one of the last lecture's pong demos as inspiration for our extra credit. 

KNOWN BUGS (if any): N/A

NOTES FOR THE GRADER (if any): N/A

EXTRA CREDIT ATTEMPTED (if any): In "fun" mode, a series of different colored pong balls will write "CS61." After a short amount of time, they will all disappear in unison. 

