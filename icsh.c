/* ICCS227: Project 1: icsh
 * Name: Weilong Xu
 * StudentID: 6581036
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CMD_BUFFER 255
#define BUFSIZE 1024

pid_t foregroundPid = -1;
int lastExitStatus = 0;

void sigIntHandler(int sig) {
    if (foregroundPid > 0) {kill(foregroundPid, SIGINT); printf("\n");}
    else {printf("\n");}
}

void sigStpHandler(int sig) {
    if (foregroundPid > 0) {kill(foregroundPid, SIGTSTP); printf("\n");}
    else {printf("\n");}
}

void externalCommandFunc(char* buffer, char* preBuffer) {
    char *argv[MAX_CMD_BUFFER];
    int argc = 0;

    char *argument = strtok(buffer, " ");
    while (argument != NULL && argc < MAX_CMD_BUFFER - 1) {
        argv[argc] = argument;
        argc++;
        argument = strtok(NULL, " ");
    }
    argv[argc] = NULL;

    if (argc == 0) return;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        execvp(argv[0], argv);
        printf("bad command\n");
        lastExitStatus = 1;
        exit(1);
    }


    foregroundPid = pid;
    int status;
    waitpid(foregroundPid, &status, WUNTRACED);
    foregroundPid = -1;

    if (WIFEXITED(status)) { lastExitStatus = WEXITSTATUS(status); }
    else if (WIFSTOPPED(status)) { lastExitStatus = WSTOPSIG(status); }
    else if (WIFSIGNALED(status)) { lastExitStatus = WTERMSIG(status); }
    else { lastExitStatus = 1; }
}

int commandFunc(char* buffer, char* preBuffer) {
    buffer[strcspn(buffer, "\r\n")] = '\0';
    char tempBuffer[MAX_CMD_BUFFER];
    strcpy(tempBuffer, buffer);

    if (strlen(buffer) == 0) {
        return 0;
    }
    else if (strcmp(buffer, "echo $?") == 0) {
        printf("%d\n", lastExitStatus);
    }
    else if (strncmp(buffer, "echo ", 5) == 0) {
        char *strToPrint = buffer + 5;
        printf("%s\n", strToPrint);
    }
    else if (strncmp(buffer, "!!", 2) == 0 && strlen(buffer) == 2) {
        if (strlen(preBuffer) != 0) {
            printf("%s\n", preBuffer);
            strcpy(tempBuffer, preBuffer);
            commandFunc(tempBuffer, preBuffer);
        }
        return 0;
    }
    else if (strncmp(buffer, "exit ", 5) == 0 && strlen(buffer) >= 6) {
        int code = atoi(buffer + 5) & 0xFF;
        printf("bye\n");
        exit(code);
    }
    else {
        strcpy(tempBuffer, buffer);
        externalCommandFunc(buffer, preBuffer);
        strcpy(preBuffer, tempBuffer);
    }
    strcpy(preBuffer, tempBuffer);
    return 0;
}

int main(int argc, char* argv[]) {
    struct sigaction sa_int = {0}, sa_tstp = {0};
    sa_int.sa_handler = sigIntHandler;
    sa_tstp.sa_handler = sigStpHandler;

    sigaction(SIGINT, &sa_int, NULL);
    sigaction(SIGTSTP, &sa_tstp, NULL);

    char buffer[MAX_CMD_BUFFER];
    char preBuffer[MAX_CMD_BUFFER] = "";
    int exitCode = 0;

    printf("Starting IC shell\n");
    printf("Welcome to ICSH by Edison\n");

    if (argc == 2) {
        int inFd = open(argv[1], O_RDONLY);
        char fileBuffer[BUFSIZE];

        ssize_t numRead;
        int currentPos = 0;

        while ((numRead = read(inFd, fileBuffer, BUFSIZE)) > 0) {
            for (ssize_t i = 0; i < numRead; i++) {
                if (fileBuffer[i] == '\n' || currentPos >= MAX_CMD_BUFFER - 1) {
                    buffer[currentPos] = '\0';
                    commandFunc(buffer, preBuffer);
                    currentPos = 0;
                } else {
                    buffer[currentPos++] = fileBuffer[i];
                }
            }
        }

        if (currentPos > 0) {
            buffer[currentPos] = '\0';
            commandFunc(buffer, preBuffer);
        }

        close(inFd);
        return 0;
    }

    while (1) {
        printf("icsh $ ");
        fflush(stdout);

        if (fgets(buffer, MAX_CMD_BUFFER, stdin) == NULL) {
            clearerr(stdin);
            continue;
        }

        commandFunc(buffer, preBuffer);
    }
}
