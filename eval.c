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
   @version 0.1.0
*/
#include <stdio.h>
#include <assert.h>

#include "eval.h"
#include "extension.h"

/*! @cond INTERNAL */
#define FN_STACK_SIZE 32

typedef struct
{
	const char *filename;
	struct stat *stbuf;
	ExtensionDir *dir;
} EvalContext;

static EvalResult _eval_node(Node *node, EvalContext *ctx);
/*! @endcond */

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
		fprintf(stderr, "Function argument cannot be of type %d.\n", node->vtype);
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
					fprintf(stderr, "%d in an unexpected expression operator when compiling argument list.\n", expr->op);
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
					fprintf(stderr, "Oops, that shouldn't have happened. Type %d is unexpected as first expression child.\n", iter->type);
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
				fprintf(stderr, "Oops, that shouldn't have happened. Type %d is unexpected as function argument.\n", iter->type);
				success = false;
			}

			if(argc == FN_STACK_SIZE)
			{
				fprintf(stderr, "Stack overflow in function \"%s\".\n", fn->name);
			}
		}
	}

	/* invoke function */
	if(success)
	{
		ExtensionCallbackStatus status = extension_dir_test_callback(ctx->dir, fn->name, argc, args->types);

		if(status == EXTENSION_CALLBACK_STATUS_OK)
		{
			success = extension_dir_invoke(ctx->dir, fn->name, ctx->filename, ctx->stbuf, argc, args->argv, fn_result) == EXTENSION_CALLBACK_STATUS_OK;
		}
		else if(status == EXTENSION_CALLBACK_STATUS_NOT_FOUND)
		{
			fprintf(stderr, "Function \"%s\" not found.\n", fn->name);
			success = false;
		}
		else
		{
			fprintf(stderr, "No function named \"%s\" has the specified signature.\n", fn->name);
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
		fprintf(stderr, "Oops, that shouldn't have happened. Operator %d is not supported in %s.\n", expr->op, __func__);
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
			fprintf(stderr, "Oops, that shouldn't have happened. Data type: %d cannot be casted to integer.\n", val->vtype);
		}
	}
	else
	{
		fprintf(stderr, "Oops, that shouldn't have happened. Node type %d is not supported in %s.\n", node->type, __func__);
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

	if(_eval_node_get_int(cmp->first, ctx, &a))
	{
		if(cmp->cmp == CMP_EQ && cmp->second->type == NODE_TRUE)
		{
			result = (a == 0) ? EVAL_RESULT_FALSE : EVAL_RESULT_TRUE;
		}
		else if(_eval_node_get_int(cmp->second, ctx, &b))
		{
			result = EVAL_RESULT_FALSE;

			switch(cmp->cmp)
			{
				case CMP_EQ:
					result = (a == b) ? EVAL_RESULT_TRUE : EVAL_RESULT_FALSE;
					break;

				case CMP_LT_EQ:
					result = (a <= b) ? EVAL_RESULT_TRUE : EVAL_RESULT_FALSE;
					break;

				case CMP_LT:
					result = (a < b) ? EVAL_RESULT_TRUE : EVAL_RESULT_FALSE;
					break;

				case CMP_GT_EQ:
					result = (a >= b) ? EVAL_RESULT_TRUE : EVAL_RESULT_FALSE;
					break;

				case CMP_GT:
					result = (a > b) ? EVAL_RESULT_TRUE : EVAL_RESULT_FALSE;
					break;

				default:
					fprintf(stderr, "Oops, that shouldn't have happened. Unsupported compare operator %d in %s.\n", cmp->cmp, __func__);
					result = EVAL_RESULT_ABORTED;
			}
		
		}
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
				fprintf(stderr, "Oops, that shouldn't have happened. Node type %d is unexpected in %s.\n", node->type, __func__);
		}
	}
	else
	{
		result = EVAL_RESULT_FALSE;
	}

	return result;
}

EvalResult
evaluate(Node *node, ExtensionDir *dir, const char *filename, struct stat *stbuf)
{
	EvalContext ctx;

	assert(node != NULL);
	assert(dir != NULL);
	assert(filename != NULL);
	assert(stbuf != NULL);

	memset(&ctx, 0, sizeof(EvalContext));

	ctx.dir = dir;
	ctx.filename = filename;
	ctx.stbuf = stbuf;

	return _eval_node(node, &ctx);
}

