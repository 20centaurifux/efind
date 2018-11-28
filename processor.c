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
   @brief Base types for building processor chains.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */

#include <assert.h>
#include "utils.h"

#include "processor.h"

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
processor_write(Processor *processor, const char *path)
{
	assert(processor != NULL);
	assert(path != NULL);

	if(!processor_is_closed(processor))
	{
		processor->write(processor, path);
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
processor_close(Processor *processor)
{
	assert(processor != NULL);

	if(processor_is_closed(processor))
	{
		processor->close(processor);
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

bool
processor_chain_write(ProcessorChain *chain, const char *path)
{
	bool completed = false;

	assert(path != NULL);

	if(chain)
	{
		Processor *head = chain->processor;

		completed = processor_is_closed(head);

		if(!completed)
		{
			processor_write(head, path);

			while (processor_is_readable(head))
			{
				completed = processor_chain_write(chain->next, processor_read(head));
			}

			completed |= processor_is_closed(head);
		}
	}

	return completed;
}

void
processor_chain_complete(ProcessorChain *chain)
{
	bool completed = false;

	if(chain)
	{
		Processor *head = chain->processor;

		completed = processor_is_closed(head);

		if(!completed)
		{
			while (processor_is_readable(head))
			{
				processor_chain_write(chain->next, processor_read(head));
			}

			processor_close(head);
		}
	}
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

