#
# Makefile
#
# Operating Systems, Exercise 4
#

CC = gcc
CFLAGS = -Wall -O2 -g

all: scheduler scheduler-shell new_scheduler-shell scheduler-shell-priority shell prog execve-example strace-test sigchld-example queue.o new_scheduler-shell new_queue.o new_scheduler-shell-priority

scheduler: scheduler.o proc-common.o queue.o
	$(CC) -o scheduler scheduler.o proc-common.o queue.o

scheduler-shell: scheduler-shell.o proc-common.o queue.o
	$(CC) -o scheduler-shell scheduler-shell.o proc-common.o queue.o

new_scheduler-shell: new_scheduler-shell.o proc-common.o new_queue.o
	$(CC) -o new_scheduler-shell new_scheduler-shell.o proc-common.o new_queue.o

new_scheduler-shell-priority: new_scheduler-shell-priority.o proc-common.o new_queue.o
	$(CC) -o new_scheduler-shell-priority new_scheduler-shell-priority.o proc-common.o new_queue.o

scheduler-shell-priority: scheduler-shell-priority.o proc-common.o queue-priority.o
	$(CC) -o scheduler-shell-priority scheduler-shell-priority.o proc-common.o queue-priority.o

shell: shell.o proc-common.o
	$(CC) -o shell shell.o proc-common.o

prog: prog.o proc-common.o
	$(CC) -o prog prog.o proc-common.o

execve-example: execve-example.o
	$(CC) -o execve-example execve-example.o

strace-test: strace-test.o
	$(CC) -o strace-test strace-test.o

sigchld-example: sigchld-example.o proc-common.o
	$(CC) -o sigchld-example sigchld-example.o proc-common.o

proc-common.o: proc-common.c proc-common.h
	$(CC) $(CFLAGS) -o proc-common.o -c proc-common.c

shell.o: shell.c proc-common.h request.h
	$(CC) $(CFLAGS) -o shell.o -c shell.c

scheduler.o: scheduler.c proc-common.h request.h queue.h
	$(CC) $(CFLAGS) -o scheduler.o -c scheduler.c

scheduler-shell.o: scheduler-shell.c proc-common.h request.h queue.h
	$(CC) $(CFLAGS) -o scheduler-shell.o -c scheduler-shell.c

new_scheduler-shell.o: new_scheduler-shell.c proc-common.h request.h new_queue.h
	$(CC) $(CFLAGS) -o new_scheduler-shell.o -c new_scheduler-shell.c

new_scheduler-shell-priority.o: new_scheduler-shell-priority.c proc-common.h request.h new_queue.h
	$(CC) $(CFLAGS) -o new_scheduler-shell-priority.o -c new_scheduler-shell-priority.c

scheduler-shell-priority.o: scheduler-shell-priority.c proc-common.h request.h queue-priority.h
	$(CC) $(CFLAGS) -o scheduler-shell-priority.o -c scheduler-shell-priority.c

prog.o: prog.c
	$(CC) $(CFLAGS) -o prog.o -c prog.c

execve-example.o: execve-example.c
	$(CC) $(CFLAGS) -o execve-example.o -c execve-example.c

strace-test.o: strace-test.c
	$(CC) $(CFLAGS) -o strace-test.o -c strace-test.c

sigchld-example.o: sigchld-example.c
	$(CC) $(CFLAGS) -o sigchld-example.o -c sigchld-example.c

queue.o: queue.c
	$(CC) $(CFLAGS) -o queue.o -c queue.c

new_queue.o: new_queue.c
	$(CC) $(CFLAGS) -o new_queue.o -c new_queue.c

queue-priority.o: queue-priority.c
	$(CC) $(CFLAGS) -o queue-priority.o -c queue-priority.c

clean:
	rm -f scheduler scheduler-shell scheduler-shell-priority shell prog execve-example strace-test sigchld-example new_scheduler-shell new_scheduler-shell-priority *.o
