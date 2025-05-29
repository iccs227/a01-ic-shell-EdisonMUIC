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

#define MAX_CMD_BUFFER 255
#define BUFSIZE 1024

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
        exit(1);
    }

    int status;
    wait(&status);
}

int commandFunc(char* buffer, char* preBuffer) {
    buffer[strcspn(buffer, "\r\n")] = '\0';
    char tempBuffer[MAX_CMD_BUFFER];
    strcpy(tempBuffer, buffer);

    if (strlen(buffer) == 0) {
        return 0;
    }
    else if (strncmp(buffer, "echo ", 5) == 0) {
        char *strToPrint = buffer + 5;
        printf("%s\n", strToPrint);
    }
    else if (strncmp(buffer, "!!", 2) == 0 && strlen(buffer) == 2) {
        printf("%s\n", preBuffer);
        strcpy(tempBuffer, preBuffer);
        commandFunc(tempBuffer, preBuffer);
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
        fgets(buffer, 255, stdin);
        commandFunc(buffer, preBuffer);
    }
}
