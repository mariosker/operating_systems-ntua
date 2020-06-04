#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

struct procedure {  //τη δημιουργήσαμε εμείς

  int id;
  int pid;
  int priority; /*0=LOW,1=HIGH*/
  char name[TASK_NAME_SZ];
  struct procedure *next;
};

struct procedure *head_low, *tail_low, *head_high, *tail_high;
int max_id;
int killed_from_shell = 0;
int wait_pid;
int shell_is_alive = 1;
int high_count = 0;
int low_count = 0;

void *safe_malloc(size_t size) {
  void *p;

  if ((p = malloc(size)) == NULL) {
    fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n", size);
    exit(1);
  }

  return p;
}

void child(char *name)  // given
{
  raise(SIGSTOP);  // stop for the first time

  // char executable[] = "prog";
  char *newargv[] = {name, NULL, NULL, NULL};
  char *newenviron[] = {NULL};

  printf("About to replace myself with the executable %s...\n", name);

  execve(name, newargv, newenviron);

  /* execve() only returns on error */
  perror("execve");
  exit(1);
}

/* Print a list of all tasks currently being scheduled.  */
static void sched_print_tasks(void)  // we created it
{
  struct procedure *temp = head_low;

  printf("High count=%d  Low count=%d\n", high_count, low_count);

  printf("LOW LIST\n");
  if (temp != NULL) {
    if (high_count == 0) printf("Current procedure:");
    for (; temp != NULL; temp = temp->next) {
      printf("Name=%s | Id=%d | Pid=%d | Priority=%d\n", temp->name, temp->id,
             temp->pid, temp->priority);
    }
  } else {
    printf("----\n");
  }

  temp = head_high;
  printf("HIGH LIST\n");
  if (temp != NULL) {
    printf("Current procedure:");
    for (; temp != NULL; temp = temp->next) {
      printf("Name=%s | Id=%d | Pid=%d | Priority=%d\n", temp->name, temp->id,
             temp->pid, temp->priority);
    }
  } else {
    printf("----\n");
  }
}

/*
 *Set the priority to HIGH for a task determined by the value of its
 * scheduler-specific id.
 */
static int sched_set_high_prio_by_id(int id)  // we created it
{
  struct procedure *temp = head_low, *prev = NULL;
  for (; temp != NULL; prev = temp, temp = temp->next) {
    if (temp->id == id) {
      if (prev == NULL && strcmp(head_low->name, SHELL_EXECUTABLE_NAME) == 0) {
        printf("Tried to change the priority on myself.Impossible\n");
        return -1;
      }

      if (temp->priority == 1) {
        printf("Priority already high.Nothing to be done.\n");
        return -1;
        // should never happen
      } else {
        temp->priority = 1;
        printf("Set the priority to high to proc %s,id=%d,pid=%d\n", temp->name,
               temp->id, temp->pid);

        // remove it from the low list
        if (temp == head_low) {
          head_low = head_low->next;
          low_count--;
        } else {
          prev->next = temp->next;
          if (temp == tail_low) tail_low = prev;
          low_count--;
        }

        // and move it to the high list;
        if (high_count == 0) {     // high is empty, move temp and shell
          head_low->priority = 1;  // change the priority on the schell
          head_high = head_low;
          tail_high = head_low;
          head_low = head_low->next;  // next should always exist as we are
                                      // upping another proc
          // in the worst case it becomes NULL

          // also move temp to high
          tail_high->next = temp;
          tail_high = temp;

          tail_high->next = NULL;
          // and remove the schell from low
          high_count += 2;
          low_count--;
        } else {  // high is not empty,move only temp
          tail_high->next = temp;
          tail_high = temp;
          tail_high->next = NULL;
          high_count++;
        }
      }
      return 1;  // return success
    }
  }

  return -1;  // return failure
}

/*
 *Set the priority to LOW for a task determined by the value of its
 * scheduler-specific id.
 */
