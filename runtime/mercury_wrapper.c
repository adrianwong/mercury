/*
INIT mercury_sys_init_wrapper
ENDINIT
*/
/*
** Copyright (C) 1994-2000 The University of Melbourne.
** This file may only be copied under the terms of the GNU Library General
** Public License - see the file COPYING.LIB in the Mercury distribution.
*/

/*
** file: mercury_wrapper.c
** main authors: zs, fjh
**
**	This file contains the startup and termination entry points
**	for the Mercury runtime.
**
**	It defines mercury_runtime_init(), which is invoked from
**	mercury_init() in the C file generated by util/mkinit.c.
**	The code for mercury_runtime_init() initializes various things, and
**	processes options (which are specified via an environment variable).
**
**	It also defines mercury_runtime_main(), which invokes
**	MR_call_engine(do_interpreter), which invokes main/2.
**
**	It also defines mercury_runtime_terminate(), which performs
**	various cleanups that are needed to terminate cleanly.
*/

#include	"mercury_imp.h"

#include	<stdio.h>
#include	<string.h>

#ifdef MR_MSVC_STRUCTURED_EXCEPTIONS
  #include <excpt.h>
#endif

#include	"mercury_getopt.h"
#include	"mercury_timing.h"
#include	"mercury_init.h"
#include	"mercury_dummy.h"
#include	"mercury_stack_layout.h"
#include	"mercury_trace_base.h"
#include	"mercury_memory.h"	/* for MR_copy_string() */

/* global variables concerned with testing (i.e. not with the engine) */

/* command-line options */

/* size of data areas (including redzones), in kilobytes */
/* (but we later multiply by 1024 to convert to bytes) */
#ifdef MR_DEBUG_AGC
  size_t	heap_size =		  128;
#else
  size_t	heap_size =		 4096;
#endif
size_t		detstack_size =  	 4096;
size_t		nondstack_size =  	  128;
size_t		solutions_heap_size =	 1024;
size_t		global_heap_size =	 1024;
size_t		trail_size =		  128;
size_t		debug_heap_size =	 4096;
size_t		generatorstack_size =  	   32;
size_t		cutstack_size =  	   32;

/* size of the redzones at the end of data areas, in kilobytes */
/* (but we later multiply by 1024 to convert to bytes) */
#ifdef NATIVE_GC
  size_t	heap_zone_size =	   96;
#else
  size_t	heap_zone_size =	   16;
#endif
size_t		detstack_zone_size =	   16;
size_t		nondstack_zone_size =	   16;
size_t		solutions_heap_zone_size = 16;
size_t		global_heap_zone_size =	   16;
size_t		trail_zone_size =	   16;
size_t		debug_heap_zone_size =	   16;
size_t		generatorstack_zone_size = 16;
size_t		cutstack_zone_size =	   16;

/* primary cache size to optimize for, in bytes */
size_t		pcache_size =	         8192;

/* file names for mdb's debugger I/O streams */
const char	*MR_mdb_in_filename = NULL;
const char	*MR_mdb_out_filename = NULL;
const char	*MR_mdb_err_filename = NULL;

/* other options */

bool		check_space = FALSE;

static	bool	benchmark_all_solns = FALSE;
static	bool	use_own_timer = FALSE;
static	int	repeats = 1;

unsigned	MR_num_threads = 1;

/* timing */
int		time_at_last_stat;
int		time_at_start;
static	int	time_at_finish;

/* time profiling */
enum MR_TimeProfileMethod
		MR_time_profile_method = MR_profile_user_plus_system_time;

const char *	progname;
int		mercury_argc;	/* not counting progname */
char **		mercury_argv;
int		mercury_exit_status = 0;

bool		MR_profiling = TRUE;

#ifdef	MR_TYPE_CTOR_STATS

#include	"mercury_type_info.h"
#include	"mercury_array_macros.h"

typedef struct {
	MR_ConstString	type_stat_module;
	MR_ConstString	type_stat_name;
	int		type_stat_ctor_rep;
	long		type_stat_count;
} MR_TypeNameStat;

struct MR_TypeStat_Struct {
	long		type_ctor_reps[MR_TYPECTOR_REP_UNKNOWN + 1];
	MR_TypeNameStat	*type_ctor_names;
	int		type_ctor_name_max;
	int		type_ctor_name_next;
};

/* we depend on these five structs being initialized to zero */
MR_TypeStat	MR_type_stat_mer_unify;
MR_TypeStat	MR_type_stat_c_unify;
MR_TypeStat	MR_type_stat_mer_compare;
MR_TypeStat	MR_type_stat_c_compare;

#endif

