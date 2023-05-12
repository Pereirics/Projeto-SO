Program Execution Tracking and Monitoring

We aim to implement a program monitoring service for a machine. Users should be able to execute programs through the client and obtain their execution time (i.e., the total time from when the user requests the client to run the program until its completion). A system administrator should be able to check, through the server, all programs currently running, including the time spent by each. Finally, the server should also allow the querying of statistics on completed programs (e.g., the aggregate execution time of a certain set of programs).

We will be using named piped and unamed pipes to communicate between files and between processes.

Here is a how the program should be used:
    1. make
    2. ./monitor (Folder where you want to store the files with the information about the programs that were run)
    3. ./tracer (Options)

Options to use in ./tracer
    ./tracer execute -u "ls -l, or any other command"
    ./tracer execute -p "ls -l | wc -l, or any other pipeline"
    ./tracer status
    ./tracer stats-time "Several PIDs"
    ./tracer stats-command command "Several PIDs"
    ./tracer stats-uniq "Several PIDs"