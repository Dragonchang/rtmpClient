#ifndef __BASE_HEADER_H_
#define __BASE_HEADER_H_
#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/time.h>

#define __DEBUG
#ifdef __DEBUG
#define Debug cout
#else
#define Debug 0 && cout
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif


#endif