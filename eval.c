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
   @file eval.c
   @brief Evalutes a post processing expression.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <stdio.h>
#include <assert.h>

#include "eval.h"
#include "log.h"
#include "extension.h"

/*! @cond INTERNAL */
#define FN_STACK_SIZE 64

typedef struct
{
	const char *filename;
	ExtensionManager *extensions;
} EvalContext;
/*! @endcond */

static EvalResult _eval_node(Node *node, EvalContext *ctx);

static bool
_eval_set_fn_arg_from_value_node(ExtensionCallbackArgs *args, uint32_t offset, ValueNode *node)
{
	bool success = true;

	assert(args != NULL);
	assert(node != NULL);

	if(node->vtype == VALUE_NUMERIC)
	{
		extension_callback_args_set_integer(args, offset, node->value.ivalue);
	}
	else if(node->vtype == VALUE_STRING)
	{
		extension_callback_args_set_string(args, offset, node->value.svalue);
	}
	else
	{
		FATALF("eval", "Unexpected argument type: %#x", node->vtype);
		success = false;
	}

	return success;
}

static bool
_eval_func_node(Node *node, EvalContext *ctx, int *fn_result)
{
	int argc = 0;
	FuncNode *fn;
	ExtensionCallbackArgs* args;
	int result;
	bool success = true;

	assert(node != NULL);
	assert(fn_result != NULL);

	fn = (FuncNode *)node;

	TRACEF("eval", "Evaluating function `%s'.", fn->name);

	*fn_result = 0;
	args = extension_callback_args_new(FN_STACK_SIZE);

	/* build argument list */
	if(fn->args)
	{
		/* count & validate arguments */
		Node *iter = fn->args;

		while(iter && success)
		{
			if(iter->type == NODE_EXPRESSION)
			{
				ExpressionNode *expr = (ExpressionNode *)iter;

				if(expr->op != OP_COMMA)
				{
					FATALF("eval", "Couldn't compile argument list, operator %#x not supported.", expr->op);
					success = false;
				}
				else if(expr->first->type == NODE_FUNC)
				{
					success = _eval_func_node(expr->first, ctx, &result);

					if(success)
					{
						extension_callback_args_set_integer(args, argc, result);
					}
				}
				else if(expr->first->type == NODE_VALUE)
				{
					success = _eval_set_fn_arg_from_value_node(args, argc, (ValueNode *)expr->first);
				}
				else
				{
					FATALF("eval", "Unexpected node type: %#x", iter->type);
					success = false;
				}

				++argc;
				iter = expr->second;
			}
			else if(iter->type == NODE_VALUE)
			{
				success = _eval_set_fn_arg_from_value_node(args, argc, (ValueNode *)iter);
				++argc;
				iter = NULL;
			}
			else
			{
				FATALF("eval", "Unexpected argument type: %#x", iter->type);
				success = false;
			}

			if(argc == FN_STACK_SIZE)
			{
				fprintf(stderr, "Stack overflow in function `%s', more than %d arguments are not supported.\n", fn->name, FN_STACK_SIZE);
			}
		}
	}

	/* invoke function */
	if(success)
	{
		ExtensionCallbackStatus status = extension_manager_test_callback(ctx->extensions, fn->name, argc, args->types);

		if(status == EXTENSION_CALLBACK_STATUS_OK)
		{
			success = extension_manager_invoke(ctx->extensions, fn->name, ctx->filename, argc, args->argv, fn_result) == EXTENSION_CALLBACK_STATUS_OK;
		}
		else if(status == EXTENSION_CALLBACK_STATUS_NOT_FOUND)
		{
			fprintf(stderr, "Function `%s' not found.\n", fn->name);
			success = false;
		}
		else
		{
			fprintf(stderr, "Function `%s' has a different signature, please check specified arguments.\n", fn->name);
			success = false;
		}
	}

	if(args)
	{
		extension_callback_args_free(args);
	}

	return success;
}

