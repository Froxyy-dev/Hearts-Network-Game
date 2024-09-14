#ifndef KIERKI_ERR_H
#define KIERKI_ERR_H

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

// Print information about a system error and quits.
[[noreturn]] void sysFatal(const char* fmt, ...);

// Print information about a system error and return.
void sysError(const char* fmt, ...);

// Print information about an error and quits.
[[noreturn]] void fatal(const char* fmt, ...);

// Print information about an error and return.
void error(const char* fmt, ...);

#endif // KIERKI_ERR_H