/*
** EXTERNAL DEPENDENCIES
**
** - The Mercury runtime initialization, namely mercury_runtime_init(),
**   calls the functions init_gc() and init_modules(), which are in
**   the automatically generated C init file; mercury_init_io(), which is
**   in the Mercury library; and it calls the predicate io__init_state/2
**   in the Mercury library.
** - The Mercury runtime main, namely mercury_runtime_main(),
**   calls main/2 in the user's program.
** - The Mercury runtime finalization, namely mercury_runtime_terminate(),
**   calls io__finalize_state/2 in the Mercury library.
** - `aditi__connect/6' in extras/aditi/aditi.m calls
**   MR_do_load_aditi_rl_code() in the automatically
**   generated C init file.
**
** But, to enable Quickstart of shared libraries on Irix 5,
** and in general to avoid various other complications
** with shared libraries and/or Windows DLLs,
** we need to make sure that we don't have any undefined
** external references when building the shared libraries.
** Hence the statically linked init file saves the addresses of those
** procedures in the following global variables.
** This ensures that there are no cyclic dependencies;
** the order is user program -> trace -> browser -> library -> runtime -> gc,
** where `->' means "depends on", i.e. "references a symbol of".
*/

void	(*address_of_mercury_init_io)(void);
void	(*address_of_init_modules)(void);

int	(*MR_address_of_do_load_aditi_rl_code)(void);

char *	(*MR_address_of_trace_getline)(const char *, FILE *, FILE *);

#ifdef	MR_USE_EXTERNAL_DEBUGGER
void	(*MR_address_of_trace_init_external)(void);
void	(*MR_address_of_trace_final_external)(void);
#endif

#ifdef CONSERVATIVE_GC
void	(*address_of_init_gc)(void);
#endif

#ifdef MR_HIGHLEVEL_CODE
void	(*program_entry_point)(void);
		/* normally main_2_p_0 (main/2) */
#else
MR_Code	*program_entry_point;
		/* normally mercury__main_2_0 (main/2) */
#endif

void	(*MR_library_initializer)(void);
		/* normally ML_io_init_state (io__init_state/2)*/
void	(*MR_library_finalizer)(void);
		/* normally ML_io_finalize_state (io__finalize_state/2) */

void	(*MR_io_stderr_stream)(MR_Word *);
void	(*MR_io_stdout_stream)(MR_Word *);
void	(*MR_io_stdin_stream)(MR_Word *);
void	(*MR_io_print_to_cur_stream)(MR_Word, MR_Word);
void	(*MR_io_print_to_stream)(MR_Word, MR_Word, MR_Word);

void	(*MR_DI_output_current_ptr)(MR_Integer, MR_Integer, MR_Integer, MR_Word, MR_String,
		MR_String, MR_Integer, MR_Integer, MR_Integer, MR_Word, MR_String, MR_Word, MR_Word);
		/* normally ML_DI_output_current (output_current/13) */
bool	(*MR_DI_found_match)(MR_Integer, MR_Integer, MR_Integer, MR_Word, MR_String, MR_String,
		MR_Integer, MR_Integer, MR_Integer, MR_Word, MR_String, MR_Word);
		/* normally ML_DI_found_match (output_current/12) */
void	(*MR_DI_read_request_from_socket)(MR_Word, MR_Word *, MR_Integer *);

/*
** This variable has been replaced by MR_io_print_to_*_stream,
** but the installed mkinit executable may still generate references to it.
** We must therefore keep it until all obsolete mkinit executables have
** been retired.
*/

MR_Code	*MR_library_trace_browser;

MR_Code	*(*volatile MR_trace_func_ptr)(const MR_Stack_Layout_Label *);

void	(*MR_address_of_trace_interrupt_handler)(void);

void	(*MR_register_module_layout)(const MR_Module_Layout *);

#ifdef USE_GCC_NONLOCAL_GOTOS

#define	SAFETY_BUFFER_SIZE	1024	/* size of stack safety buffer */
#define	MAGIC_MARKER_2		142	/* a random character */

#endif

static	void	process_args(int argc, char **argv);
static	void	process_environment_options(void);
static	void	process_options(int argc, char **argv);
static	void	usage(void);
static	void	make_argv(const char *, char **, char ***, int *);

#ifdef MEASURE_REGISTER_USAGE
static	void	print_register_usage_counts(void);
#endif
#ifdef MR_TYPE_CTOR_STATS
static	void	MR_print_type_ctor_stats(void);
static	void	MR_print_one_type_ctor_stat(FILE *fp, const char *op,
			MR_TypeStat *type_stat);
#endif

#ifdef MR_HIGHLEVEL_CODE
  static void do_interpreter(void);
#else
  Declare_entry(do_interpreter);
#endif

/*---------------------------------------------------------------------------*/

