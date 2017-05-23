#include <extension-interface.h>
#include <string.h>
#include <ctype.h>
	
void
registration(RegistrationCtx *ctx, RegisterExtension fn)
{
	fn(ctx, "example extension", "0.1.0", "An example extension written in C.");
}
	
void
discover(RegistrationCtx *ctx, RegisterCallback fn)
{
	fn(ctx, "c_check_extension", 2, CALLBACK_ARG_TYPE_STRING, CALLBACK_ARG_TYPE_INTEGER);
}

int
c_check_extension(const char *filename, int argc, void *argv[])
{
	const char *offset;
	int result = 0;

	/* find extension */
	if((offset = strrchr(filename, '.')))
	{
		/* first argument: extension to test */
		char *extension = argv[0];

		/* second argument: compare mode */
		int icase = *((int *)argv[1]);

		if(extension && strlen(offset) == strlen(extension))
		{
			result = 1;

			while(*offset && result)
			{
				if(icase)
				{
					result = tolower(*offset) == tolower(*extension);
				}
				else
				{
					result = *offset == *extension;
				}

				++offset, ++extension;
			}
		}
	}

	return result;
}
