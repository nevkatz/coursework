README for CS 61 Problem Set 1
------------------------------
YOU MUST FILL OUT THIS FILE BEFORE SUBMITTING!

YOUR NAME: Nevin Katz 
YOUR HUID: 80519354

(Optional, for partner)
YOUR NAME: N/A
YOUR HUID: N/A

OTHER COLLABORATORS AND CITATIONS (if any):

Bubble Sort Resource from C-Programming.com
http://www.cprogramming.com/tutorial/computersciencetheory/sorting1.html

Frequency Estimations of Internet Packet Streams with Limited Space
http://erikdemaine.org/papers/NetworkStats_ESA2002/paper.pdf

NOTES FOR THE GRADER (if any):

My heavy hitters piece is a modified implementation of the probabilistic algorithm from the Frequency Estimations paper. The algorithm involves dividing the stream of malloc calls into a number of rounds. These rounds are interspersed through runtime, and a given round is called at a random time. Each round starts with an array of empty, inactive counters, and each counter may be activated by being matched with a file-line pair during malloc call. The number of times new data can be added to a round increases with the start of each new round.

During a round, a counter becomes activated when it receives its first instance of malloc data. Each activated counter keeps track of its file-line pair and increments its number of malloc calls and byte size every time its file-line pair calls malloc.

WHEN ROUNDS END: Rounds end either when the max number of searches it can accept has been reached, or when there are no more counters left for a new file-line pair. There is a fixed number of counters each round has, which allows me to malloc a certain amount of space for them.

FINAL STORAGE: At the end of each round, its counters are moved to a final round, which I'm referring to as storage, if storage still has capacity. Storage is unlimited in terms of size, but once it is out of free counters, no new counters are accepted. I have storage as an array so it can be sorted when the report is printed. 

SORTING: The sorting at the end happens via bubble sort, which I know is not the most efficient, but I wanted to focus on the overall program and not on sorting algorithms - I also am only calling it once on a small array of fixed size, so I didn't think its drawbacks would be an issue. I like to use simple solutions when the opportunity presents itself.

A few notable aspects of the algorithm I aimed to emphasize: 

I have rounds occurring at random times. With each malloc, there is a 1/5 chance a round will start if a round is not currently in progress. 

The capacity of a round, measured in the number of searches it can accept, increases over time. I start with the first round only being able to accept two searches, and then I increment this capacity by 10 with the start of each subsequent round. 

My implementation differs from the algorithm in two key ways: 

If a counter added to a round runs across another counter with the same file-line pair, the algorithm stipulates that it is swapped in. In my implementation, however, the bytes and calls from the new counter will simply be added to those of the counter that's already in there. My rationale is that I am already not sampling all the malloc traffic, and of the traffic I do sample, I do not want to discard anything. I found that when I did not discard my data, my data was closer to those in the spec.

I don't really have a "tournament" at the end wherein counters with low counts are eliminated - I just print the counters from storage that are above the threshold for # allocations or memory size. My rationale is that the data already looks accurate so I'd rather keep the algorithm simple and avoid an extra step that could cost more time. 

EXTRA CREDIT ATTEMPTED (if any): In addition to reporting heavy hitters that malloc more than 10% of the bytes, I am also reporting those that make more than 10% of the allocations. 
