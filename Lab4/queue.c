#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

process* head;
process* tail;
unsigned queue_length = 0;

void* safe_malloc(size_t size) {
  void* p;

  if ((p = malloc(size)) == NULL) {
    fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n", size);
    exit(1);
  }

  return p;
}

void enqueue(pid_t pid, char* name) {
  process* new_node;
  new_node = safe_malloc(sizeof(new_node));
  new_node->id = (queue_length++);
  new_node->pid = pid;
  new_node->name = name;
  if (head == NULL) {
    head = new_node;
  } else {
    tail->next = new_node;
  }
  tail = new_node;
  tail->next = head;
}

void dequeue(pid_t pid) {
  process* temp = head;
  unsigned count = 0;
  while (1) {
    if (count > queue_length) {
      fprintf(stderr, "PID not found in queue");
      exit(1);
    }
    if (temp->next->pid == pid) break;
    temp = temp->next;
    count++;
  }

  process* to_delete = temp->next;
  temp->next = temp->next->next;
  free(to_delete);
  queue_length--;
}
