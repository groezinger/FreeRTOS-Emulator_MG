# FreeRTOS Exercises 2-4

Exercises 2-4 for ESPL Course by Maximilian Groezinger

## Which Buttons to press?
Switch between Tasks by pressing E and Q to quit.
In Task 2 you can increase the counter of Buttons A,B,C and D and reset them with any mouse button. 
In Task 3 you can stop/restart the increasing variable by pressing J
In Task 3 you can show additional counters by Pressing K and L which are resetted every 15 seconds (starting after you 
switch to the Screeen of Exercise 3)

## Theory Questions:

### How does the mouse guarantee thread-safe functionality?
By using a Semaphore Mutex mouse.lock

### What is the kernel tick?
Real-time kernels use a timer or a similar source of periodic interrupts (Ticks) to implement delays
and other helpful services for multi-task applications. For example every tick the FreeRTos scheduler checks
which tasks should be run.

### What is a tickless kernel
A tickless kernel is an operating system kernel in which timer interrupts do not occur at regular intervals, but are only delivered as required.

### What happens if the stack size is too low?
The stack can overflow meaning that there is data lost because the stack is not big enough. Stack overflow is a very common cause of application instability.

### Observations from changing priorities in Task 4:
Since Task 2 enables Task 3, the two is always printed before the 3 independent of the priorities. Besides that the order of the printed numbers changes according
the priorities. If prioritize the OutputTask too high it messes up my output completely because of the way I implemented that task. 