void
mercury_runtime_init(int argc, char **argv)
{
	bool	saved_trace_enabled;

#if NUM_REAL_REGS > 0
	MR_Word c_regs[NUM_REAL_REGS];
#endif

	/*
	** Save the callee-save registers; we're going to start using them
	** as global registers variables now, which will clobber them,
	** and we need to preserve them, because they're callee-save,
	** and our caller may need them ;-)
	*/
	save_regs_to_mem(c_regs);

#if defined(MR_LOWLEVEL_DEBUG) || defined(MR_TABLE_DEBUG)
	/*
	** Ensure stdio & stderr are unbuffered even if redirected.
	** Using setvbuf() is more complicated than using setlinebuf(),
	** but also more portable.
	*/

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
#endif

#ifdef CONSERVATIVE_GC
	MR_init_conservative_GC();
#endif

	/*
	** Process the command line and the options in the environment
	** variable MERCURY_OPTIONS, and save results in global variables.
	*/

	process_args(argc, argv);
	process_environment_options();

	/*
	** Some of the rest of this function may call Mercury code
	** that may have been compiled with tracing (e.g. the initialization
	** routines in the library called via MR_library_initializer).
	** Since this initialization code shouldn't be traced, we disable
	** tracing until the end of this function.
	*/

	saved_trace_enabled = MR_trace_enabled;
	MR_trace_enabled = FALSE;

#ifdef MR_NEED_INITIALIZATION_AT_START
	do_init_modules();
#endif

	(*address_of_mercury_init_io)();

#ifndef MR_HIGHLEVEL_CODE
	/* start up the Mercury engine */
  #ifndef MR_THREAD_SAFE
	init_thread(MR_use_now);
  #else
	{
		int i;
		init_thread_stuff();
		init_thread(MR_use_now);
		MR_exit_now = FALSE;
		for (i = 1 ; i < MR_num_threads ; i++)
			create_thread(NULL);
	}
  #endif /* ! MR_THREAD_SAFE */
#endif /* ! MR_HIGHLEVEL_CODE */

	/* initialize profiling */
	if (MR_profiling) MR_prof_init();

	/*
	** We need to call save_registers(), since we're about to
	** call a C->Mercury interface function, and the C->Mercury
	** interface convention expects them to be saved.  And before we
	** can do that, we need to call restore_transient_registers(),
	** since we've just returned from a C call.
	*/
	restore_transient_registers();
	save_registers();

	MR_trace_init();

	/* initialize the Mercury library */
	(*MR_library_initializer)();

#ifndef MR_HIGHLEVEL_CODE
	save_context(&(MR_ENGINE(context)));
#endif

	/*
	** Now the real tracing starts; undo any updates to the trace state
	** made by the trace code in the library initializer.
	*/
	MR_trace_start(saved_trace_enabled);

	/*
	** Restore the callee-save registers before returning,
	** since they may be used by the C code that called us.
	*/
	restore_regs_from_mem(c_regs);

} /* end runtime_mercury_init() */

#ifdef CONSERVATIVE_GC
void
MR_init_conservative_GC(void)
{
	/*
	** sometimes mercury apps fail the GC_is_visible() test.
	** dyn_load.c traverses the entire address space and registers
	** all segments that could possibly have been written to, which
	** makes us suspect that &MR_runqueue_head is not in the registered
	** roots.  So we force a write to that address, which seems to make
	** the problem go away.
	*/
	MR_runqueue_head = NULL;

	GC_quiet = TRUE;

	/*
	** Call GC_INIT() to tell the garbage collector about this DLL.
	** (This is necessary to support Windows DLLs using gnu-win32.)
	*/
	GC_INIT();

	/*
	** call the init_gc() function defined in <foo>_init.c,
	** which calls GC_INIT() to tell the GC about the main program.
	** (This is to work around a Solaris 2.X (X <= 4) linker bug,
	** and also to support Windows DLLs using gnu-win32.)
	*/
	(*address_of_init_gc)();

	/*
	** Double-check that the garbage collector knows about
	** global variables in shared libraries.
	*/
	GC_is_visible(&MR_runqueue_head);

	/* The following code is necessary to tell the conservative */
	/* garbage collector that we are using tagged pointers */
	{
		int i;

		for (i = 1; i < (1 << TAGBITS); i++) {
			GC_REGISTER_DISPLACEMENT(i);
		}
	}
}
#endif /* CONSERVATIVE_GC */

void 
do_init_modules(void)
{
	static	bool	done = FALSE;

	if (! done) {
		(*address_of_init_modules)();
		MR_close_prof_decl_file();
		done = TRUE;
	}
}

/*
** Given a string, parse it into arguments and create an argv vector for it.
** Returns args, argv, and argc.  It is the caller's responsibility to
** MR_GC_free() args and argv when they are no longer needed.
*/

static void
make_argv(const char *string, char **args_ptr, char ***argv_ptr, int *argc_ptr)
{
	char *args;
	char **argv;
	const char *s = string;
	char *d;
	int args_len = 0;
	int argc = 0;
	int i;
	
	/*
	** First do a pass over the string to count how much space we need to
	** allocate
	*/

	for (;;) {
		/* skip leading whitespace */
		while(MR_isspace(*s)) {
			s++;
		}

		/* are there any more args? */
		if(*s != '\0') {
			argc++;
		} else {
			break;
		}

		/* copy arg, translating backslash escapes */
		if (*s == '"') {
			s++;
			/* "double quoted" arg - scan until next double quote */
			while (*s != '"') {
				if (s == '\0') {
					fatal_error(
				"Mercury runtime: unterminated quoted string\n"
				"in MERCURY_OPTIONS environment variable\n"
					);
				}
				if (*s == '\\')
					s++;
				args_len++; s++;
			}
			s++;
		} else {
			/* ordinary white-space delimited arg */
			while(*s != '\0' && !MR_isspace(*s)) {
				if (*s == '\\')
					s++;
				args_len++; s++;
			}
		}
		args_len++;
	} /* end for */

	/*
	** Allocate the space
	*/
	args = MR_GC_NEW_ARRAY(char, args_len);
	argv = MR_GC_NEW_ARRAY(char *, argc + 1);

	/*
	** Now do a pass over the string, copying the arguments into `args'
	** setting up the contents of `argv' to point to the arguments.
	*/
	s = string;
	d = args;
	for(i = 0; i < argc; i++) {
		/* skip leading whitespace */
		while(MR_isspace(*s)) {
			s++;
		}

		/* are there any more args? */
		if(*s != '\0') {
			argv[i] = d;
		} else {
			argv[i] = NULL;
			break;
		}

		/* copy arg, translating backslash escapes */
		if (*s == '"') {
			s++;
			/* "double quoted" arg - scan until next double quote */
			while (*s != '"') {
				if (*s == '\\')
					s++;
				*d++ = *s++;
			}
			s++;
		} else {
			/* ordinary white-space delimited arg */
			while(*s != '\0' && !MR_isspace(*s)) {
				if (*s == '\\')
					s++;
				*d++ = *s++;
			}
		}
		*d++ = '\0';
	} /* end for */

	*args_ptr = args;
	*argv_ptr = argv;
	*argc_ptr = argc;
} /* end make_argv() */


