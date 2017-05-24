# Multiple Producers-Consumers and Rail Manager

**Multiple Producers and Consumers**: Implementation of a system which ensures synchronisation in a producer-consumer
scenario. We demonstrate deadlock condition and provide solutions for
avoiding deadlock. In this system, a main process creates 5 producer processes and 5
consumer processes who share 2 resources (queues). The producer's job is to generate a
piece of data, put it into the queue and repeat. At the same time, the consumer process
consumes the data i.e., removes it from the queue. In the implementation, synchronization and mutual exclusion have been ensured. For instance, the producer should be stopped
if the buffer is full and that the consumer should be blocked if the buffer is empty. 
Mutual exclusion is also enforced while the processes are trying to acquire the resources.

**Rail Manager**: Assume, you have a rail-crossing scenario (as given in *problem.pdf*). Trains may come from all the four
different directions. We implement a system where it never happens that more than
one train crosses the junction (shaded region) at the same time. Every train coming to the
junction, waits if there is already a train at the junction from its own direction. Each train
also gives a precedence to the train coming from its right side (for example, right of North is
West) and waits for it to pass. We also check for deadlock condition if there is any.
Each train is a separate process. A manager is created which creates those
processes and controls the synchronization among them.

The complete problem overview is given in *problem.pdf*.

Commands to use to run the program:
-----------------------------------

**Multiple Producers and Consumers**
* Compile all files

  ```
  make all
  ```

* Clean all files

  ```
  make clean
  ```

* Run

  ```
  ./manager <DPP_toggle (0 or 1)> <probability>
  ```


**Rail Manager**
* Compile all files

  ```
  make all
  ```

* Clean all files

  ```
  make clean
  ```

* Run

  ```
  ./manager <probability>

