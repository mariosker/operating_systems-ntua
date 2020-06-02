#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

process* head;
process* tail;
unsigned queue_length;

void* safe_malloc(size_t size) {
  void* p;

  if ((p = malloc(size)) == NULL) {
    fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n", size);
    exit(1);
  }

  return p;
}

void enqueue(pid_t pid, char* name) {
  process* new_node = safe_malloc(sizeof(new_node));
  new_node->pid = pid;
  new_node->id = queue_length;
  new_node->name = name;
  process* temp = head;
  while (temp->next != NULL) temp = temp->next;

  temp->next = new_node;
  tail = new_node;
  queue_length++;
}

void dequeue(pid_t pid) {
  process* temp = head;
  while (temp->next->pid != pid) temp = temp->next;

  process* to_delete = temp->next;
  free(to_delete);
  temp->next = temp->next->next;
  queue_length--;
  if (queue_length == 0) {
    printf("Done!\n");
    exit(10);
  }
}

void rotate_queue() {
  head = head->next;
  tail = tail->next;
}
