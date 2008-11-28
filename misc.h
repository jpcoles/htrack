#ifndef MISC_H
#define MISC_H

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

#define ERRORIF(_cond_, _msg_) if (_cond_) { fprintf(stderr, "ERROR: " _msg_ " [%s:%i]\n", __FILE__,__LINE__); exit(1); }

#define MALLOC(type, n) (type *)malloc((n) * sizeof(type))
#define CALLOC(type, n) (type *)calloc(n, sizeof(type))


#endif

