# You Spin Me Round Robin

The goal of this lab was to simulate the round-robin scheduling algorithm for a given number of processes and quantum length.

## Building

```shell
make
```

## Running

cmd for running 
```shell
./rr filename quantum_length
```
To run the program, type the above into the command line, replaceing filename with a '.txt' file following this format:

4  _//Number of processes_  
1, 0, 7  _//PID, arrival time, burst time_     
2, 2, 4  
3, 4, 1  
4, 5, 4  

**ex:**
```shell
./rr processes.txt 3
```

results 
```shell
Average waiting time: 7.00
Average response time: 2.75
```
The output of the program will be the above. You can check by hand or uncomment the print statements to see step by step how the scheduler works.

## Cleaning up

```shell
make clean
```
