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
   @file processor.c
   @brief Base types and functions for building processor chains.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "processor.h"
#include "utils.h"
#include "log.h"

const char *
processor_read(Processor *processor)
{
	const char *path = NULL;

	assert(processor != NULL);

	if(processor_is_readable(processor) && !processor_is_closed(processor))
	{
		path = processor->read(processor);
	}

	return path;
}

void
processor_write(Processor *processor, const char *dir, const char *path)
{
	assert(processor != NULL);
	assert(path != NULL);

	if(!processor_is_closed(processor))
	{
		processor->write(processor, dir, path);
	}
}

void
processor_free(Processor *processor)
{
	assert(processor != NULL);

	if(processor->free)
	{
		processor->free(processor);
	}
}

void
processor_destroy(Processor *processor)
{
	assert(processor != NULL);

	processor_free(processor);
	free(processor);
}

void
processor_close(Processor *processor, const char *dir)
{
	assert(processor != NULL);

	if(!processor_is_closed(processor))
	{
		if(processor->close)
		{
			processor->close(processor);
		}
		else
		{
			processor->flags |= PROCESSOR_FLAG_CLOSED;
		}
	}
}

ProcessorChain *
processor_chain_prepend(ProcessorChain *chain, Processor *processor)
{
	assert(processor != NULL);

	ProcessorChain *item = utils_new(1, ProcessorChain);

	item->processor = processor;
	item->next = chain;

	return item;
}

static ProcessorChainResult
_processor_state_to_chain_result(const Processor *processor)
{
	ProcessorChainResult result = PROCESSOR_CHAIN_CONTINUE;

	assert(processor != NULL);

	if(processor_is_closed(processor))
	{
		result = PROCESSOR_CHAIN_COMPLETED;
	}

	if(processor_has_error(processor))
	{
		result = PROCESSOR_CHAIN_ERROR;
	}

	return result;
}

ProcessorChainResult
processor_chain_write(ProcessorChain *chain, const char *dir, const char *path)
{
	ProcessorChainResult result = PROCESSOR_CHAIN_CONTINUE;

	assert(dir != NULL);
	assert(path != NULL);

	TRACEF("processor", "Writing to processor chain: dir=%s, path=%s", dir, path);

	if(chain)
	{
		Processor *head = chain->processor;

		result = _processor_state_to_chain_result(head);

		if(result == PROCESSOR_CHAIN_CONTINUE)
		{
			processor_write(head, dir, path);

			while(processor_is_readable(head) && result == PROCESSOR_CHAIN_CONTINUE)
			{
				TRACE("processor", "Reading from processor.");

				result = processor_chain_write(chain->next, dir, processor_read(head));
			}
		}
	}
	else
	{
		TRACE("processor", "Chain is empty.");
	}

	return result;
}

ProcessorChainResult
processor_chain_complete(ProcessorChain *chain, const char *dir)
{
	ProcessorChainResult result = PROCESSOR_CHAIN_COMPLETED;

	assert(dir != NULL);

	if(chain)
	{
		Processor *head = chain->processor;

		result = _processor_state_to_chain_result(head);

		if(result == PROCESSOR_CHAIN_CONTINUE)
		{
			processor_close(head, dir);

			while(processor_is_readable(head) && result != PROCESSOR_CHAIN_COMPLETED)
			{
				result = processor_chain_write(chain->next, dir, processor_read(head));
			}

			if(result == PROCESSOR_CHAIN_CONTINUE)
			{
				result = PROCESSOR_CHAIN_COMPLETED;
			}
		}
	}

	return result;
}

void processor_chain_destroy(ProcessorChain *chain)
{
	if(chain)
	{
		processor_destroy(chain->processor);
		processor_chain_destroy(chain->next);
		free(chain);
	}
}

void
processor_chain_builder_init(ProcessorChainBuilder *builder, const void *user_data)
{
	assert(builder != NULL);

	memset(builder, 0, sizeof(ProcessorChainBuilder));
	builder->user_data = user_data;
}

void
processor_chain_builder_do(ProcessorChainBuilder *builder, ProcessorChainBuilderFn fn, ...)
{
	assert(builder != NULL);

	ProcessorChainBuilderFn f = fn;
	va_list ap;

	va_start(ap, fn);

	while(f && !builder->failed)
	{
		f(builder);
		f = va_arg(ap, ProcessorChainBuilderFn);
	}

	va_end(ap);
}

bool
processor_chain_builder_try_prepend(ProcessorChainBuilder *builder, Processor *processor)
{
	assert(builder != NULL);

	if(processor && !builder->failed)
	{
		builder->chain = processor_chain_prepend(builder->chain, processor);
	}
	else
	{
		processor_chain_builder_fail(builder);
	}

	return !builder->failed;
}

void
processor_chain_builder_fail(ProcessorChainBuilder *builder)
{
	assert(builder != NULL);

	if(!builder->failed)
	{
		if(builder->chain)
		{
			processor_chain_destroy(builder->chain);
			builder->chain = NULL;
		}

		builder->failed = true;
	}
}

ProcessorChain *
processor_chain_builder_get_chain(const ProcessorChainBuilder *builder)
{
	assert(builder != NULL);

	return builder->chain;
}

