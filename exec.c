/***************************************************************************
    begin........: April 2015
    copyright....: Sebastian Fedrau
    email........: sebastian.fedrau@gmail.com
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License v3 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License v3 for more details.
 ***************************************************************************/
/**
   @file exec.c
   @brief Executes shell commands.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */

#include <assert.h>

#include "exec.h"
#include "log.h"
#include "utils.h"
#include "format.h"

/*! @cond INTERNAL */
typedef struct
{
	Processor padding;
	const char *path;
	const ExecArgs *args;
} ExecProcessor;
/*! @endcond */

static const char *
_exec_processor_read(Processor *processor)
{
	assert(processor != NULL);

	processor->flags &= ~PROCESSOR_FLAGS_READABLE;

	return ((ExecProcessor *)processor)->path;
}

static void
_exec_processor_write(Processor *processor, const char *dir, const char *path)
{
	ExecProcessor *exec = (ExecProcessor *)processor;

	/* TODO */

	processor->flags |= PROCESSOR_FLAGS_READABLE;
	exec->path = path;
}

static void
_exec_processor_close(Processor *processor)
{
	assert(processor != NULL);

	processor->flags |= PROCESSOR_FLAGS_CLOSED;
}

Processor *
exec_processor_new(const ExecArgs *args)
{
	assert(commands != NULL);

	Processor *processor = (Processor *)utils_malloc(sizeof(ExecProcessor));

	memset(processor, 0, sizeof(ExecProcessor));

	processor->read = _exec_processor_read;
	processor->write = _exec_processor_write;
	processor->close = _exec_processor_close;

	ExecProcessor *exec = (ExecProcessor *)processor;

	exec->args = args;

	return processor;
}