static int sched_set_low_prio_by_id(int id)  // we created it
{
  struct procedure *temp = head_high, *prev = NULL;

  for (; temp != NULL; prev = temp, temp = temp->next) {
    if (temp->id == id) {
      if (prev == NULL && strcmp(head_high->name, SHELL_EXECUTABLE_NAME) == 0) {
        printf("Tried to change the priority on myself.Impossible\n");
        return -1;
      }

      if (temp->priority == 0) {
        printf("Priority already low.Nothing to be done.\n");
        return -1;  // should never happen
      } else {
        temp->priority = 0;
        printf("Set the priority to low to proc %s,id=%d,pid=%d\n", temp->name,
               temp->id, temp->pid);

        // move it to the low list
        // if the shell is the only in the high list move him as well

        // remove it from the high list
        if (temp == head_high) {
          head_high = head_high->next;
          high_count--;
        } else {
          prev->next = temp->next;
          if (temp == tail_high) tail_high = prev;
          high_count--;
        }

        // and move it to the low list;
        if (low_count == 0 &&
            high_count == 1) {  // low is empty, only shell in high
          head_high->priority = 0;
          head_low = head_high;  // shell is head_high
          tail_low = head_high;

          head_high = head_high->next;  // next should always exist as we are
                                        // upping another proc
          // in the worst case it becomes NULL

          // also move temp to low
          tail_low->next = temp;
          tail_low = temp;

          tail_low->next = NULL;
          high_count--;
          low_count += 2;
        } else if (low_count == 0) {  // low is empty,more than shell in high
          head_low = temp;
          tail_low = temp;
          head_low->next = NULL;
          low_count++;
        } else if (high_count == 1) {  // low is not empty, only shell in high
          head_high->priority = 0;
          head_high->next = head_low;
          head_low = head_high;
          head_high = NULL;

          tail_low->next = temp;
          tail_low = temp;
          tail_low->next = NULL;
          low_count += 2;
          high_count--;
        } else {  // low is not empty,more than shell in high
          tail_low->next = temp;
          tail_low = temp;
          tail_low->next = NULL;
          low_count++;
        }
      }

      return 1;  // return success
    }
  }

  return -1;  // return failure
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int sched_kill_task_by_id(int id)  // we created it
{
  struct procedure *temp = head_low, *prev = NULL;

  for (; temp != NULL; prev = temp, temp = temp->next) {
    if (temp->id == id) {
      if (prev == NULL && strcmp(head_low->name, SHELL_EXECUTABLE_NAME) == 0) {
        printf("Tried to kill myself.Impossible\n");
      } else {
        if (low_count == 1) {
          head_low = tail_low = NULL;
          low_count--;
          killed_from_shell = 1;
          kill(temp->pid, SIGKILL);
          free(temp);
        } else {
          if (temp == head_low) {
            head_low = head_low->next;
          } else {
            prev->next = temp->next;
            if (temp == tail_low) tail_low = prev;
          }

          low_count--;
          killed_from_shell = 1;
          kill(temp->pid, SIGKILL);
          free(temp);
        }
      }
      return 1;  // return success
    }
  }

  temp = head_high;
  prev = NULL;

  for (; temp != NULL; prev = temp, temp = temp->next) {
    if (temp->id == id) {
      if (prev == NULL && strcmp(head_high->name, SHELL_EXECUTABLE_NAME) == 0) {
        printf("Tried to kill myself.Impossible\n");
      } else {
        if (high_count == 2) {  // shell and the one we want to kill
          head_high->priority = 0;
          if (low_count == 0) {
            head_low = tail_low =
                head_high;  // move the shell to the front of the low list
            tail_low->next = NULL;
          } else {
            head_high->next = head_low;
            head_low = head_high;
          }
          head_high = tail_high = NULL;  // remove the proc from high list
          low_count++;
          high_count -= 2;
          killed_from_shell = 1;
          kill(temp->pid, SIGKILL);
          free(temp);
        } else {  // no need to move shell
          prev->next = temp->next;
          if (temp == tail_high) tail_high = prev;
          high_count--;
          killed_from_shell = 1;
          kill(temp->pid, SIGKILL);
          free(temp);
        }
      }
      return 1;  // return success
    }
  }

  return -1;  // return failure
}

/* Create a new task.  */
static void sched_create_task(char *executable) {
  struct procedure *temp;
  pid_t p;

  /*Create the new procedure*/
  printf("Parent: Creating child from shell comand...\n");
  p = fork();
  if (p < 0) {
    /* fork failed */
    perror("fork");
    exit(1);
  }
  if (p == 0) {
    /* In child process */
    raise(SIGSTOP);
    char *newargv[] = {executable, NULL, NULL, NULL};
    char *newenviron[] = {NULL};

    printf("About to replace myself with the executable %s...\n", executable);

    execve(executable, newargv, newenviron);

    /* execve() only returns on error */
    perror("execve");
    exit(1);
  }

  /*Add the new procedure to the list of the scheduler*/
  temp = safe_malloc(1 * sizeof(struct procedure));
  temp->id = max_id;  // could also do a search
  max_id++;
  temp->priority = 0;
  temp->pid = p;  // might change it
  temp->next = NULL;
  strcpy(temp->name, executable);
  low_count++;

  if (head_low == NULL) {  // if low list is empty
    head_low = temp;
    tail_low = temp;
  } else {
    tail_low->next = temp;
    tail_low = temp;
  }
}

/* Process requests by the shell.  */
static int process_request(
    struct request_struct *rq)  // changed HIGH,LOW from given
{
  switch (rq->request_no) {
    case REQ_PRINT_TASKS:
      sched_print_tasks();
      return 0;

    case REQ_KILL_TASK:
      return sched_kill_task_by_id(rq->task_arg);

    case REQ_EXEC_TASK:
      sched_create_task(rq->exec_task_arg);
      return 0;

    case REQ_HIGH_TASK:
      return sched_set_high_prio_by_id(rq->task_arg);

    case REQ_LOW_TASK:
      return sched_set_low_prio_by_id(rq->task_arg);

    default:
      return -ENOSYS;
  }
}

/*
 * SIGALRM handler
 */

static void sigalrm_handler(int signum)  // we created it
{
  if (signum != SIGALRM) {
    fprintf(stderr, "Internal error: Called for signum %d, not SIGALRM\n",
            signum);
    exit(1);
  }

  if (high_count > 0) {
    printf("About to stop procedure %s:id=%d:pid=%d\n", head_high->name,
           head_high->id, head_high->pid);
    kill(head_high->pid, SIGSTOP);
  } else {
    printf("About to stop procedure %s:id=%d:pid=%d\n", head_low->name,
           head_low->id, head_low->pid);
    kill(head_low->pid, SIGSTOP);
  }
}

/*
 * SIGCHLD handler
 */
static void sigchld_handler(int signum)  // we created it
{
  pid_t p;
  int status;

  if (signum != SIGCHLD) {
    fprintf(stderr, "Internal error: Called for signum %d, not SIGCHLD\n",
            signum);
    exit(1);
  }

  /*
   * Something has happened to one of the children.
   * We use waitpid() with the WUNTRACED flag, instead of wait(), because
   * SIGCHLD may have been received for a stopped, not dead child.
   *
   * A single SIGCHLD may be received if many processes die at the same time.
   * We use waitpid() with the WNOHANG flag in a loop, to make sure all
   * children are taken care of before leaving the handler.
   */
  for (;;) {
    p = waitpid(-1, &status, WUNTRACED | WNOHANG);
    if (p < 0) {
      perror("waitpid");
      exit(1);
    }
    if (p == 0) break;

    explain_wait_status(p, status);

    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      /* A child has died */
      if (!killed_from_shell) {
        // always a HEAD will die here
        if (head_high != NULL) {  // got elements in high
          if (p == head_high->pid) {
            if (strcmp(head_high->name, SHELL_EXECUTABLE_NAME) == 0)
              shell_is_alive = 0;
            struct procedure *temp;
            temp = head_high;
            head_high = head_high->next;
            free(temp);  // remove HEAD from high and see if empty
            high_count--;

            if (shell_is_alive) {      // if shell alive
              if (high_count == 1) {   // if shell alone in high, go to low
                if (low_count == 0) {  // only shell in high,low empty
                  head_low = head_high;
                  tail_low = head_high;
                } else {
                  tail_low->next = head_high;
                  tail_low = head_high;
                }
                head_high = tail_high = NULL;
                low_count++;
                high_count--;
              }
            } else {  // if shell already dead
              ;       // nothing
            }

            temp = head_high;  // temp->which proc will continue
            if (high_count == 0 && low_count == 0) {  // no procs left
              printf("Nothing more to do.Exiting\n");
              exit(0);
            } else if (high_count == 0) {  // no procs in high, wake head_low
              head_high = tail_high = NULL;
              temp = head_low;
            }
            if (alarm(SCHED_TQ_SEC) < 0) {
              perror("alarm");
              exit(1);
            }
            wait_pid = temp->pid;
            kill(temp->pid, SIGCONT);
          }
        }
        if (head_low != NULL) {  // procs in low
          if (p == head_low->pid) {
            struct procedure *temp;
            temp = head_low;
            head_low = head_low->next;
            free(temp);  // remove HEAD from low and see if empty
            low_count--;

            if (low_count == 0) {  // we know that high is already empty
              printf("Nothing more to do.Exiting\n");
              exit(0);
            }
            if (alarm(SCHED_TQ_SEC) < 0) {
              perror("alarm");
              exit(1);
            }
            wait_pid = head_low->pid;
            kill(head_low->pid, SIGCONT);
          }
        }

        printf("Parent: Received SIGCHLD, child is dead.\n");
      } else {                  // if the kill was a result from the shell
        killed_from_shell = 0;  // do nothing
      }
    }
    if (WIFSTOPPED(status)) {
      // add again to the end of the list
      /* A child has stopped due to SIGSTOP/SIGTSTP, etc... */
      if (wait_pid != p) {
        return;
      }
      printf("Parent: Child has been stopped.\n");
      if (high_count > 0) {
        tail_high->next = head_high;
        head_high = head_high->next;
        tail_high = tail_high->next;
        tail_high->next = NULL;
        /* Arrange for an alarm after tq sec */
        if (alarm(SCHED_TQ_SEC) < 0) {
          perror("alarm");
          exit(1);
        }
        wait_pid = head_high->pid;
        kill(head_high->pid, SIGCONT);
      } else {
        tail_low->next = head_low;
        head_low = head_low->next;
        tail_low = tail_low->next;
        tail_low->next = NULL;
        /* Arrange for an alarm after tq sec */
        if (alarm(SCHED_TQ_SEC) < 0) {
          perror("alarm");
          exit(1);
        }
        wait_pid = head_low->pid;
        kill(head_low->pid, SIGCONT);
      }
    }
  }
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void signals_disable(void)  //δίνεται
{
  sigset_t sigset;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  sigaddset(&sigset, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
    perror("signals_disable: sigprocmask");
    exit(1);
  }
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void signals_enable(void)  // given
{
  sigset_t sigset;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  sigaddset(&sigset, SIGCHLD);
  if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
    perror("signals_enable: sigprocmask");
    exit(1);
  }
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void install_signal_handlers(void)  // given
{
  sigset_t sigset;
  struct sigaction sa;

  sa.sa_handler = sigchld_handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGCHLD);
  sigaddset(&sigset, SIGALRM);
  sa.sa_mask = sigset;
  if (sigaction(SIGCHLD, &sa, NULL) < 0) {
    perror("sigaction: sigchld");
    exit(1);
  }

  sa.sa_handler = sigalrm_handler;
  if (sigaction(SIGALRM, &sa, NULL) < 0) {
    perror("sigaction: sigalrm");
    exit(1);
  }

  /*
   * Ignore SIGPIPE, so that write()s to pipes
   * with no reader do not result in us being killed,
   * and write() returns EPIPE instead.
   */
  if (signal(SIGPIPE, SIG_IGN) < 0) {
    perror("signal: sigpipe");
    exit(1);
  }
}

static void do_shell(char *executable, int wfd, int rfd)  // given
{
  char arg1[10], arg2[10];
  char *newargv[] = {executable, NULL, NULL, NULL};
  char *newenviron[] = {NULL};

  sprintf(arg1, "%05d", wfd);
  sprintf(arg2, "%05d", rfd);
  newargv[1] = arg1;
  newargv[2] = arg2;

  raise(SIGSTOP);
  execve(executable, newargv, newenviron);

  /* execve() only returns on error */
  perror("scheduler: child: execve");
  exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static pid_t sched_create_shell(char *executable, int *request_fd,
                                int *return_fd) {
  pid_t p;
  int pfds_rq[2], pfds_ret[2];

  if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
    perror("pipe");
    exit(1);
  }

  p = fork();
  if (p < 0) {
    perror("scheduler: fork");
    exit(1);
  }

  if (p == 0) {
    /* Child */
    close(pfds_rq[0]);
    close(pfds_ret[1]);
    do_shell(executable, pfds_rq[1], pfds_ret[0]);
    assert(0);
  }
  /* Parent */
  close(pfds_rq[1]);
  close(pfds_ret[0]);
  *request_fd = pfds_rq[0];
  *return_fd = pfds_ret[1];

  return p;
}

static void shell_request_loop(int request_fd, int return_fd)  // given
{
  int ret;
  struct request_struct rq;

  /*
   * Keep receiving requests from the shell.
   */
  for (;;) {
    if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
      perror("scheduler: read from shell");
      fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
      break;
    }

    signals_disable();
    ret = process_request(&rq);
    signals_enable();

    if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
      perror("scheduler: write to shell");
      fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  int nproc, i;
  pid_t p, shell_pid;
  struct procedure *temp;

  nproc = argc - 1; /* number of proccesses goes here */
  if (nproc == 0) {
    fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
    exit(1);
  }

  /* Two file descriptors for communication with the shell */
  static int request_fd, return_fd;

  /* Create the shell. */
  shell_pid =
      sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);

  /*
   * For each of argv[1] to argv[argc - 1],
   * create a new child process, add it to the process list.
   */

  head_low = head_high = tail_low = tail_high = NULL;
  low_count = nproc + 1;

  for (i = 0; i < nproc; i++) {
    printf("Parent: Creating child...\n");
    p = fork();
    if (p < 0) {
      /* fork failed */
      perror("fork");
      exit(1);
    }
    if (p == 0) {
      /* In child process */
      child(argv[i + 1]);
      /*
       * Should never reach this point,
       * child() does not return
       */
      assert(0);
    }
    temp = safe_malloc(1 * sizeof(struct procedure));
    temp->id = i;
    temp->priority = 0; /*initially all procedure are low priority*/
    temp->pid = p;
    if (strlen(argv[i + 1]) > TASK_NAME_SZ - 1) {
      printf("Name of the task too long.Exiting...\n");
      exit(0);
    }
    strcpy(temp->name, argv[i + 1]);
    temp->next = NULL;
    if (i == 0) {
      head_low = temp;
      tail_low = temp;
    } else {
      tail_low->next = temp;
      tail_low = temp;
    }
  }

  /*  add the shell to the scheduler's tasks */
  temp = safe_malloc(1 * sizeof(struct procedure));
  temp->id = i;
  max_id = i + 1;
  temp->priority = 0;     /*initially all procedure are low priority*/
  temp->pid = shell_pid;  // might change it
  temp->next = NULL;
  strcpy(temp->name, SHELL_EXECUTABLE_NAME);

  tail_low->next = temp;
  tail_low = temp;

  for (temp = head_low, i = 0; temp != NULL; temp = temp->next, i++) {
    printf("%s %d %d\n", temp->name, temp->id, temp->pid);
  }

  /* Wait for all children to raise SIGSTOP before exec()ing. */
  wait_for_ready_children(nproc + 1);

  /* Install SIGALRM and SIGCHLD handlers. */
  install_signal_handlers();

  // start the clock and the first procedure
  /* Arrange for an alarm after tq sec */
  if (alarm(SCHED_TQ_SEC) < 0) {
    perror("alarm");
    exit(1);
  }
  wait_pid = head_low->pid;
  kill(head_low->pid, SIGCONT);

  shell_request_loop(request_fd, return_fd);

  /* Now that the shell is gone, just loop forever
   * until we exit from inside a signal handler.
   */
  while (pause())
    ;

  /* Unreachable */
  fprintf(stderr, "Internal error: Reached unreachable point\n");
  return 1;
}
