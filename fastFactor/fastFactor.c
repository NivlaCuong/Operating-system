#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int fastFactor(long long int num);

int main(int argc, char **argv) {
    long long int val;
    char *eptr;
    
    if (argc == 1) { // Read from file
        char *ptr;
        size_t n = 0;
        int res = 0;
        while (getline(&ptr, &n, stdin) > 0) {
            for(int j = strlen(ptr); j > 0; j--) {
                if (ptr[j] > 57 || ptr[j] < 48) {
                    printf("Error not a number \n");
                    res = -1;
                    break;
                }
            }
            if (res == -1) {
                break;
            }
            if (res == 0) {
                val = strtoll(ptr, &eptr, 10);
                fastFactor(val);    
            }
            
        }
    } else {
        for (int i = 1; i < argc; i++) {
            
            val = strtoll(argv[i], &eptr, 10);
            fastFactor(val);
        }    
    }
} 


int fastFactor(long long int val) {

    if (val == 0) {
        printf("%llu: [Error] This number is invalid. Cannot enter 0 \n", val);
        return -1;
    }
    if (val < 0) {
        printf("%llu: [Error] This number is invalid. Cannot enter a negative number \n", val);
        return -1;
    }


    char *endptr;
    pid_t pid[3];

    long long int halfValue = val/2;

    int fd1[2];  // first child pipe
    int fd2[2];  // second child pipe
    int fd3[2];  // third child pipe


    if (pipe(fd1) == -1) {
        fprintf (stderr, "Pipe 1 failed.\n");
        exit(0);
    }
    if (pipe(fd2) == -1) {
        fprintf(stderr, "Pipe 2 failed.\n");    
        exit(0);
    }

    if (pipe(fd3) == -1) {
        fprintf(stderr, "Pipe 3 failed.\n");    
        exit(0);
    }

    for(int i = 0; i < 3; i++) {
        pid[i] = fork();
        char mesg[1000] = "";
        char res[1000] = "";
        long long int *num;
        // long long int *res;
        if(pid[i] == 0 && i == 0) { 
            for (long long int j = 1; j <= halfValue / 3; j++) { 
                if (val % j == 0) {
                    sprintf(res, "%llu ", j);
                    strcat(mesg, res);
                }    
            }
            close(fd1[0]);
            write(fd1[1], mesg, 1000); 
            close(fd1[1]);
            exit(0); 
        } 

        else if(pid[i] == 0 && i == 1) { 
        
            for (long long int j = halfValue / 3 + 1; j <= 2 * (halfValue / 3); j++) { 
                if (val % j == 0) {
                    sprintf(res, "%llu ", j);
                    strcat(mesg, res);
                }    
            }

            close(fd2[0]);
            write(fd2[1], mesg, 1000); 
            close(fd2[1]);
            exit(0); 
        } 

        else if(pid[i] == 0 && i == 2) { 
            for (long long int j = 2*(halfValue / 3) + 1; j <= halfValue; j++) { 
                if (val % j == 0) {
                    sprintf(res, "%llu ", j);
                    strcat(mesg, res);
                }    
            }
            close(fd3[0]);
            write(fd3[1], mesg, 1000); 
            close(fd3[1]);
            exit(0); 
        } 

    }

    char mesg[16] = "";

    if (getpid() > 0) {
        for(int i = 0; i < 3; i++) { 
            wait(NULL); 
        }

        printf("%llu: ", val);

        close(fd1[1]);
        close(fd2[1]);
        close(fd3[1]);

        read(fd1[0], mesg, 100);
        printf("%s ",mesg);

        read(fd2[0], mesg, 100);
        printf("%s ",mesg);

        read(fd3[0], mesg, 100);
        printf("%s ",mesg);
        printf("%lld \n", val);

        close(fd1[0]);
        close(fd2[0]);
        close(fd3[0]);
    }
    
    return 0;

}
