#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "civetweb.h"
#include "main.h"

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define PORT "8888"

static void initializeArena(MemoryArena *arena, size_t size, void *base) {
  arena->size = size;
  arena->base = (uint8_t *)base;
  arena->used = 0;
}

inline static size_t getAlignmentOffset(MemoryArena *arena, size_t alignment) {
  size_t resultPointer = (size_t)arena->base + arena->used;
  size_t alignmentOffset = 0;

  size_t alignmentMask = alignment - 1;
  if (resultPointer & alignmentMask) {
    alignmentOffset = alignment - (resultPointer & alignmentMask);
  }

  return alignmentOffset;
}

#define DEFAULT_ALIGNMENT 4
void *pushSize(MemoryArena *arena, size_t size, size_t alignment) {
  size_t originalSize = size;
  size_t alignmentOffset = getAlignmentOffset(arena, alignment);
  size += alignmentOffset;

  assert((arena->used + size) <= arena->size);
  void *result = arena->base + arena->used + alignmentOffset;
  arena->used += size;

  assert(size >= originalSize);

  return result;
}

TemporaryMemory h_begin_temporary_memory(struct mg_connection *conn) {
  // TODO(hugo): My understanding is a thread handles one request at the time
  // if this is not true we can split the thread arena in max requests
  // and follow this exact same procedure
  MemoryArena *arena = mg_get_thread_pointer(conn);
  TemporaryMemory result;

  result.arena = arena;
  result.used = arena->used;

  return result;
}

void h_end_temporary_memory(TemporaryMemory tmp_mem) {
  MemoryArena *arena = tmp_mem.arena;
  assert(arena->used >= tmp_mem.used);
  arena->used = tmp_mem.used;
}

int AHandler(struct mg_connection *conn, void *cbdata) {
  (void)cbdata; /* unused */
  TemporaryMemory tmp_mem = h_begin_temporary_memory(conn);
  pushSize(tmp_mem.arena, 100, DEFAULT_ALIGNMENT);
  printf("%zu %zu\n", tmp_mem.arena->size, tmp_mem.arena->used);

  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
                  "close\r\n\r\n");
  mg_printf(conn, "<html><body>");
  mg_printf(conn, "<h2>This is the A handler.</h2>");
  mg_printf(conn, "</body></html>\n");
  h_end_temporary_memory(tmp_mem);
  return 1;
}

void *init_thread(const struct mg_context *ctx, int thread_type) {
  (void)ctx; /* unused */
  printf("initializing thread %d\n", thread_type);
  if (thread_type == 1) {
    // 1 indicates a worker thread handling client connections
    size_t size = Megabytes(100);
    void *base = malloc(size);
    assert(base != 0);
    MemoryArena *arena = base;
    initializeArena(arena, size - sizeof(MemoryArena),
                    ((uint8_t *)base) + sizeof(MemoryArena));
    return base;
  }
  return NULL;
}

void exit_thread(const struct mg_context *ctx, int thread_type,
                 void *thread_pointer) {
  (void)ctx; /* unused */
  printf("exiting thread %d\n", thread_type);
  if (thread_type == 1) {
    // 1 indicates a worker thread handling client connections
    free(thread_pointer);
  }
}

int log_message(const struct mg_connection *conn, const char *message) {
  (void)conn; /* unused */
  puts(message);
  return 1;
}

int main(void) {
  struct mg_callbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.log_message = log_message;
  callbacks.init_thread = init_thread;
  callbacks.exit_thread = exit_thread;

  struct mg_context *ctx;
  const char *options[] = {"listening_ports",
                           PORT,
                           "request_timeout_ms",
                           "10000",
                           "error_log_file",
                           "error.log",
                           "enable_auth_domain_check",
                           "no",
                           0};
  ctx = mg_start(&callbacks, 0, options);
  mg_set_request_handler(ctx, "/A", AHandler, 0);
  if (ctx == NULL) {
    fprintf(stderr, "Cannot start CivetWeb - mg_start failed.\n");
    return 1;
  }

  printf("listening\n");

  while (1) {
    sleep(10);
  }
  mg_stop(ctx);
  printf("Server stopped.\n");
  printf("Bye!\n");

  return 0;
}
