all: master scheduler mmu process
	
Master:Master.c
	gcc -o master master.c

MMU:MMU.c
	gcc -o mmu mmu.c

Process:Process.c
	gcc -o process process.c

Scheduler:Scheduler.c
	gcc -o scheduler scheduler.c

clean:
	rm master scheduler mmu process
