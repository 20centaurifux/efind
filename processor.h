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
   @file processor.h
   @brief Base types for building processor chains.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdbool.h>

/**
   @enum ProcessorFlags
   @brief Available processor state flags.
 */
typedef enum
{
	/*! Indicates that data can be read from the processor. */
	PROCESSOR_FLAGS_READABLE = 1,
	/*! Set if the processor is closed. */
	PROCESSOR_FLAGS_CLOSED = 2
} ProcessorFlags;

/**
   @struct  Processor
   @brief Base type for processing data. Processors have a source
          and a sink. Multiple processors can be chained together.
 */
typedef struct _Processor
{
	/*! Processor state flags. */
	int flags;

	/**
	   @param processor processor to read data from
	   @return processed data
	   
	   Reads (and pops) data from the processor's source.
	 */
	const char *(*read)(struct _Processor *processor);

	/**
	   @param processor processor to write to
	   @param dir search directory
	   @param path found path
	   
	   Writes a found path to the processor's sink.
	 */
	void (*write)(struct _Processor *processor, const char *dir, const char *path);

	/**
	   @param processor processor to close
	   
	   Closes the processor.
	 */
	void (*close)(struct _Processor *processor);

	/**
	   @param processor processor to free
	   
	   Frees resources allocated by a processor.
	 */
	void (*free)(struct _Processor *processor);
} Processor;

/**
   @struct ProcessorChain
   @brief Multiple processors can be chained together.
 */
typedef struct _ProcessorChain
{
	/*! A processor. */
	Processor *processor;

	/*! Pointer to next processor. */
	struct _ProcessorChain *next;
} ProcessorChain;

/**
   @struct ProcessorChainBuilder
   @brief Utility for building processor chains.
 */
typedef struct
{
	/*! Set if a chain couldn't be prepended. */
	bool failed;
	/*! Chain to build. */
	ProcessorChain *chain;
	/*! User data. */
	const void *user_data;
} ProcessorChainBuilder;

/**
   @param builder a ProcessorChainBuilder

   A function applied to a ProcessorChainBuilder.
 */
typedef void (*ProcessorChainBuilderFn)(ProcessorChainBuilder *builder);

/*! Sentinel value to terminate a variadic argument list of ProcessorChainBuilderFn functions. */
#define PROCESSOR_CHAIN_BUILDER_NULL_FN (ProcessorChainBuilderFn *)NULL

/*! Checks if data can be read from a processor. */
#define processor_is_readable(p) (p->flags & PROCESSOR_FLAGS_READABLE)

/*! Checks if a processor has been closed. */
#define processor_is_closed(p) (p->flags & PROCESSOR_FLAGS_CLOSED)

/**
   @param processor processor to read from
   @return processed data

   Reads (and pops) data from the processor's source.
 */
const char *processor_read(Processor *processor);

/**
   @param processor processor to write to
   @param dir search directory
   @param path found path

   Writes a found path to the processor's sink.
 */
void processor_write(Processor *processor, const char *dir, const char *path);

/**
   @param processor processor to free

   Frees resources allocated by a processor.
*/
void processor_free(Processor *processor);

/**
   @param processor processor to destroy

   Frees resources allocated by and for a processor.
*/
void processor_destroy(Processor *processor);

/**
   @param processor processor to close
   @param dir search directory
   
   Closes a processor.
 */
void processor_close(Processor *processor, const char *dir);

/**
   @param chain a processor chain (or NULL if empty)
   @param processor processor to prepend
   @return the new processor chain
   
   Prepends a processor to a chain.
 */
ProcessorChain *processor_chain_prepend(ProcessorChain *chain, Processor *processor);

/**
   @param chain chain which should process a found path
   @param dir search directory
   @param path a found path
   @return false if the chain has been closed, no data will be processed anymore
   
   Processes a found path.
 */
bool processor_chain_write(ProcessorChain *chain, const char *dir, const char *path);

/**
   @param chain a processor chain
   @param dir search directory
   
   Closes the first processor of the chain. This will stop any further processing.
 */
void processor_chain_complete(ProcessorChain *chain, const char *dir);

/**
   @param chain chain to destroy
   
   Frees resources allocated by the given processor chain.
 */
void processor_chain_destroy(ProcessorChain *chain);

/**
   @param builder ProcessorChainBuilder to initialize
   @param user_data custom data assigned to the builder
   
   Initializes a ProcessorChainBuilder.
 */
void processor_chain_builder_init(ProcessorChainBuilder *builder, const void *user_data);

/**
   @param builder ProcessorChainBuilder to apply functions to
   @param fn first function to apply
   
   Applies functions to a ProcessorChainBuilder instance. Stops when the state
   changes to failed.
 */
void processor_chain_builder_do(ProcessorChainBuilder *builder, ProcessorChainBuilderFn fn, ...);

/**
   @param builder a ProcessorChainBuilder
   @param processor processor to prepend to the wrapped ProcessorChain
   @return true on success

   Prepends a processor to the chain wrapped by the ProcessorChainBuilder. If processor
   is NULL the builder's state changes to failed and the chain is destroyed.
 */
bool processor_chain_builder_try_prepend(ProcessorChainBuilder *builder, Processor *processor);

/**
   @param builder a ProcessorChainBuilder

   Sets the builder's state to failed and destroys the already built chain.
 */
void processor_chain_builder_fail(ProcessorChainBuilder *builder);

/**
   @param builder a ProcessorChainBuilder
   @return the build ProcessorChain or NULL

   Gets the build chain. Returns NULL if an error occurred during build.
 */
ProcessorChain *processor_chain_builder_get_chain(ProcessorChainBuilder *builder);
#endif

