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
   @file eval.h
   @brief Evalutes a post processing expression.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
   @version 0.1.0
*/

#ifndef __EVAL_H__
#define __EVAL_H__

#include "ast.h"
#include "extension.h"

/**
   @enum EvalResult
   @brief Evaluation result.
  */
typedef enum
{
	/*!* Expression evaluates to true. */
	EVAL_RESULT_TRUE,
	/*!* Expression evaluates to false. */
	EVAL_RESULT_FALSE,
	/*!* Expression evaluation failed .*/
	EVAL_RESULT_ABORTED
} EvalResult;

/**
   @param node node to evaluate
   @param extensions an ExtensionManager
   @param filename name of the found file
   @param stbuf status of the found file
   @return a boolean

   Evaluates an abstract syntax tree.
  */
EvalResult evaluate(Node *node, ExtensionManager *manager, const char *filename);
#endif

