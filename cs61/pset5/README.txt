README for CS 61 Problem Set 5
------------------------------
YOU MUST FILL OUT THIS FILE BEFORE SUBMITTING!

YOUR NAME: Nevin Katz
YOUR HUID: 80519354

(Optional, for partner)
YOUR NAME: Hemant Bajpai
YOUR HUID: 80887507

OTHER COLLABORATORS AND CITATIONS (if any): Ore was very helpful with lending insight on execvp when I (Nevin) was getting started.  Nikhil was extraordinarily helpful for both of us - particularly with regards to pipes and interrupts. He has great dedication and a willingness to go above and beyond.

NOTES FOR THE GRADER (if any): This assignment was as always a tremendous journey, and we both learned a great deal about shell programming. 

On a practical note, we added a usage response if cd is entered without an argument. I'm sure there are places where usage messages could be added if we had more time.

EXTRA CREDIT ATTEMPTED (if any): We have implemented some history functionality. Here is a quick explanation of how it works. 

1) Upon typing in "history" (with no arguments), a history of the most recent command lines entered will be displayed.  

2) If the number of command lines the user entered was less than or equal to the max amount of commands (which is set to 20 for testing) all commands ever entered during the session will be displayed. 

3) If the number of commands entered is greater than the max amount, the most recent twenty command lines typed in will be displayed. The commands will be numbered 1 through 20. 

4) If "history -c" is pressed, it will clear the history. 

5) If "history" is entered with an argument other than "-c", a usage statement will be displayed. 

If we had more time, it would be great to add more usage statements for other incorrectly entered commands, and perhaps have the previous statement show up when the 'up' cursor is pressed. 