static EvalResult
_eval_expression_node(Node *node, EvalContext *ctx)
{
	EvalResult result = EVAL_RESULT_ABORTED;
	ExpressionNode *expr = (ExpressionNode *)node;

	assert(expr->first != NULL);
	assert(expr->second != NULL);

	TRACE("eval", "Evaluating expression node.");

	if(expr->op == OP_AND)
	{
		result = _eval_node(expr->first, ctx);

		if(result == EVAL_RESULT_TRUE)
		{
			result = _eval_node(expr->second, ctx);
		}
	}
	else if(expr->op == OP_OR)
	{
		result = _eval_node(expr->first, ctx);

		if(result == EVAL_RESULT_FALSE)
		{
			result = _eval_node(expr->second, ctx);
		}
	}
	else
	{
		FATALF("eval", "Unexpected operator: %#x", expr->op);
	}

	return result;
}

static bool
_eval_node_get_int(Node *node, EvalContext *ctx, int *result)
{
	bool success = false;

	assert(node != NULL);
	assert(result != NULL);

	if(node->type == NODE_FUNC)
	{
		success = _eval_func_node(node, ctx, result);
	}
	else if(node->type == NODE_VALUE)
	{
		ValueNode *val = (ValueNode *)node;

		if(val->vtype == VALUE_NUMERIC)
		{
			*result = val->value.ivalue;
			success = true;
		}
		else
		{
			FATALF("eval", "Data type %#x cannot be casted to integer.", val->vtype);
		}
	}
	else
	{
		FATALF("eval", "Unexpected node type: %#x", node->type);
	}

	return success;
}

static EvalResult
_eval_compare_node(Node *node, EvalContext *ctx)
{
	EvalResult result = EVAL_RESULT_ABORTED;
	CompareNode *cmp = (CompareNode *)node;
	int a, b;

	assert(cmp->first != NULL);
	assert(cmp->second != NULL);

	TRACE("eval", "Evaluating compare node, retrieving integer from first child node.");

	if(_eval_node_get_int(cmp->first, ctx, &a))
	{
		TRACE("eval", "Comparing result with second node.");

		if(cmp->cmp == CMP_EQ && cmp->second->type == NODE_TRUE)
		{
			TRACEF("eval", "%d == true", a);
			result = (a == 0) ? EVAL_RESULT_FALSE : EVAL_RESULT_TRUE;
		}
		else if(_eval_node_get_int(cmp->second, ctx, &b))
		{
			result = EVAL_RESULT_ABORTED;

			switch(cmp->cmp)
			{
				case CMP_EQ:
					TRACEF("eval", "%d == %d", a, b);
					result = (a == b) ? EVAL_RESULT_TRUE : EVAL_RESULT_FALSE;
					break;

				case CMP_LT_EQ:
					TRACEF("eval", "%d <= %d", a, b);
					result = (a <= b) ? EVAL_RESULT_TRUE : EVAL_RESULT_FALSE;
					break;

				case CMP_LT:
					TRACEF("eval", "%d < %d", a, b);
					result = (a < b) ? EVAL_RESULT_TRUE : EVAL_RESULT_FALSE;
					break;

				case CMP_GT_EQ:
					TRACEF("eval", "%d >= %d", a, b);
					result = (a >= b) ? EVAL_RESULT_TRUE : EVAL_RESULT_FALSE;
					break;

				case CMP_GT:
					TRACEF("eval", "%d > %d", a, b);
					result = (a > b) ? EVAL_RESULT_TRUE : EVAL_RESULT_FALSE;
					break;

				default:
					FATALF("eval", "Unexpected compare operator: %#x", cmp->cmp);
			}
		
		}
	}
	else
	{
		FATAL("eval", "Couldn't retrieve integer from first child node.");
	}

	return result;
}

static EvalResult
_eval_node(Node *node, EvalContext *ctx)
{
	EvalResult result = EVAL_RESULT_ABORTED;

	if(node)
	{
		switch(node->type)
		{
			case NODE_EXPRESSION:
				result = _eval_expression_node(node, ctx);
				break;

			case NODE_COMPARE:
				result = _eval_compare_node(node, ctx);
				break;

			default:
				FATALF("eval", "Unexpected node type: %#x", node->type);
		}
	}
	else
	{
		result = EVAL_RESULT_FALSE;
	}

	return result;
}

EvalResult
evaluate(ExtensionManager *manager, Node *node, const char *filename)
{
	EvalContext ctx;

	assert(node != NULL);
	assert(manager != NULL);
	assert(filename != NULL);

	TRACE("eval", "Evaluating syntax tree.");

	memset(&ctx, 0, sizeof(EvalContext));

	ctx.extensions = manager;
	ctx.filename = filename;

	return _eval_node(node, &ctx);
}

