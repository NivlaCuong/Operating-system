/**
 *      SE 149: Operating System - Ben Reed
 *      Author: Cuong (Calvin) Nguyen
 *
 *      forAll is the program that will run multiple commands.
 *      To run the program: "gcc -o forAll forAll.c && ./forAll <command name> <...files that you want the run the command on>"
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

static pid_t globalPid;
void usrHandler(int sig, siginfo_t *si, void *ignore);
void initSignalHandler();

// main function that will store the logic of the program and will run the commands
int main(int argc, char **argv) {
    
    // Init all files
    char fileName[1000] = "";
    char cmdText[1000] = "";
    char statusCodeText[1000] = "";
    char signalText[1000] = "";
    mode_t mode = S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH;
    
    initSignalHandler();
    
    // Run command for all arguments
    for(int i = 2; i < argc; i++) {
        
        // Open A File
        sprintf(fileName, "%d", i - 1);
        strcat(fileName, ".out");
        int outfd = open(fileName, O_CREAT | O_TRUNC | O_WRONLY, mode);
        
        globalPid = fork();
        
        if (globalPid < 0) {
            fprintf(stderr, "%s\n", "Fork"); 
            exit(EXIT_FAILURE);
        }
                
        else if (globalPid == 0) {      // Child Process -> All the child will execute the command on each file
            sprintf(cmdText, "Executing %s %s\n", argv[1], argv[i]);
            write(outfd, cmdText, strlen(cmdText));

            if (!outfd) {
                perror("open");
                return EXIT_FAILURE;
            }
            
            dup2(outfd, 1);     // redirect stdout to FileDescriptor
            dup2(outfd, 2);     // Error redirect to stdout

            execlp(argv[1], argv[1], argv[i], NULL);
            
        } else {    // Parent Process
            int status;
            wait(&status);
            
            if (status == 0) {
                sprintf(statusCodeText, "Finished Executing %s %s exit code = %d\n", argv[1], argv[i], status);
                write(outfd, statusCodeText, strlen(statusCodeText));  
            } else if (status == 2) {
                sprintf(signalText, "Stopped executing %s %s signal = %d\n", argv[1], argv[i], status);
                write (outfd, signalText, strlen(signalText));
            }
        }
        
        close(outfd);
    }
    return 0;
}

// Initialize the Sigaction
void initSignalHandler() {
    static struct sigaction sa;
    
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = usrHandler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("SigAction SIGINT FAILED");
        exit(errno);
    }
    if (sigaction(SIGQUIT, &sa, NULL) == -1) {
        perror("Sig Action SIGQUIT FAILE");
        exit(errno);
    }
}

// usrHandler will check for SIGINT and SIGQUIT when the user hit "crtl-c" or "ctrl-\"
void usrHandler(int sig, siginfo_t *si, void *ignore) {
    if(sig == SIGINT) {
        printf("Signaling %d \n", globalPid);
    } else if (sig == SIGQUIT) {
        printf("Signaling %d\nExiting due to quit signal\n", globalPid);
        exit(EXIT_FAILURE);
    }
}
