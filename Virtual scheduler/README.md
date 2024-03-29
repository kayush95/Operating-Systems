# Virtual scheduler
Virtual scheduler runs on top of the existing linux scheduler which you should not change. It consists of two major
components: **a generator** and **a scheduler**. The generator is responsible for generating the
processes at regular interval. The scheduler maintains a *Ready Queue* which contains the
set of runnable processes at any time-instance. It chooses/schedules one of the runnable
processes according to the scheduling algorithm for execution. Next it sends a *notify* signal
to only that chosen/scheduled process and *suspend* signal to all other processes.
Depending on the signal (*notify*/*suspend*) received from the scheduler, the processes can
either (re)start processing or wait until it’s scheduled again. Once all the processes
terminate, the scheduler also terminate. After building this simulation framework, we evaluate different scheduling strategies
and evaluate their performance in terms of average response time, average waiting time and
average turnaround time. In this implementation, processes and scheduler communicate among themselves in various
different ways which are discussed in detail in the *Communication Protocol* section of *problem.pdf*.


The complete problem overview is given in *problem.pdf*.


Commands to use to run the program:
-----------------------------------
* Compile all files

  ```
  make all
  ```

* Clean all files

  ```
  make clean
  ```

* Run

In one terminal
  ```
  ./sched <PR or RR> <Quantum>
  ```
Here 
PR: Priority based round robin
RR: Regular round robin

Then open another terminal and run ```./gen```.
