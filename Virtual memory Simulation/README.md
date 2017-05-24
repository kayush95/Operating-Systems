# Simulating Virtual memory through (pure) demand paging
The problem overview is given in *problem.pdf.

Compile using TinyC
-------------------
* Allow files to execute

  ```
  chmod +x compile.sh
  chmod +x test.sh
  ```
* Compile
  
  ```
  ./compile.sh filename.c
  ```
* Run

  ```
  ./filename.out
  ```
  
Run test files
--------------
To run all test files provided in test folder,

```
./test.sh
```



Commands to use to run the program:
-----------------------------------
*Compile:```To compile all files use: make all```
*Clean: To clean use: make clean
*To run use: ```./master k m f
		k = no of processes
		m = no of pages in virtual space
		f = no of frames
```

The output will be shown as required and also written to result.txt