/*  
**  process_args() is a function that sets some global variables from the
**  command line.  `mercury_arg[cv]' are `arg[cv]' without the program name.
**  `progname' is program name.
*/

static void
process_args( int argc, char ** argv)
{
	progname = argv[0];
	mercury_argc = argc - 1;
	mercury_argv = argv + 1;
}


/*
**  process_environment_options() is a function to parse the MERCURY_OPTIONS
**  environment variable.  
*/ 

static void
process_environment_options(void)
{
	char*	options;

	options = getenv("MERCURY_OPTIONS");
	if (options != NULL) {
		const char	*cmd;
		char		*arg_str, **argv;
		char		*dummy_command_line;
		int		argc;

		/*
		** getopt() expects the options to start in argv[1],
		** not argv[0], so we need to insert a dummy program
		** name (we use "mercury_runtime") at the start of the
		** options before passing them to make_argv() and then
		** to getopt().
		*/
		cmd = "mercury_runtime ";
		dummy_command_line = MR_GC_NEW_ARRAY(char,
					strlen(options) + strlen(cmd) + 1);
		strcpy(dummy_command_line, cmd);
		strcat(dummy_command_line, options);
		
		make_argv(dummy_command_line, &arg_str, &argv, &argc);
		MR_GC_free(dummy_command_line);

		process_options(argc, argv);

		MR_GC_free(arg_str);
		MR_GC_free(argv);
	}
}

enum MR_long_option {
	MR_HEAP_SIZE,
	MR_DETSTACK_SIZE,
	MR_NONDETSTACK_SIZE,
	MR_TRAIL_SIZE,
	MR_HEAP_REDZONE_SIZE,
	MR_DETSTACK_REDZONE_SIZE,
	MR_NONDETSTACK_REDZONE_SIZE,
	MR_TRAIL_REDZONE_SIZE,
	MR_MDB_TTY,
	MR_MDB_IN,
	MR_MDB_OUT,
	MR_MDB_ERR
};

struct MR_option MR_long_opts[] = {
	{ "heap-size", 			1, 0, MR_HEAP_SIZE },
	{ "detstack-size", 		1, 0, MR_DETSTACK_SIZE },
	{ "nondetstack-size", 		1, 0, MR_NONDETSTACK_SIZE },
	{ "trail-size", 		1, 0, MR_TRAIL_SIZE },
	{ "heap-redzone-size", 		1, 0, MR_HEAP_REDZONE_SIZE },
	{ "detstack-redzone-size", 	1, 0, MR_DETSTACK_REDZONE_SIZE },
	{ "nondetstack-redzone-size", 	1, 0, MR_NONDETSTACK_REDZONE_SIZE },
	{ "trail-redzone-size", 	1, 0, MR_TRAIL_REDZONE_SIZE },
	{ "mdb-tty", 			1, 0, MR_MDB_TTY },
	{ "mdb-in", 			1, 0, MR_MDB_IN },
	{ "mdb-out", 			1, 0, MR_MDB_OUT },
	{ "mdb-err", 			1, 0, MR_MDB_ERR }
};

