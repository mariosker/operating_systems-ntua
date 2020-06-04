#include "queue-priority.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned queue_max;

void* safe_malloc(size_t size) {
  void* p;

  if ((p = malloc(size)) == NULL) {
    fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n", size);
    exit(1);
  }
  return p;
}

void enqueue(list* queue, pid_t pid, char* name, int has_id) {
  process* new_node = safe_malloc(sizeof(process));
  queue->queue_length++;
  new_node->pid = pid;
  if (has_id == -1) {
    queue_max++;
    new_node->id = queue_max;
  } else
    new_node->id = has_id;
  new_node->name = name;

  if (queue->queue_length == 1) {
    queue->head = new_node;
    queue->tail = queue->head;
    queue->tail->next = queue->head;
  } else {
    queue->tail->next = new_node;
    queue->tail = new_node;
    queue->tail->next = queue->head;
  }
}

void dequeue(list* queue, pid_t pid) {
  if (queue->queue_length == 1) {
    process* temp = queue->head;
    queue->head = NULL;
    queue->tail = NULL;
    free(temp);
    queue->queue_length--;
    return;
  }

  process* temp = queue->head;
  while (temp->next->pid != pid) temp = temp->next;

  process* to_delete = temp->next;
  free(to_delete);
  temp->next = temp->next->next;
  queue->queue_length--;
  // if (queue->queue_length == 0) {
  //   printf("Done!\n");
  //   exit(10);
  // }
}

process* search_by_id(list* queue, int id) {
  if (queue->queue_length == 0) return NULL;
  process* temp = queue->head;
  for (int i = 0; i < queue->queue_length; i++) {
    if (temp == NULL) return NULL;
    if (temp->id == id) {
      // printf("%")
      return temp;
    }
    temp = temp->next;
  }
  return NULL;
}

process* search_by_name(list* queue, char* name) {
  if (queue->queue_length == 0) return NULL;
  process* temp = queue->head;
  for (int i = 0; i < queue->queue_length; i++) {
    if (temp == NULL) return NULL;
    if (strcmp(temp->name, name) == 0) {
      // printf("%")
      return temp;
    }
    temp = temp->next;
  }
  return NULL;
}

void rotate_queue(list* queue) {
  if (queue->queue_length <= 1) return;
  queue->head = queue->head->next;
  queue->tail = queue->tail->next;
}

void print_queue(list* queue) {
  if (queue->queue_length == 0) return;
  printf("QUEUE SIZE = %d\n", queue->queue_length);
  process* temp = queue->head;
  for (int i = 0; i < queue->queue_length; i++) {
    printf("-->ID: %d, PID: %d, NAME: %s\n", temp->id, temp->pid, temp->name);
    temp = temp->next;
  }
}
