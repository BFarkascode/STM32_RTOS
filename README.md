# STM32_RTOS
Bare metal implementation of FREERTOS. 
Project showcases running a bitbang machine driving an Adafruit neopixel strip.

## General description
Sometimes, one needs to act in a parallel. Like, doing multiple things at the same time. Why? Because a strict timing needs to be followed (baby needs to be fed AND be ready for kindergarten in 5 minutes), or because we want certain activity to have higher priority than others (baby needs to be changed IMMEDIATELY) or because we actually want to do two things at the same time (it is practical to eat your breakfast while the baby is having hers).

Now, detaching ourselves from parenting and returning to embedded systems design, parallel activities can be managed by peripherals in an MCU handily, for instance, running an LTDC to drive a screen or a DMA to feed data into a serial port from memory…and thus leave the main “while” loop do something else. Similarly, timers can be used to trigger an IRQ and thus force our superloop to execute in a manner we demand it, producing (relatively) precise timing profiles. All in all, in most cases, we are very much good to go using the tools that ST is giving us.

But, what happens if waiting for the super loop to come around with an IRQ updated flag just takes too much time – say, we are generating a high frequency analogue signal? What if we absolutely must update a frame buffer on time with zero tolerance to timing deviation – say because the screen we are driving demand a precise timing? What if we don’t want to waste time waiting for a blocking delay function (the ones like “HAL_Delay”) to conclude? And what if the parallel activities we intend to execute cannot be covered by peripherals - say, we ought to generate a bluetooth stack while doing nonstop calculations on data received from a sensor suite?

Well, that is when an RTOS comes into play.

### RTOS pros
RTOS – or Real Time Operating System - is a software that allows concurrent execution of code. We can define tasks (or threads, depending on which RTOS we are using) and then tell the RTOS’ scheduler to run these threads and tasks in whatever sequence we desire (using the tasks’ priority profile). We can also tell scheduler to run multiple tasks at the same time, where the scheduler will shift between executing two tasks of the same priority. This is called “context switching”.

In practical sense, we can remove the blocking nature of delay from our code: while we delay individual tasks, we allow other tasks to take action while we are waiting. This means that we will be able to run multiple tasks and execute them respecting the timing they demand without having to precisely calculate, how much time should pass between them (which would be the tedious alternative to this approach).

Also, in case we have some kind of fault within our system, only parts of it would be blocked, not the entirety of the code.

All this capacity allows a highly flexible execution of any code where we can manipulate said execution with only a few small changes.

RTOS – since it is a common cross-system library – also helps a lot with porting and then debugging, where only individual tasks would need to be checked au contraire to the entire super loop. This latest comment makes RTOS also ideal for a team of programmers doing concurrent programming on a project.

### RTOS cons
As indicated above, an RTOS by itself isn’t actually a real concurrent program execution like an FPGA would be (only exception to that claim is when we have multiple MCU cores to run the different tasks, this time, actually in parallel). Instead, what it does is that, following its own tick timer, it will shift between the tasks – imagine cutting the tasks up into smaller chunks which will then be executed in an alternating fashion. This tick timer is usually set to 1 ms, so if your superloop finishes in less than 1 ms, an RTOS doesn’t quite help you much. It also doesn’t do you as much good if you are running it on one core only, committing/sharing resources between tasks instead of just assigning them to separate cores.

Another problem is the RAM overhead the RTOS. Due to its construction, RTOS stores almost everything in RAM, meaning that it will struggle to run on simpler MCUs due to memory constraints. Also, linkers and compilers very often fail to point out memory leak issues when using an RTOS, pushing any overly adventurous programmer down the hell of stack overflow.

### Project aims
All things considered, an RTOS is a useful tool, but definitely not a Swiss army knife that should be applied to every situation. As a matter of fact, the project I am proposing below would be perfectly fine as a super loop too…but suggesting anything more complicated would just take the attention away from what I wish to achieve. Which is: run an RTOS to my liking on a relatively complex code structure.

What shall we to do here then?