static void
process_options(int argc, char **argv)
{
	unsigned long	size;
	int		c;
	int		long_index;

	while ((c = MR_getopt_long(argc, argv, "acC:d:e:D:i:m:o:P:pr:tT:x",
		MR_long_opts, &long_index)) != EOF)
	{
		switch (c)
		{

		case MR_HEAP_SIZE:
			if (sscanf(MR_optarg, "%lu", &size) != 1)
				usage();

			heap_size = size;
			break;

		case MR_DETSTACK_SIZE:
			if (sscanf(MR_optarg, "%lu", &size) != 1)
				usage();

			detstack_size = size;
			break;

		case MR_NONDETSTACK_SIZE:
			if (sscanf(MR_optarg, "%lu", &size) != 1)
				usage();

			nondstack_size = size;
			break;

		case MR_TRAIL_SIZE:
			if (sscanf(MR_optarg, "%lu", &size) != 1)
				usage();

			trail_size = size;
			break;

		case MR_HEAP_REDZONE_SIZE:
			if (sscanf(MR_optarg, "%lu", &size) != 1)
				usage();

			heap_zone_size = size;
			break;

		case MR_DETSTACK_REDZONE_SIZE:
			if (sscanf(MR_optarg, "%lu", &size) != 1)
				usage();

			detstack_zone_size = size;
			break;

		case MR_NONDETSTACK_REDZONE_SIZE:
			if (sscanf(MR_optarg, "%lu", &size) != 1)
				usage();

			nondstack_zone_size = size;
			break;

		case MR_TRAIL_REDZONE_SIZE:
			if (sscanf(MR_optarg, "%lu", &size) != 1)
				usage();

			trail_zone_size = size;
			break;

		case 'i':
		case MR_MDB_IN:
			MR_mdb_in_filename = MR_copy_string(MR_optarg);
			break;

		case 'o':
		case MR_MDB_OUT:
			MR_mdb_out_filename = MR_copy_string(MR_optarg);
			break;

		case 'e':
		case MR_MDB_ERR:
			MR_mdb_err_filename = MR_copy_string(MR_optarg);
			break;

		case 'm':
		case MR_MDB_TTY:
			MR_mdb_in_filename = MR_copy_string(MR_optarg);
			MR_mdb_out_filename = MR_copy_string(MR_optarg);
			MR_mdb_err_filename = MR_copy_string(MR_optarg);
			break;

		case 'a':
			benchmark_all_solns = TRUE;
			break;

		case 'c':
			check_space = TRUE;
			break;

		case 'C':
			if (sscanf(MR_optarg, "%lu", &size) != 1)
				usage();

			pcache_size = size * 1024;

			break;

		case 'd':	
			if (streq(MR_optarg, "a")) {
				MR_calldebug      = TRUE;
				MR_nondstackdebug = TRUE;
				MR_detstackdebug  = TRUE;
				MR_heapdebug      = TRUE;
				MR_gotodebug      = TRUE;
				MR_sregdebug      = TRUE;
				MR_finaldebug     = TRUE;
				MR_tracedebug     = TRUE;
#ifdef CONSERVATIVE_GC
				GC_quiet = FALSE;
#endif
			}
			else if (streq(MR_optarg, "b"))
				MR_nondstackdebug = TRUE;
			else if (streq(MR_optarg, "c"))
				MR_calldebug    = TRUE;
			else if (streq(MR_optarg, "d"))
				MR_detaildebug  = TRUE;
			else if (streq(MR_optarg, "f"))
				MR_finaldebug   = TRUE;
			else if (streq(MR_optarg, "g"))
				MR_gotodebug    = TRUE;
			else if (streq(MR_optarg, "G"))
#ifdef CONSERVATIVE_GC
				GC_quiet = FALSE;
#else
				; /* ignore inapplicable option */
#endif
			else if (streq(MR_optarg, "h"))
				MR_heapdebug    = TRUE;
			else if (streq(MR_optarg, "H"))
				MR_hashdebug    = TRUE;
			else if (streq(MR_optarg, "m"))
				MR_memdebug     = TRUE;
			else if (streq(MR_optarg, "p"))
				MR_progdebug    = TRUE;
			else if (streq(MR_optarg, "r"))
				MR_sregdebug    = TRUE;
			else if (streq(MR_optarg, "s"))
				MR_detstackdebug  = TRUE;
			else if (streq(MR_optarg, "S"))
				MR_tablestackdebug = TRUE;
			else if (streq(MR_optarg, "t"))
				MR_tracedebug   = TRUE;
			else if (streq(MR_optarg, "T"))
				MR_tabledebug   = TRUE;
			else
				usage();

			use_own_timer = FALSE;
			break;

		case 'D':
			MR_trace_enabled = TRUE;

			if (streq(MR_optarg, "i"))
				MR_trace_handler = MR_TRACE_INTERNAL;
#ifdef	MR_USE_EXTERNAL_DEBUGGER
			else if (streq(MR_optarg, "e"))
				MR_trace_handler = MR_TRACE_EXTERNAL;
#endif

			else
				usage();

			break;

		case 'p':
			MR_profiling = FALSE;
			break;

		case 'P':
#ifdef	MR_THREAD_SAFE
			if (sscanf(MR_optarg, "%u", &MR_num_threads) != 1)
				usage();

			if (MR_num_threads < 1)
				usage();

#endif
			break;

		case 'r':	
			if (sscanf(MR_optarg, "%d", &repeats) != 1)
				usage();

			break;

		case 't':	
			use_own_timer = TRUE;

			MR_calldebug      = FALSE;
			MR_nondstackdebug = FALSE;
			MR_detstackdebug  = FALSE;
			MR_heapdebug      = FALSE;
			MR_gotodebug      = FALSE;
			MR_sregdebug      = FALSE;
			MR_finaldebug     = FALSE;
			break;

		case 'T':
			if (streq(MR_optarg, "r")) {
				MR_time_profile_method = MR_profile_real_time;
			} else if (streq(MR_optarg, "v")) {
				MR_time_profile_method = MR_profile_user_time;
			} else if (streq(MR_optarg, "p")) {
				MR_time_profile_method =
					MR_profile_user_plus_system_time;
			} else {
				usage();
			}
			break;

		case 'x':
#ifdef CONSERVATIVE_GC
			GC_dont_gc = TRUE;
#endif

			break;

		default:	
			usage();

		} /* end switch */
	} /* end while */

	if (MR_optind != argc) {
		printf("The MERCURY_OPTIONS environment variable contains "
			"the word `%s'\n"
			"which is not an option. Please refer to the "
			"Environment Variables section\n"
			"of the Mercury user's guide for details.\n",
			argv[MR_optind]);
		fflush(stdout);
		exit(1);
	}
} /* end process_options() */

