#ifndef QUEUE_H
#define QUEUE_H

#include <unistd.h>

typedef struct process_s {
  unsigned id;
  pid_t pid;
  char *name;
  struct process_s *next;
} process;

typedef struct list_s {
  process *head;
  process *tail;
  unsigned queue_length;
} list;

void *safe_malloc(size_t size);
void enqueue(list *queue, pid_t pid, char *name, int has_id);
void dequeue(list *queue, pid_t pid);
void rotate_queue(list *queue);
process *search_by_id(list *queue, int id);
process *search_by_name(list *queue, char *name);
void print_queue(list *queue);
#endif
