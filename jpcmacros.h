#ifndef JPCMACROS_H
#define JPCMACROS_H

/*****************************************************************************
 * Macros for parsing command line arguments
 ****************************************************************************/
#define IF_OPT(opt)    if (!strcmp((opt), argv[i]))
#define NEXT_ARG_STR   ((++i < argc) ? argv[i]       : (help(), (char*)""))
#define NEXT_ARG_INT   ((++i < argc) ? atoi(argv[i]) : (help(), 0))
#define NEXT_ARG_FLOAT ((++i < argc) ? atof(argv[i]) : (help(), 0.0F))

/*****************************************************************************
 * Macro for switch verbosity levels.
 ****************************************************************************/
#define VL(_level_) if (verbosity >= _level_)

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

#define ERRORIF(_cond_, ...) do { if (_cond_)    { eprintf("ERROR:   "); eprintf(__VA_ARGS__); eprintf(" [%s:%i]\n", __FILE__,__LINE__); exit(1); } } while (0)
#define WARNIF(_cond_, ...)  do { if (_cond_)    { eprintf("WARNING: "); eprintf(__VA_ARGS__); eprintf(" [%s:%i]\n", __FILE__,__LINE__);          } } while (0)
#define ASSERT(_cond_, ...)  do { if (!(_cond_)) { eprintf("ASSERT:  "); eprintf(__VA_ARGS__); eprintf(" [%s:%i]\n", __FILE__,__LINE__); exit(1); } } while (0)

#define MALLOC(type, n) (type *)malloc((n) * sizeof(type))
#define CALLOC(type, n) (type *)calloc((n),  sizeof(type))
#define REALLOC(ptr, type, n) (type *)realloc((ptr), (n) * sizeof(type))
#define MEMSET(ptr, val, n, type) memset((ptr), (val), (n) * sizeof(type))

#endif