static void 
usage(void)
{
	printf("The MERCURY_OPTIONS environment variable "
		"contains an invalid option.\n"
		"Please refer to the Environment Variables section of "
		"the Mercury\nuser's guide for details.\n");
	fflush(stdout);
	exit(1);
} /* end usage() */

/*---------------------------------------------------------------------------*/

void 
mercury_runtime_main(void)
{
#if NUM_REAL_REGS > 0
	MR_Word c_regs[NUM_REAL_REGS];
#endif

#if defined(MR_LOWLEVEL_DEBUG) && defined(USE_GCC_NONLOCAL_GOTOS)
	unsigned char	safety_buffer[SAFETY_BUFFER_SIZE];
#endif

	static	int	repcounter;

#ifdef MR_MSVC_STRUCTURED_EXCEPTIONS
	/*
	** Under Win32 we use the following construction to handle exceptions.
	**   __try
	**   {
	**     <various stuff>
	**   }
	**   __except(MR_filter_win32_exception(GetExceptionInformation())
	**   {
	**   }
	**
	** This type of contruction allows us to retrieve all the information
	** we need (exception type, address, etc) to display a "meaningful"
	** message to the user.  Using signal() in Win32 is less powerful,
	** since we can only trap a subset of all possible exceptions, and
	** we can't retrieve the exception address.  The VC runtime implements
	** signal() by surrounding main() with a __try __except block and
	** calling the signal handler in the __except filter, exactly the way
	** we do it here.
	*/
	__try
	{
#endif

	/*
	** Save the C callee-save registers
	** and restore the Mercury registers
	*/
	save_regs_to_mem(c_regs);
	restore_registers();

#if defined(MR_LOWLEVEL_DEBUG) && defined(USE_GCC_NONLOCAL_GOTOS)
	/*
	** double-check to make sure that we're not corrupting
	** the C stack with these non-local gotos, by filling
	** a buffer with a known value and then later checking
	** that it still contains only this value
	*/

	global_pointer_2 = safety_buffer;	/* defeat optimization */
	memset(safety_buffer, MAGIC_MARKER_2, SAFETY_BUFFER_SIZE);
#endif

#ifdef MR_LOWLEVEL_DEBUG
  #ifndef CONSERVATIVE_GC
	MR_ENGINE(heap_zone)->max      = MR_ENGINE(heap_zone)->min;
  #endif
	MR_CONTEXT(detstack_zone)->max  = MR_CONTEXT(detstack_zone)->min;
	MR_CONTEXT(nondetstack_zone)->max = MR_CONTEXT(nondetstack_zone)->min;
#endif

	time_at_start = MR_get_user_cpu_miliseconds();
	time_at_last_stat = time_at_start;

	for (repcounter = 0; repcounter < repeats; repcounter++) {
#ifdef MR_HIGHLEVEL_CODE
		do_interpreter();
#else
		debugmsg0("About to call engine\n");
		(void) MR_call_engine(ENTRY(do_interpreter), FALSE);
		debugmsg0("Returning from MR_call_engine()\n");
#endif
	}

        if (use_own_timer) {
		time_at_finish = MR_get_user_cpu_miliseconds();
	}

#if defined(USE_GCC_NONLOCAL_GOTOS) && defined(MR_LOWLEVEL_DEBUG)
	{
		int i;

		for (i = 0; i < SAFETY_BUFFER_SIZE; i++)
			MR_assert(safety_buffer[i] == MAGIC_MARKER_2);
	}
#endif

	if (MR_detaildebug) {
		debugregs("after final call");
	}

#ifdef MR_LOWLEVEL_DEBUG
	if (MR_memdebug) {
		printf("\n");
  #ifndef CONSERVATIVE_GC
		printf("max heap used:      %6ld words\n",
			(long) (MR_ENGINE(heap_zone)->max
				- MR_ENGINE(heap_zone)->min));
  #endif
		printf("max detstack used:  %6ld words\n",
			(long)(MR_CONTEXT(detstack_zone)->max
			       - MR_CONTEXT(detstack_zone)->min));
		printf("max nondstack used: %6ld words\n",
			(long) (MR_CONTEXT(nondetstack_zone)->max
				- MR_CONTEXT(nondetstack_zone)->min));
	}
#endif

#ifdef MEASURE_REGISTER_USAGE
	printf("\n");
	print_register_usage_counts();
#endif

        if (use_own_timer) {
		printf("%8.3fu ",
			((double) (time_at_finish - time_at_start)) / 1000);
	}

#ifdef	MR_TYPE_CTOR_STATS
	MR_print_type_ctor_stats();
#endif

	/*
	** Save the Mercury registers and
	** restore the C callee-save registers before returning,
	** since they may be used by the C code that called us.
	*/
	save_registers();
	restore_regs_from_mem(c_regs);

#ifdef MR_MSVC_STRUCTURED_EXCEPTIONS
	}
	__except(MR_filter_win32_exception(GetExceptionInformation()))
	{
		/* Everything is done in MR_filter_win32_exception */
	}
#endif

} /* end mercury_runtime_main() */

