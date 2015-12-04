all:
	$(MAKE) -k -C ..

#THIRD_FLAGS = -inval -run -leaks new -before sched_step -every 1024
THIRD_FLAGS = -uninit heap+stack -inval -run -leaks new -before sched_step -every 1024

fifo-third:
	third $(THIRD_FLAGS) fifo

rand-third:
	third $(THIRD_FLAGS) rand