The timing tolerance of the neopixels is infinite due to how they hold their value upon publish. Yet, this won’t be a case when driving OLED display with composite video signals (which are just very long analogue signals). There the signal must be uninterrupted and must be coming at the exact frequency, otherwise the display will fail to generate an output. As such, what we want to do here is drive a neopixel strip, assuming that it won’t hold its value, i.e. that it demands a constant refresh rate for a uniform colour output (say, the output MUST be updated with whatever value we have every 30 ms at the highest priority). We will be able to check this using an oscilloscope. At the same time, we want to update the source matrix for the neopixel whenever we can and definitely NOT the same time as we are publishing the pixels.

Let’s jump onto it then!

## Previous relevant projects
We will be using the pixelBanger project as the source for the tasks/threads:
- STM32_PixelBanger

## To read
I suggest going through quickly the official RTOS documentation: 

https://www.freertos.org/Documentation/00-Overview

Also, the Shawn Hymel video series on ESP32 RTOS mostly applies:

https://www.youtube.com/watch?v=F321087yYy4&list=PLEBQazB0HUyQ4hAPU1cJED6t3DU0h34bz&index=2

## Particularities
### Getting the RTOS poppin’
As usual, ST has their own way of implementing RTOS. Not too surprisingly, this is just an extra layer over the official FREERTOS code, making everything just a bit more detached and convoluted for no apparent reason. I suggest forgetting the ST approach and simply use the functions that are within the original FREERTOS. They all work, given they are enabled…

…which brings us the “RTOS_config.h” file. This header is the initialization setup for our RTOS. Whatever we change here, will change, how the RTOS runs. (Similarly, if we decide to use the STM32 IDE to integrate the RTOS into our code, any configuration we enable/disable or any limitation we pose on the RTOS, will simply change this header file…and basta).

### RTOS specialities
There are a few concepts that I want to quicky discuss that are crucial for understanding RTOS.

#### Reentry and thread safety
RTOS pretty much does the same thing as IRQs do, meaning that they “cut” the execution of a code to do something else…only to return to the execution of the code once we have the IRQ concluded. Now, certain functions that may come from other libraries (I am looking at you, HAL…) are not, what they call, “reentrant”, meaning that once they are executed within a task, they will not allow the RTOS scheduler to “reenter” its activities. As such, if we have these non-reentrant functions within a task, they can completely break the RTOS. Always pay attention to the functions that are being called within a task and be sure that they are reentrant! (A good example would be delay functions of which RTOS has its own called “vTaskDelay”. Another good example is serial communication using “printf”: if used in one task, it should not be used in another.)

Similarly, certain actions are not “thread safe”, meaning that one task could completely jumble up the execution of another. A good example is updating a global variable by one task while another one is using it. It is always best to use queues (see below) to send data from one task to the other.

#### Heap management
RTOS resides – mostly – in RAM. It defines a heap section for itself (using the config header) as well as a section for each task (using the xTaskCreate). The stack still exists though, which means that stack overflow problems can easily emerge of the heap is not properly allocated and freed (“pvPortMalloc” and “vPortFree”, the reentrant versions of their original counterparts, are your friends during dynamic memory allocation).

There are multiple heap managing schemes for the RTOS to manage the RAM and allocate them properly for itself and the tasks. The RTOS manual has a description of all 5 schemes, though only “heap_management_4” (and 5) the one that is recommended for use.

#### Priority – and its inversion
We can define a baseline priority for any task using xTaskCreate. Doing so will mean that whenever a higher priority task is evoked (using timers, IRQs or just by polling), it will take priority and force the scheduler to make it become executed (become RUNNING). Other states of tasks could be “READY”, “DELAYED” or “BLOCKED”, meaning that the task will either resume the soonest possible occasion, is being pre-empted by another task, or is being skipped by the scheduler until a certain time has passed. The reentrant “vTaskDelay” is a good example to push a task into “BLOCKED”, not allowing it to block the MCU while the task is waiting for some time to pass.

Now, there is a problem with priority, more precisely, with something called “priority inversion”. This occurs when a task takes a resource (say, a flag or a peripheral output) for itself and, despite another task with higher priority coming to play, refuses to relinquish this resource for the other, higher priority task, making this later not able to execute. Of note, some tools within RTOS (such as mutexes, see below) are protected against inversion, but not all of them. This must be kept in mind when tasks are supposed to communicate with each other or share a resource. Alternatively, one can assign a “critical section” within a task that will turn that section into “atomic” in execution: one that can not be interrupted by the RTOS’ own scheduler.

