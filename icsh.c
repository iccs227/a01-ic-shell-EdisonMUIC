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
#define JOBSIZE 100

pid_t foregroundPid = -1;
int lastExitStatus = 0;

typedef struct {
    int id;
    pid_t pid;
    char command[MAX_CMD_BUFFER];
    int isRunning;
} Job;

Job jobList[JOBSIZE];
int jobCount = 0;
int nextJobId = 1;

void addJob(pid_t pid, char* cmd) {
    int added = 0;
    for (int i = 0; i < jobCount; i++) {
        if (jobList[i].pid == pid) {added = 1;}
    }
    if (jobCount < JOBSIZE && added == 0) {
        jobList[jobCount].id = nextJobId;
        nextJobId++;
        jobList[jobCount].pid = pid;
        strncpy(jobList[jobCount].command, cmd, MAX_CMD_BUFFER);
        jobList[jobCount].isRunning = 0;
        jobCount++;
    }
}

void removeJob(pid_t pid) {
    for (int i = 0; i < jobCount; i++) {
        if (jobList[i].pid == pid) {
            for (int j = i; j < jobCount - 1; ++j) {
                jobList[j] = jobList[j + 1];
            }
            jobCount--;
            break;
        }
    }
}

void listJobs() {
    for (int i = 0; i < jobCount; i++) {
        int cmdLen = strlen(jobList[i].command);
        if (jobList[i].command[cmdLen - 1] != '&') {strcat(jobList[i].command, " &");}
        if (jobList[i].isRunning == 1) {printf("Running [%d] %d %s\n", jobList[i].id, jobList[i].pid, jobList[i].command);}
        else if (jobList[i].isRunning == 0) {printf("Stopped [%d] %d %s\n", jobList[i].id, jobList[i].pid, jobList[i].command);}
    }
}

Job* findJob(int id) {
    for (int i = 0; i < jobCount; i++) {
        if (jobList[i].id == id) {return &jobList[i];}
    }
    return NULL;
}

Job* findJobByPid(pid_t pid) {
    for (int i = 0; i < jobCount; i++) {
        if (jobList[i].pid == pid) {return &jobList[i];}
    }
    return NULL;
}

void signalHandler(int sig) {
    if (sig == SIGINT) {
        if (foregroundPid > 0) {kill(-foregroundPid, SIGINT);}
        else {printf("\n");}
    }
    else if (sig == SIGTSTP) {
        if (foregroundPid > 0) {kill(-foregroundPid, SIGTSTP);}
        else {printf("\n");}
    }
    else if (sig == SIGCHLD) {
        int status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {removeJob(pid);}
    }
}

void externalCommandFunc(char* buffer, char* preBuffer) {
    char *argv[MAX_CMD_BUFFER];
    int argc = 0;

    char temp[MAX_CMD_BUFFER];
    strcpy(temp, buffer);

    char *inputFile = NULL;
    char *outputFile = NULL;

    int isBackground = 0;
    char *argument = strtok(buffer, " ");
    while (argument != NULL && argc < MAX_CMD_BUFFER - 1) {
        if (strcmp(argument, "&") == 0) {
            isBackground = 1;
        }
        else if (strcmp(argument, "<") == 0) {
            argument = strtok(NULL, " ");
            if (argument != NULL) {inputFile = argument;}
            else {printf("Error: Empty argument after <\n"); return;}
        }
        else if (strcmp(argument, ">") == 0) {
            argument = strtok(NULL, " ");
            if (argument != NULL) {outputFile = argument;}
            else {printf("Error: Empty argument after >\n"); return;}
        }
        else {
            argv[argc] = argument;
            argc++;
        }
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
        setpgid(0, 0);
        if (inputFile != NULL) {
            int inputFd = open(inputFile, O_RDONLY);
            if (inputFd < 0) {
                perror("Input file open failed");
                exit(1);
            }
            dup2(inputFd, STDIN_FILENO);
            close(inputFd);
        }

        if (outputFile != NULL) {
            int outputFd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (outputFd < 0) {
                perror("Output file open failed");
                exit(1);
            }
            dup2(outputFd, STDOUT_FILENO);
            close(outputFd);
        }

        execvp(argv[0], argv);
        printf("bad command\n");
        exit(1);
    }
    setpgid(pid, pid);
    int w = 0;
    int status;
    if (isBackground) {
        printf("[%d] %d\n", nextJobId, pid);
        addJob(pid, temp);
    }
    else {
        foregroundPid = pid;
        tcsetpgrp(STDIN_FILENO, foregroundPid);
        w = waitpid(foregroundPid, &status, WUNTRACED);
        tcsetpgrp(STDIN_FILENO, getpid());
        foregroundPid = -1;
    }
    foregroundPid = -1;

    if (WIFEXITED(status)) {lastExitStatus = WEXITSTATUS(status);}
    else if (WIFSTOPPED(status) && w != -1) {
        Job* job = findJobByPid(pid);
        if (job) {job->isRunning = 0;}
        else {addJob(pid, temp);}
        lastExitStatus = WSTOPSIG(status);
    }
    else if (WIFSIGNALED(status)) {lastExitStatus = WTERMSIG(status);}
    else {lastExitStatus = 1;}
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
    else if (strcmp(buffer, "jobs") == 0) {
        listJobs();
    }
    else if (strncmp(buffer, "fg %", 4) == 0) {
        int jobId = atoi(buffer + 4);
        Job* job = findJob(jobId);
        if (job != NULL) {
            kill(-job->pid, SIGCONT);
            job->isRunning = 1;

            foregroundPid = job->pid;
            tcsetpgrp(STDIN_FILENO, job->pid);

            int status;
            waitpid(job->pid, &status, WUNTRACED);

            tcsetpgrp(STDIN_FILENO, getpid());
            foregroundPid = -1;

            if (WIFEXITED(status) || WIFSIGNALED(status)) {removeJob(job->pid);}
            else if (WIFSTOPPED(status)) {job->isRunning = 0;}

            if (WIFEXITED(status)) {lastExitStatus = WEXITSTATUS(status);}
            else if (WIFSIGNALED(status)) {lastExitStatus = WTERMSIG(status);}
            else if (WIFSTOPPED(status)) {lastExitStatus = WSTOPSIG(status);}
            else {lastExitStatus = 1;}
        }
        else {printf("Can't find job with the provided ID.\n");}
    }
    else if (strncmp(buffer, "bg %", 4) == 0) {
        int id = atoi(buffer + 4);
        Job* job = findJob(id);
        if (job && job->isRunning == 0) {
            kill(-job->pid, SIGCONT);
            job->isRunning = 1;
            printf("Resumed [%d] %d %s\n", job->id, job->pid, job->command);
        }
        else {printf("Can't find job with the provided ID or Job is already running.\n");}
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
        externalCommandFunc(tempBuffer, preBuffer);
        strcpy(preBuffer, tempBuffer);
    }
    strcpy(preBuffer, tempBuffer);

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        removeJob(pid);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTSTP, signalHandler);
    signal(SIGCHLD, signalHandler);

    char buffer[MAX_CMD_BUFFER];
    char preBuffer[MAX_CMD_BUFFER] = "";
    int exitCode = 0;

    pid_t shellpgid = getpid();
    setpgid(shellpgid, shellpgid);
    tcsetpgrp(STDIN_FILENO, shellpgid);
    signal(SIGTTOU, SIG_IGN);

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
