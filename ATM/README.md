# ATM - Client Transaction System
An ATM system, where clients can perform various operations like withdraw money, deposit money, or check balance. The master process creates a
set of ATM processes, an ATM locator file and different communication mediums. Each client interacts
with the ATM with the help of the ATM locator. The ATM locator is used to lock and access the
individual ATM. Once gained the access, client performs different ATM transaction operations. Each
ATM has its own shared memory to store information about the transactions conducted on it. The ATMs
executes two different types of consistency check protocols, based on the client transaction request.

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
  ./master k
  ```
k = no of ATMs (e.g. 3)

Open ```k``` terminals and run ```./atm <ATM_key>``` in each one of them (ATM_keys will be shown when ./master is executed)

Then open ```m``` terminals. Run ```./client``` in each one of them to create ```m``` clients. Instructions will be given in each client terminal to perform various operations like withdraw money, deposit money, or check balance.

