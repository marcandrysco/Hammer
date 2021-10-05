#include "inc.h"


/**
 * Main entry point.
 *   @argc: The number of arguments.
 *   @argv: The argument array.
 *   &returns: Exit status.
 */
int main(int argc, char **argv)
{
	cli_app = "hammer";
	cli_proc(argv + 1);
	
	return 0;
}


/**
 * Create a 64-bit hash of a string.
 *   @hash: The starting hash.
 *   @str: The string.
 *   &returns: The hash.
 */
uint64_t hash64(uint64_t hash, const char *str)
{
	while(*str != '\0') {
		hash += (uint64_t)*str++ ^ 0x687ebde0fd11d4ae;
		hash *= 0xe278b9fdc665d444;
		hash ^= (hash >> 53);
		hash *= 0x19ba967de10aacc0;
		hash ^= (hash >> 48);
		hash *= 0x1b2b216668c9e998;
		hash ^= (hash >> 43);
		hash *= 0x73c3d43eb467f8df;
		hash ^= (hash >> 37);
	}

	return hash;
}


/**
 * Create a formatted string.
 *   @pat: The printf-style pattern.
 */
char *str_fmt(const char *pat, ...)
{
	char *ret;
	va_list args;

	va_start(args, pat);
	ret = malloc(vsnprintf(NULL, 0, pat, args) + 1);
	va_end(args);

	va_start(args, pat);
	vsprintf(ret, pat, args);
	va_end(args);

	return ret;
}

/**
 * Set a string.
 *   @dst: Ref. The destination, freed is non-null.
 *   @src: The string to use.
 */
void str_set(char **dst, char *src)
{
	if(*dst != NULL)
		free(*dst);

	*dst = src;
}
