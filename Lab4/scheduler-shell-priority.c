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
#include "queue.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

process *head, *tail;
unsigned queue_length, queue_max, h_count;

/* Print a list of all tasks currently being scheduled.  */
static void sched_print_tasks(void) {
  process *temp = head;
  printf("Current Process: ");
  for (int i = 0; i < queue_length; i++) {
    if (temp != head) printf("                 ");
    printf("ID: %d, PID: %d, NAME: %s\n", temp->id, temp->pid, temp->name);
    temp = temp->next;
  }
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int sched_kill_task_by_id(int id) {
  process *temp = head;

  while (temp->id != id) {
    temp = temp->next;
    if (temp == head) {
      printf("ID not in process queue\n");
      return -1;
    }
  }
  printf("ID: %d, PID: %d, NAME: %s is being killed\n", temp->id, temp->pid,
         temp->name);

  if (kill(temp->pid, SIGKILL) < 0) {
    perror("kill");
  }

  head = temp;
  return 1;
}

void child(char *name) {
  char *newargv[] = {name, NULL, NULL, NULL};
  char *newenviron[] = {NULL};

  printf("I am %s, PID = %ld\n", name, (long)getpid());
  printf("About to replace myself with the executable %s...\n", name);
  sleep(2);
  raise(SIGSTOP);
  execve(name, newargv, newenviron);

  /* execve() only returns on error */
  perror("execve");
  exit(1);
}

/* Create a new task.  */
static void sched_create_task(char *executable) {
  pid_t pid;

  pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);
  } else if (pid == 0) {
    fprintf(stderr, "A new proccess is created with pid=%ld \n",
            (long int)getpid());

    child(executable);
    assert(0);
  } else {
    show_pstree(getpid());
    tail->next = NULL;  // make queue linear for enqueue
    enqueue(pid, executable);
    tail->next = head;  // make queue circular again
    printf(
        "Parent: Created child with PID = %ld, waiting for it to "
        "terminate...\n",
        (long)pid);
  }
}

// static int sched_set_high_p(int id) {
//   process *temp = head;
//   process *prev = NULL;
//   tail->next = NULL;  // make queue linear for enqueue

//   if (temp->id == id) {
//     h_count++;
//     temp->h_priority = 1;
//     tail->next = head;
//     return 0;
//   }
//   prev = temp;
//   temp = temp->next;
//   while (temp != NULL) {
//     if (temp->id == id) {
//       h_count++;
//       temp->h_priority = 1;

//       prev->next = temp->next;
//       temp->next = head->next;
//       head = temp;
//       tail->next = head;  // make queue circular again
//       printf("ID: %d, PID: %d, NAME: %s has HIGH priority now\n", temp->id,
//              temp->pid, temp->name);
//       return 0;
//     }
//     prev = temp;
//     temp = temp->next;
//   }

static int sched_set_high_p(int id) {
  process *temp = head;
  process *prev = head;
  tail->next = NULL;  // make queue linear for enqueue

  if (temp->id == id) {
    h_count++;
    temp->h_priority = 1;
    tail->next = head;
    printf("ID: %d, PID: %d, NAME: %s has HIGH priority now\n", temp->id,
           temp->pid, temp->name);
    return 0;
  }

  prev = temp;
  temp = temp->next;

  while (temp != NULL) {
    if (temp->id == id) {
      temp->h_priority = 1;
      prev->next = temp->next;
      temp->next = head;
      head = temp;
      tail->next = head;
      h_count++;
      printf("ID: %d, PID: %d, NAME: %s has HIGH priority now\n", temp->id,
             temp->pid, temp->name);

      return 0;
    }
    prev = temp;
    temp = temp->next;
  }
  tail->next = head;
  return -1;
}

static int sched_set_low_p(int id) {
  process *temp = head;
  process *prev = head;
  tail->next = NULL;  // make queue linear for enqueue

  if (temp->id == id) {
    h_count--;
    temp->h_priority = 0;
    head = temp->next;
    tail->next = temp;
    tail = temp;
    tail->next = head;
    printf("ID: %d, PID: %d, NAME: %s has LOW priority now\n", temp->id,
           temp->pid, temp->name);
    return 0;
  }

  prev = temp;
  temp = temp->next;

  while (temp != NULL) {
    if (temp->id == id) {
      temp->h_priority = 0;
      prev->next = temp->next;
      tail->next = temp;
      tail = temp;
      tail->next = head;
      h_count--;
      printf("ID: %d, PID: %d, NAME: %s has LOW priority now\n", temp->id,
             temp->pid, temp->name);

      return 0;
    }
    prev = temp;
    temp = temp->next;
  }
  tail->next = head;
  return -1;
}

// static int sched_set_low_p(int id) {
//   process *temp = head;
//   process *prev = NULL;
//   tail->next = NULL;  // make queue linear for enqueue