Hardware-based IRQs (EXTI or peripheral IRQs) always have the higher priority and will pre-empt any other task running using RTOS…i.e., they will get executed first, no matter what the scheduler is doing. This means that any IRQ’s handler will fall outside of RTOS jurisdiction, opening up a clear way to introduce unnecessary reentry problems.

NEVER run RTOS-based function in an IRQ handler…if possible. If not possible, use the “ISR” version of them which should be reentrant. And only use global variables!

Also, word of advice, if threads are executing “weird, but functional”, the likely culprit will be with the assignment of priorities to the threads – i.e., the timing of the RTOS.

#### RTOS concepts
Below are the most common concepts within an RTOS. Of note, some of these may demand the inclusion of additional source files since they may not be activated (well, called) through the config file.
1) Mutex - Mutexes are what we may call as “flags” in a superloop. They are  a simple, one-bit trigger that can be taken/given back by a task (“xSemaphoreTake” and “xSemaphoreGive”, respectively), effectively barring other tasks from executing the section of the code that is “protected” by the mutex. The “take” command has a timeout parameter which allows the task to either wait for the mutex to be available or bypass this “take” command after a certain amount of waiting (can be no waiting too). This allows a delicate control of the flow of execution. Mutexes are also immune to priority inversion by design.
2) Semaphores - Semaphores are similar to mutexes, except that they can have multiple elements compared to a mutex, which is just a flag. As a matter of fact, a binary semaphore is the same as a mutex, minus the protection against priority inversion. Semaphores can be useful when multiple tasks are loading the same FIFO: if the semaphore’s count is the same as the slots within the FIFO, we can use it to avoid loading the FIFO when it is already full. Semaphores can be used in IRQs to indicate to tasks that certain actions have been fulfilled. The difference between a binary semaphore and a mutex is that whenever a mutex is “given back”, it is back in the pool and available to all other tasks to be used. A binary mutex must be incremented by something to be available again, effectively allowing a “giver” task to control other “taker” tasks. In other words, mutexes only work between equal tasks but can not be used to implement a hierarchy between them.
3) Queues - Passing data between tasks can be done using global variables, though, alternatively, one might rather rely on queues instead to avoid memory corruption (situations where one task is updating a global variable and is immediately interrupted by another task, doing the same thing before a third task could react to a change in the variable). A queue is a FIFO, outside the tasks, run by the RTOS. Queues have send and receive functions which will come back an error message in case the queue is full or empty. Queues retain their last value and will keep on publishing it on recurring readouts.
4) Timers - Software timers are the same as hardware timer, just run within the RTOS…and as such, are a separate entity (called Timer Service Task). They are based on the RTOS tick timer, so they can’t be more precise than that. They have their own callback functions that are being activated when the timer is triggered. These are all sorted when the timer is defined using “xTimeCreate”. Lastly, “xTaskGetTickCount” is “millis” for RTOS and will give back the number, how many times the RTOS ticked since it got activated.
5) Other stuff - I am not touching in critical section definition, notifications, event groups as well as multicore solutions since I have not used them yet in any project of mine. They are explained well in the RTOS official documentation.

#### Debugging
If we decide to set up the RTOS using the IDE, it will be possible to open the ST version of an RTOS debugger without any fuss – if in the config the right functions are enabled. These will be just an extra set of windows (so they will be in Window/Show view/Other/FreeRTOS) with some basic information on the RTOS functions and tasks. I have found these windows useful initially, though they were set on what information they were presenting so I have eventually decided to close them off.

I recommend writing an additional debug task instead and call all the functions manually using the task handles (which are pointer to the tasks set in the task definition “xTaskCreate”). This debug task will then collect all the desired debugging data into some global variables, making them available at our leisure. Of note, don’t forget to enable hooks, timers, handles and debug functions for RTOS using either the IDE or directly the config file. Failing to do so will NOT involve these functions within the code.

### Punishment by RTOS
As much as I see the utility in RTOS, I am very much advising against it, mostly due to its construction. Since it is not an integral part of the ST environment, we can’t rely on the debugging tools to help us out as much as we might be used to running superloops.