#ifdef	MR_TYPE_CTOR_STATS

static	MR_ConstString	MR_ctor_rep_name[] = {
	MR_CTOR_REP_NAMES
};

#define	MR_INIT_CTOR_NAME_ARRAY_SIZE	10

void
MR_register_type_ctor_stat(MR_TypeStat *type_stat,
	MR_TypeCtorInfo type_ctor_info)
{
	int	i;

	type_stat->type_ctor_reps[type_ctor_info->type_ctor_rep]++;

	for (i = 0; i < type_stat->type_ctor_name_next; i++) {
		/*
		** We can compare pointers instead of using strcmp,
		** because the pointers in the array come from the
		** type_ctor_infos themselves, and there is only one
		** static type_ctor_info for each modulename:typename
		** combination.
		*/

		if (type_stat->type_ctor_names[i].type_stat_module ==
			type_ctor_info->type_ctor_module_name &&
			type_stat->type_ctor_names[i].type_stat_name ==
			type_ctor_info->type_ctor_name)
		{
			type_stat->type_ctor_names[i].type_stat_count++;
			return;
		}
	}

	MR_ensure_room_for_next(type_stat->type_ctor_name, MR_TypeNameStat,
		MR_INIT_CTOR_NAME_ARRAY_SIZE);

	i = type_stat->type_ctor_name_next;
	type_stat->type_ctor_names[i].type_stat_module =
		type_ctor_info->type_ctor_module_name;
	type_stat->type_ctor_names[i].type_stat_name =
		type_ctor_info->type_ctor_name;
	type_stat->type_ctor_names[i].type_stat_ctor_rep =
		type_ctor_info->type_ctor_rep;
	type_stat->type_ctor_names[i].type_stat_count = 1;
	type_stat->type_ctor_name_next++;
}

static void
MR_print_type_ctor_stats(void)
{
	FILE	*fp;

	fp = fopen(MR_TYPE_CTOR_STATS, "a");
	if (fp == NULL) {
		return;
	}

	MR_print_one_type_ctor_stat(fp, "UNIFY", &MR_type_stat_mer_unify);
	MR_print_one_type_ctor_stat(fp, "UNIFY_C", &MR_type_stat_c_unify);
	MR_print_one_type_ctor_stat(fp, "COMPARE", &MR_type_stat_mer_compare);
	MR_print_one_type_ctor_stat(fp, "COMPARE_C", &MR_type_stat_c_compare);

	(void) fclose(fp);
}

static void
MR_print_one_type_ctor_stat(FILE *fp, const char *op, MR_TypeStat *type_stat)
{
	int	i;

	for (i = 0; i < (int) MR_TYPECTOR_REP_UNKNOWN; i++) {
		if (type_stat->type_ctor_reps[i] > 0) {
			fprintf(fp, "%s %s %ld\n", op,
				MR_ctor_rep_name[i],
				type_stat->type_ctor_reps[i]);
		}
	}

	for (i = 0; i < type_stat->type_ctor_name_next; i++) {
		fprintf(fp, "%s %s %s %s %ld\n", op,
			type_stat->type_ctor_names[i].type_stat_module,
			type_stat->type_ctor_names[i].type_stat_name,
			MR_ctor_rep_name[type_stat->
				type_ctor_names[i].type_stat_ctor_rep],
			type_stat->type_ctor_names[i].type_stat_count);
	}
}

#endif

#ifdef MEASURE_REGISTER_USAGE
static void 
print_register_usage_counts(void)
{
	int	i;

	printf("register usage counts:\n");
	for (i = 0; i < MAX_RN; i++) {
		if (1 <= i && i <= ORD_RN) {
			printf("r%d", i);
		} else {
			switch (i) {

			case SI_RN:
				printf("MR_succip");
				break;
			case HP_RN:
				printf("MR_hp");
				break;
			case SP_RN:
				printf("MR_sp");
				break;
			case CF_RN:
				printf("MR_curfr");
				break;
			case MF_RN:
				printf("MR_maxfr");
				break;
			case MR_TRAIL_PTR_RN:
				printf("MR_trail_ptr");
				break;
			case MR_TICKET_COUNTER_RN:
				printf("MR_ticket_counter");
				break;
			case MR_TICKET_HIGH_WATER_RN:
				printf("MR_ticket_high_water");
				break;
			case MR_SOL_HP_RN:
				printf("MR_sol_hp");
				break;
			case MR_MIN_HP_REC:
				printf("MR_min_hp_rec");
				break;
			case MR_MIN_SOL_HP_REC:
				printf("MR_min_sol_hp_rec");
				break;
			case MR_GLOBAL_HP_RN:
				printf("MR_global_hp");
				break;
			case MR_GEN_STACK_RN:
				printf("MR_gen_stack");
				break;
			case MR_GEN_NEXT_RN:
				printf("MR_gen_next");
				break;
			case MR_CUT_STACK_RN:
				printf("MR_cut_stack");
				break;
			case MR_CUT_NEXT_RN:
				printf("MR_cut_next");
				break;
			default:
				printf("UNKNOWN%d", i);
				break;
			}
		}

		printf("\t%lu\n", num_uses[i]);
	} /* end for */
} /* end print_register_usage_counts() */
#endif

