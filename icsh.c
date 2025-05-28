/* ICCS227: Project 1: icsh
 * Name: Weilong Xu
 * StudentID: 6581036
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CMD_BUFFER 255

int main() {
    char buffer[MAX_CMD_BUFFER];
    char preBuffer[MAX_CMD_BUFFER] = "";

    printf("Starting IC shell\n");
    printf("Welcome to ICSH by Edison\n");

    while (1) {
        printf("icsh $ ");
        fgets(buffer, 255, stdin);
        if (strlen(buffer) == 1) {
            continue;
        }
        else if (strncmp(buffer, "echo ", 5) == 0) {
            char *strToPrint = buffer + 5;
            printf("%s", strToPrint);
        }
        else if (strncmp(buffer, "!!", 2) == 0 && strlen(buffer) == 3) {
            if (strncmp(preBuffer, "echo ", 5) == 0) {
                char *strToPrint = preBuffer + 5;
                printf("%s", preBuffer);
                printf("%s", strToPrint);
                continue;
            }
        }
        else if (strncmp(buffer, "exit ", 5) == 0 && strlen(buffer) >= 7) {
            int code = atoi(buffer + 5) & 0xFF;
            printf("bye\n");
            exit(code);
        }
        else {
            printf("bad command\n");
        }
        strcpy(preBuffer, buffer);
    }
}
