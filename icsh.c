/* ICCS227: Project 1: icsh
 * Name: Weilong Xu
 * StudentID: 6581036
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_CMD_BUFFER 255
#define BUFSIZE 1024

int commandFunc(char* buffer, char* preBuffer) {
    buffer[strcspn(buffer, "\r\n")] = '\0';

    if (strlen(buffer) == 0) {
        return 0;
    }
    else if (strncmp(buffer, "echo ", 5) == 0) {
        char *strToPrint = buffer + 5;
        printf("%s\n", strToPrint);
    }
    else if (strncmp(buffer, "!!", 2) == 0 && strlen(buffer) == 2) {
        if (strncmp(preBuffer, "echo ", 5) == 0) {
            char *strToPrint = preBuffer + 5;
            printf("%s\n", preBuffer);
            printf("%s\n", strToPrint);
            return 0;
        }
    }
    else if (strncmp(buffer, "exit ", 5) == 0 && strlen(buffer) >= 6) {
        int code = atoi(buffer + 5) & 0xFF;
        printf("bye\n");
        exit(code);
    }
    else {
        printf("bad command\n");
    }
    strcpy(preBuffer, buffer);
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