This is most apparent in the memory allocation profile of the RTOS. Yes, there are multiple ways to track the heap usage as well as to indicate a stack overflow, but none of them come by naturally. Hard faults due to memory issues are a commonality and are complicated to debug (Pro tip: use Fault analyser window, the Build analyser and memory tracking to see, where the stack pointer is pointing at the time of crashing). Similarly, the heap size necessary to run a task may not be defined properly during task definition, though heap watermark checking and heap size redefinition could be done to deal with that. (Ideally, just pick an MCU with abundantly available RAM and overkill the heap demands for everything at your leisure.)

I also find it difficult to exactly pinpoint, which functions or actions are reentrant and which ones are not. For instance, running an I2C from two tasks at the same time could lead to code freezing or hard fault due to peripheral crash. Of course, the solution is to dereference any kind of peripheral into its own task and then feed it information via a queue, but still…One may have difficulties figuring out, what functions are reentrant.

Similarly, one has to be very careful over how variables are defined. Ideally, all variables should be “static”, their place in RAM already assigned and not tinkered with by the RTOS. Failing to do that will likely lead to “Hard Fault” on the device since the variable is de-allocated by the RTOS on the fly, making any pointer to them just go into nothingness.

Preferably, any kind of “extern” variable (global variable) should be replaced by a queue to allow safe transition between tasks or interrupts. If not done so, the global variable might be update while another task is also polling it, resulting in undesired outcomes. In general, global variables are “reentrant” though, their place in memory assigned by the compiler, not by the RTOS.

I also had a bad time defining local variables within an interrupt while running an RTOS. Apparently, the stack gets all messed up if we allow the interrupts as well as the tasks to access the stack in a dynamic manner. Once I have removed local variables from the interrupt routines, things started to work as intended.

On the topic of task execution, it must not be forgotten that only “blocked” or “suspended” tasks allow lower priority tasks to execute. If a higher priority task is polling continuously something, it will not release the CPU for lower priority tasks otherwise. One can thus use a semaphore to block the higher level task or just add a vTaskDelay function into the polling “while” loop.

Within an interrupt routine, all RTOS APIs have their own specific version. Also, they do not take into effect immediately, unless we force the scheduler to “yield” runtime to a task that has been released by the ISR.

Lastly, in case you want to start playing with hardware interrupts alongside the RTOS, any hardware interrupt interfacing with RTOS must be of lower priority than what is being set up within the config header at “configMAX_SYSCALL_INTERRUPT_PRIORITY”, or the interrupt would clash and not execute properly.

Do not ever forget to put the ISR specific APIs into the IRQ Handler, by the way…

## User guide
I ended up integrating the RTOS into the project through the IDE (pick Middleware and RTOS) instead of importing it manually. I know this is lazy of me, but I couldn’t be bothered to do this. When configuring the RTOS, we should also enable the desired functions in the “config parameters” and “include parameters” section in the CubeMx. “NEWLIB” should also be made reentrant in “Advanced settings”. At any rate, one will have to remove the automatically generated code: we don’t actually need the Kernel Start and Kernel Init functions (kernel start calls the start scheduler, while init simply fills up the CubeMX ‘s own struct), nor do we need any kind of task definition (the imported RTOS will always define a default task that we can’t remove).

Regarding the tasks, I have reworked the pixelBanger so it would only generate one output. More precisely, we have three different tasks running at the same time:
1) Task 1 publishes the pixels on the strip every 30 ms. It takes a mutex when it publishes the source matrix. Task 1 runs at the same priority as Task 2. The mutex prevents the publishing the occur the same time we are updating the source matrix.
2) Task 2 updates the source matrix every 100 ms. It runs at the same priority as Task 1 and takes a mutex whenever it is about to update the source matrix. The update can be either filling the strip with a colour, then changed it dynamically or drain it down to zero, effectively resetting the LEDs.
3) Task 3 checks the control interface for the neopixel strip. We have two buttons which are polled for deciding, which way Task 2 should change the source matrix. We poll the button states twice with 2 ms apart to include a crude button debouncing. Pushing the first button (the one that is on the blue push button on the nucleo) will generate a global flag to fill the strip, the other button (must be installed externally to the nucleo) will drain the strip.
4) Task 4 is the debug task, collecting information on the other 3 tasks as well as the runtime and the heap usage.

## Conclusion
This project is mostly just a recap on RTOS for my own documentation purposes. RTOS isn’t that complicated to use and implement, albeit comes with a massive number of pitfalls. It also brings additional risks when running on lower and MCUs, such as the L053R8, so tread lightly.

