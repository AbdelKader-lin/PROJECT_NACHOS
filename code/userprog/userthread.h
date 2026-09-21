#ifndef USERTHREAD_H
#define USERTHREAD_H

// typedef struct _UserThreadArgs {
//     void (*f)(void *);
//     void *arg;
// } UserThreadArgs;

int do_UserThreadCreate(int f, int arg);
int do_UserThreadExit();
void do_WaitUserThreads();
void do_UserThreadJoin(unsigned int threadId);
void do_ForkExec(char *filename);
int do_Exec(char *filename);
int do_Join(int spaceId);
int do_ProcessExit(int status);

#endif // USERTHREAD_H