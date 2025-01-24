# Linux Kernel Rootkit
This program was created to learn linux kernel module programming in C. The original assignment problem is pasted below.

## Overview
In this assignment, you will implement a portion of Rootkit functionality to gain:
1. Hands-on practice with kernel programming
2. A detailed understanding of the operation of system calls within the kernel
3. Practice with fork/exec to launch child processes
4. An understanding of the types of malicious activities that attackers may attempt against a system (particularly against privileged systems programs).
Our assumption will be that via a successful exploit of a vulnerability, you have gained the ability to execute privileged code in the system. Your “attack” code will be represented by a small program that you will write, which will (among a few other things, described below) load a kernel module that will conceal the presence of your attack program as well as some of its malicious
activities. The specific functionality required of the attack program and kernel module (as well as helpful hints about implementing this functionality) are described next.

### Attack Program
Your attack program (named sneaky_process.c) will give you practice with executing system calls by calling relevant APIs (for process creation, file I/O, and receiving keyboard input from
standard input) from a user program. Your program should operate in the following steps if you
run `sudo ./sneaky_process`:
1. Your program should print its own process ID to the screen, with exactly following message (the print command in your code may vary, but the printed text should match): `printf(“sneaky_process pid = %d\n”, getpid());`
2. Your program will perform 1 malicious act. It will copy the `/etc/passwd` file (used for user authentication) to a new file: `/tmp/passwd`. Then it will open the `/etc/passwd` file and print a new line to the end of the file that contains a username and password that may allow a desired user to authenticate to the system. Note that this won’t actually allow you to authenticate to the system as the ‘sneakyuser’, but this step illustrates a type of subversive behavior that attackers may utilize. This line added to the password file should be exactly the following: `sneakyuser:abc123:2000:2000:sneakyuser:/root:bash`
3. Your program will load the sneaky module (`sneaky_mod.ko`) using the `insmod` command. Note that when loading the module, your sneaky program will also pass its process ID into the module.
4. Your program will then enter a loop, reading a character at a time from the keyboard input until it receives the character ‘q’ (for quit, you don’t need to hit carriage return or other keys after ‘q’). Then the program will exit this waiting loop. Note this step is here so that you will have a chance to interact with the system while: 1) your sneaky process is running, and 2) the sneaky kernel module is loaded. This is the point when the malicious behavior will be tested in another terminal on the same system while the process is running.
5. Your program will unload the sneaky kernel module using the `rmmod` command.
6. Your program will restore the `/etc/passwd` file (and remove the addition of “sneakyuser” authentication information) by copying `/tmp/passwd` to `/etc/passwd`.

### Sneaky Kernel Module
Your sneaky kernel module will implement the following subversive actions:
1. It will hide the “sneaky_process” executable file from both the `ls` and `find` UNIX commands. For example, if your executable file named “sneaky_process” is located in `/home/userid/hw5`:
    a. `ls /home/userid/hw5` should show all files in that directory except for “sneaky_process”.
    b. `cd /home/userid/hw5; ls` should show all files in that directory except for “sneaky_process”.
    c.  `find /home/userid -name sneaky_process”` should not return any results.
2. In a UNIX environment, every executing process will have a directory under `/proc` that is named with its process ID (e.g `/proc/1480`). This directory contains many details about the process. Your sneaky kernel module will hide the `/proc/<sneaky_process_id>` directory (note hiding a directory with a particular name is equivalent to hiding a file!). For example, if your sneaky_process is assigned process ID of 500, then:
    a. `ls /proc` should not show a sub-directory with the name “500”
    b. `ps -a -u <your_user_id>` should not show an entry for process 500 named “sneaky_process” (since the `p` command looks at the `/proc` directory to examine all executing processes).
3. It will hide the modifications to the `/etc/passwd` file that the sneaky_process made. It will do this by opening the saved `/tmp/passwd` when a request to open the `/etc/passwd` is seen. For example:
    a. `cat /etc/passwd` should return contents of the original password file without the modifications the sneaky process made to `/etc/passwd`.
4. It will hide the fact that the sneaky_module itself is an installed kernel module. The list of active kernel modules is stored in the `/proc/modules` file. Thus, when the contents of that file are read, the sneaky_module will remove the contents of the line for “sneaky_mod” from the buffer of read data being returned. For example:
    a. `lsmod` should return a listing of all modules except for the “sneaky_mod”
    
Your overall submission will be tested by compiling your kernel module and sneaky process, running the sneaky process, and then executing commands as described above (hint: in another terminal) to make sure your module is performing the intended subversive actions.