//   if (temp->id == id) {
//     h_count--;
//     temp->h_priority = 0;
//     head = temp->next;
//     tail->next = temp;
//     tail = temp;
//     tail->next = head;
//     printf("ID: %d, PID: %d, NAME: %s has LOW priority now\n", temp->id,
//            temp->pid, temp->name);
//     return 0;
//   }
//   prev = temp;
//   temp = temp->next;
//   while (temp != NULL) {
//     if (temp->id == id) {
//       h_count--;
//       temp->h_priority = 0;

//       prev->next = temp->next;
//       tail->next = temp;
//       tail = temp;
//       tail->next = head;  // make queue circular again
//       return 0;
//     }
//     prev = temp;
//     temp = temp->next;
//   }
//   tail->next = head;  // make queue circular again
//   return -1;
// }

/* Process requests by the shell.  */
static int process_request(struct request_struct *rq) {
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
      sched_set_high_p(rq->task_arg);
      return 0;

    case REQ_LOW_TASK:
      sched_set_low_p(rq->task_arg);
      return 0;

    default:
      return -ENOSYS;
  }
}

/*
 * SIGALRM handler
 */
static void sigalrm_handler(int signum) {
  if (signum != SIGALRM) {
    fprintf(stderr, "Internal error: Called for signum %d, not SIGALRM\n",
            signum);
    exit(1);
  }

  // kill the proccess
  if (kill(head->pid, SIGSTOP) < 0) {
    perror("kill");
    exit(1);
  }
}

// kill the proccess
static void sigchld_handler(int signum) {
  pid_t p;
  int status;

  if (signum != SIGCHLD) {
    fprintf(stderr, "Internal error: Called for signum %d, not SIGCHLD\n",
            signum);
    exit(1);
  }

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
      printf("Parent: Received SIGCHLD, child is dead.\n");

      dequeue(head->pid);

      rotate_queue();
      if (head->h_priority == 0 && h_count > 0)
        while (head->h_priority == 0) rotate_queue();

      fprintf(stderr, "Proccess with pid=%ld is about to begin...\n",
              (long int)head->pid);

      if (kill(head->pid, SIGCONT) < 0) {
        perror("Continue to process");
        exit(1);
      }
      /* Setup the alarm again */
      if (alarm(SCHED_TQ_SEC) < 0) {
        perror("alarm");
        exit(1);
      }
    }
    if (WIFSTOPPED(status)) {
      /* A child has stopped due to SIGSTOP/SIGTSTP, etc... */

      // rotate queue
      rotate_queue();
      if (head->h_priority == 0 && h_count > 0)
        while (head->h_priority == 0) rotate_queue();

      fprintf(stderr, "Proccess with pid=%ld is about to begin...\n",
              (long int)head->pid);
      if (kill(head->pid, SIGCONT) < 0) {
        perror("Continue to process");
        exit(1);
      }

      /* Setup the alarm again */
      if (alarm(SCHED_TQ_SEC) < 0) {
        perror("alarm");
        exit(1);
      }
    }
  }
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void signals_disable(void) {
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
static void signals_enable(void) {
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
static void install_signal_handlers(void) {
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

static void do_shell(char *executable, int wfd, int rfd) {
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
static void sched_create_shell(char *executable, int *request_fd,
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
  // initialize queue with the shell process
  head = safe_malloc(sizeof(process));
  head->id = 0;
  head->h_priority = 0;
  head->pid = p;
  head->name = SHELL_EXECUTABLE_NAME;
  tail = head;
  tail->next = NULL;
  /* Parent */
  close(pfds_rq[1]);
  close(pfds_ret[0]);
  *request_fd = pfds_rq[0];
  *return_fd = pfds_ret[1];
}

static void shell_request_loop(int request_fd, int return_fd) {
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
  int nproc;
  pid_t pid;
  /* Two file descriptors for communication with the shell */
  static int request_fd, return_fd;

  /* Create the shell. */
  sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);

  /*
   * For each of argv[1] to argv[argc - 1],
   * create a new child process, add it to the process list.
   */
  queue_length = 0;
  queue_max = 0;
  nproc = argc; /* number of proccesses goes here */

  for (int i = 1; i < nproc; i++) {
    printf("Parent: Creating child...\n");
    pid = fork();

    if (pid < 0) {
      perror("fork");
      exit(1);
    } else if (pid == 0) {
      fprintf(stderr, "A new proccess is created with pid=%ld \n",
              (long int)getpid());

      child(argv[i]);
      assert(0);
    } else {
      enqueue(pid, argv[i]);
      printf(
          "Parent: Created child with PID = %ld, waiting for it to "
          "terminate...\n",
          (long)pid);
    }
  }
  // make the queue circular

  free(tail->next);
  tail->next = head;

  /* Wait for all children to raise SIGSTOP before exec()ing. */
  wait_for_ready_children(nproc);
  show_pstree(getpid());

  /* Install SIGALRM and SIGCHLD handlers. */
  install_signal_handlers();

  if (nproc == 0) {
    fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
    exit(1);
  }

  if (kill(head->pid, SIGCONT) < 0) {
    perror("First child error with continuing");
    exit(1);
  }

  if (alarm(SCHED_TQ_SEC) < 0) {
    perror("alarm");
    exit(1);
  }

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
