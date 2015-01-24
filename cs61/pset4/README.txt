README for CS 61 Problem Set 4
------------------------------
YOU MUST FILL OUT THIS FILE BEFORE SUBMITTING!

YOUR NAME: Nevin Katz
YOUR HUID: 80519354

(Optional, for partner)
YOUR NAME:
YOUR HUID:

NOTE: Last few commits were for extra credit only. 

OTHER COLLABORATORS AND CITATIONS (if any):
At several points during the p-set, I discussed the problem set with another extension student, Hemant Bajpai. We discussed our work during Quincy office hours and traded tips on coding and debugging. During the week, we corresponded via email and Skype and talked about navigating part 2 and 5 of the p-set. In particular, he was very helpful with giving me insight on how to debug and work through Step 2. 

Also,

*Eddie provided sage guidance on memcopying in step 2, and helped a great deal with debugging step 5. He was also super responsive on Piazza, as were so many other TFs. 

*Iva helped me with getting started with step 2. 

*Afdab helped me to conceptualize the process of fork. The outcome of our conversation was a complete paradigm shift that switched me onto the right track with step 5. 

*Marcus provided me with a terrific idea for how to use virtual_memory_lookup in fork. (Aside: both him and Afdab were still rotating around tables and helping long hours after OH's had ended!)

*Zach  provided a helpful line of code that I used in INT_SYS_FORK for checking whether a file was writeable:    if ((vam.perm & (PTE_U|PTE_W)) == (PTE_U|PTE_W)). I use this in my fork_process helper function. He also provided insight on how to leverage virtual_memory_map in fork.

*Nikhil provided moral support as always. Early in the week, he went the extra mile after his office hours had ended and helped me understand some of the concepts after I had solved Step 2. And just when I was ready to call it a day after working on Step 7 a bit, he encouraged me to press on and finish Step 7.

NOTES FOR THE GRADER (if any): Thanks to the staff for your help and support! I look forward to hearing your feedback. 

I tried to create helper functions for stretches of code cleaner (ex. in INT_SYS FORK) and for actions that are called often. Let me know what you think of how I used the helper functions and if there are ways to improve them. 

I tried to implement feedback from pset2 and stay away from unnecessary or under-utilized constants.

EXTRA CREDIT ATTEMPTED (if any): I completed Step 7! Pressing "E" will now result in the crazy patterns similar to those in the spec. Testing revealed no assert issues or other unexpected behavior. 



