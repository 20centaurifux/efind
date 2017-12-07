#include <extension-interface.h>
#include <string.h>
#include <ctype.h>
	
void
registration(RegistrationCtx *ctx, RegisterExtension fn)
{
	fn(ctx, "c-test", "0.1.0", "efind test extension.");
}
	
void
discover(RegistrationCtx *ctx, RegisterCallback fn)
{
	fn(ctx, "c_name_equals", 1, CALLBACK_ARG_TYPE_STRING);
	fn(ctx, "c_add", 2, CALLBACK_ARG_TYPE_INTEGER, CALLBACK_ARG_TYPE_INTEGER);
	fn(ctx, "c_sub", 2, CALLBACK_ARG_TYPE_INTEGER, CALLBACK_ARG_TYPE_INTEGER);
}

int
c_name_equals(const char *filename, int argc, void *argv[])
{
	return strcmp(filename, *argv) == 0;
}

int
c_add(const char *filename, int argc, void *argv[])
{
	int a = *((int *)argv[0]);
	int b = *((int *)argv[1]);

	return a + b;
}

int
c_sub(const char *filename, int argc, void *argv[])
{
	int a = *((int *)argv[0]);
	int b = *((int *)argv[1]);

	return a - b;
}