#ifdef MR_HIGHLEVEL_CODE

void
do_interpreter(void)
{
  #ifdef  PROFILE_TIME
	if (MR_profiling) MR_prof_turn_on_time_profiling();
  #endif

	/* call the Mercury predicate main/2 */
	(*program_entry_point)();

  #ifdef  PROFILE_TIME
	if (MR_profiling) MR_prof_turn_off_time_profiling();
  #endif
}

#else /* ! MR_HIGHLEVEL_CODE */

Define_extern_entry(do_interpreter);
Declare_label(global_success);
Declare_label(global_fail);
Declare_label(all_done);

BEGIN_MODULE(interpreter_module)
	init_entry_ai(do_interpreter);
	init_label_ai(global_success);
	init_label_ai(global_fail);
	init_label_ai(all_done);
BEGIN_CODE

Define_entry(do_interpreter);
	MR_incr_sp(4);
	MR_stackvar(1) = (MR_Word) MR_hp;
	MR_stackvar(2) = (MR_Word) MR_succip;
	MR_stackvar(3) = (MR_Word) MR_maxfr;
	MR_stackvar(4) = (MR_Word) MR_curfr;

	MR_mkframe("interpreter", 1, LABEL(global_fail));

	MR_nondet_stack_trace_bottom = MR_maxfr;
	MR_stack_trace_bottom = LABEL(global_success);

	if (program_entry_point == NULL) {
		fatal_error("no program entry point supplied");
	}

#ifdef  PROFILE_TIME
	if (MR_profiling) MR_prof_turn_on_time_profiling();
#endif

	noprof_call(program_entry_point, LABEL(global_success));

Define_label(global_success);
#ifdef	MR_LOWLEVEL_DEBUG
	if (MR_finaldebug) {
		save_transient_registers();
		printregs("global succeeded");
		if (MR_detaildebug)
			dumpnondstack();
	}
#endif

	if (benchmark_all_solns)
		MR_redo();
	else
		GOTO_LABEL(all_done);

Define_label(global_fail);
#ifdef	MR_LOWLEVEL_DEBUG
	if (MR_finaldebug) {
		save_transient_registers();
		printregs("global failed");

		if (MR_detaildebug)
			dumpnondstack();
	}
#endif

Define_label(all_done);

#ifdef  PROFILE_TIME
	if (MR_profiling) MR_prof_turn_off_time_profiling();
#endif

	MR_hp     = (MR_Word *) MR_stackvar(1);
	MR_succip = (MR_Code *) MR_stackvar(2);
	MR_maxfr  = (MR_Word *) MR_stackvar(3);
	MR_curfr  = (MR_Word *) MR_stackvar(4);
	MR_decr_sp(4);

#ifdef MR_LOWLEVEL_DEBUG
	if (MR_finaldebug && MR_detaildebug) {
		save_transient_registers();
		printregs("after popping...");
	}
#endif

	proceed();
#ifndef	USE_GCC_NONLOCAL_GOTOS
	return 0;
#endif
END_MODULE

#endif

/*---------------------------------------------------------------------------*/

int
mercury_runtime_terminate(void)
{
#if NUM_REAL_REGS > 0
	MR_Word c_regs[NUM_REAL_REGS];
#endif
	/*
	** Save the callee-save registers; we're going to start using them
	** as global registers variables now, which will clobber them,
	** and we need to preserve them, because they're callee-save,
	** and our caller may need them.
	*/
	save_regs_to_mem(c_regs);

	MR_trace_end();

	(*MR_library_finalizer)();

	/*
	** Restore the registers before calling MR_trace_final()  
	** as MR_trace_final() expect them to be valid.
	*/
	restore_registers(); 

	MR_trace_final();

	if (MR_profiling) MR_prof_finish();

#ifdef	MR_THREAD_SAFE
	MR_exit_now = TRUE;
	pthread_cond_broadcast(MR_runqueue_cond);
#endif

	terminate_engine();

	/*
	** Restore the callee-save registers before returning,
	** since they may be used by the C code that called us.
	*/
	restore_regs_from_mem(c_regs);

	return mercury_exit_status;
}

/*---------------------------------------------------------------------------*/

int
MR_load_aditi_rl_code(void)
{
	if (MR_address_of_do_load_aditi_rl_code != NULL) {
		return (*MR_address_of_do_load_aditi_rl_code)();	
	} else {
		fatal_error(
			"attempt to load Aditi-RL code from an executable\n"
			"not compiled for Aditi execution.\n"
			"Add `--aditi' to C2INITFLAGS.\n"
		);
	}
}

/*---------------------------------------------------------------------------*/
void mercury_sys_init_wrapper(void); /* suppress gcc warning */
void mercury_sys_init_wrapper(void) {
#ifndef MR_HIGHLEVEL_CODE
	interpreter_module();
#endif
}
