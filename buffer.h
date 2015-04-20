#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifndef BUFFER_H
#define BUFFER_H

#define MAX_BUFFER_SIZE 4096

typedef struct Buffer
{
	char *data;
	size_t len;
	size_t msize;
	bool valid;
} Buffer;

void buffer_init(Buffer *buf);
void buffer_free(Buffer *buf);
void buffer_clear(Buffer *buf);
size_t buffer_len(const Buffer *buf);
bool buffer_is_valid(const Buffer *buf);
bool buffer_is_empty(const Buffer *buf);
bool buffer_fill(Buffer *buf, const char *data, size_t len);
ssize_t buffer_fill_from_fd(Buffer *buf, int fd, size_t count);
bool buffer_read_line(Buffer *buf, char **dst, size_t *len);
bool buffer_flush(Buffer *buf, char **dst, size_t *len);
char *buffer_to_string(Buffer *buf);

#endif
