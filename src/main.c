/**
 * Siege, http regression tester / benchmark utility
 *
 * Copyright (C) 2000-2015 by  
 * Jeffrey Fulmer - <jeff@joedog.org>, et al. 
 * This file is distributed as part of Siege
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#define  INTERN  1

#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#ifdef  HAVE_PTHREAD_H
# include <pthread.h>
#endif/*HAVE_PTHREAD_H*/

/*LOCAL HEADERS*/
#include <setup.h>
#include <array.h>
#include <handler.h>
#include <timer.h>
#include <browser.h>
#include <util.h>
#include <log.h>
#include <init.h>
#include <cfg.h>
#include <url.h>
#include <ssl.h>
#include <cookies.h>
#include <crew.h>
#include <data.h>
#include <version.h>
#include <memory.h>
#include <notify.h>
#include <sys/resource.h>
#include <time.h>
#ifdef __CYGWIN__
# include <getopt.h>
#else
# include <joedog/getopt.h>
#endif 

/**
 * long options, std options struct
 */
static struct option long_options[] =
{
  { "version",      no_argument,       NULL, 'V' },
  { "help",         no_argument,       NULL, 'h' },
  { "verbose",      no_argument,       NULL, 'v' },
  { "quiet",        no_argument,       NULL, 'q' },
  { "config",       no_argument,       NULL, 'C' },
  { "debug",        no_argument,       NULL, 'D' },
  { "get",          no_argument,       NULL, 'g' },
  { "print",        no_argument,       NULL, 'p' },
  { "concurrent",   required_argument, NULL, 'c' },
  { "no-parser",    no_argument,       NULL, 'N' },
  { "no-follow",    no_argument,       NULL, 'F' },
  { "internet",     no_argument,       NULL, 'i' },
  { "benchmark",    no_argument,       NULL, 'b' },
  { "reps",         required_argument, NULL, 'r' },
  { "time",         required_argument, NULL, 't' },
  { "delay",        required_argument, NULL, 'd' },
  { "log",          optional_argument, NULL, 'l' },
  { "file",         required_argument, NULL, 'f' },
  { "rc",           required_argument, NULL, 'R' }, 
  { "mark",         required_argument, NULL, 'm' },
  { "header",       required_argument, NULL, 'H' },
  { "user-agent",   required_argument, NULL, 'A' },
  { "user-agent-file", required_argument, NULL, 1000 },
  { "user-agent-mode", required_argument, NULL, 1001 },
  { "usera-agents", optional_argument, NULL, 'u' },
  { "user-agents",  optional_argument, NULL, 'u' },
  { "content-type", required_argument, NULL, 'T' },
  { "json-output",  no_argument,       NULL, 'j' },
  { "wp-search",    required_argument, NULL, 1002 },
  { "wp-search-terms", required_argument, NULL, 1003 },
  { "wp-litespeed-owasp", no_argument, NULL, 1004 },
  { "nocache",      required_argument, NULL, 1005 },
  { "auto-tune",    no_argument,       NULL, 1006 },
  { "autotune",     no_argument,       NULL, 1006 },
  { "thread-stack", required_argument, NULL, 1007 },
  { "limit",        required_argument, NULL, 1008 },
  { "crash-guard",  no_argument,       NULL, 1009 },
  { "no-core-dump", no_argument,       NULL, 1010 },
  {0, 0, 0, 0}
};

/**
 * display_version   
 * displays the version number and exits on boolean false. 
 * continue running? TRUE=yes, FALSE=no
 * return void
 */

void 
display_version(BOOLEAN b)
{
  /**
   * version_string is defined in version.c 
   * adding it to a separate file allows us
   * to parse it in configure.  
   */
  char name[128]; 

  xstrncpy(name, program_name, sizeof(name));

  if (my.debug) {
    fprintf(stderr,"%s %s: debugging enabled\n\n%s\n", uppercase(name, strlen(name)), version_string, copyright);
  } else {
    if (b == TRUE) {
      fprintf(stderr,"%s %s\n\n%s\n", uppercase(name, strlen(name)), version_string, copyright);
      exit(EXIT_SUCCESS);
    } else {
      fprintf(stderr,"%s %s\n", uppercase(name, strlen(name)), version_string);
    }
  }
}  /* end of display version */

/**
 * display_help 
 * displays the help section to STDOUT and exits
 */ 
void 
display_help()
{
  /**
   * call display_version, but do not exit 
   */
  display_version(FALSE); 
  printf("Usage: %s [options]\n", program_name);
  printf("       %s [options] URL\n", program_name);
  printf("       %s -g URL\n", program_name);
  printf("Options:\n"                    );
  puts("  -V, --version             VERSION, prints the version number.");
  puts("  -h, --help                HELP, prints this section.");
  puts("  -C, --config              CONFIGURATION, show the current config.");
  puts("  -v, --verbose             VERBOSE, prints notification to screen.");
  puts("  -q, --quiet               QUIET turns verbose off and suppresses output.");
  puts("  -g, --get                 GET, pull down HTTP headers and display the");
  puts("                            transaction. Great for application debugging.");
  puts("  -p, --print               PRINT, like GET only it prints the entire page.");
  puts("  -c, --concurrent=NUM      CONCURRENT users, default is 10");
  puts("  -r, --reps=NUM            REPS, number of times to run the test." );
  puts("  -t, --time=NUMm           TIMED testing where \"m\" is modifier S, M, or H");
  puts("                            ex: --time=1H, one hour test." );
  puts("  -d, --delay=NUM           Time DELAY, random delay before each request");
  puts("  -b, --benchmark           BENCHMARK: no delays between requests." );
  puts("  -i, --internet            INTERNET user simulation, hits URLs randomly.");
  puts("  -f, --file=FILE           FILE, select a specific URLS FILE." );
  printf("  -R, --rc=FILE             RC, specify an %src file\n",program_name);
  puts("  -l, --log[=FILE]          LOG to FILE. If FILE is not specified, the");
  printf("                            default is used: PREFIX/var/%s.log\n", program_name);
  puts("  -m, --mark=\"text\"         MARK, mark the log file with a string." );
  puts("                            between .001 and NUM. (NOT COUNTED IN STATS)");
  puts("  -H, --header=\"text\"       Add a header to request (can be many)" ); 
  puts("  -A, --user-agent=\"text\"   Sets User-Agent in request" ); 
  puts("      --user-agent-file=FILE Rotate User-Agent strings from FILE" );
  puts("      --user-agent-mode=MODE fixed, round-robin, or random" );
  puts("  -u, --usera-agents[=NUM]  Use bundled User-Agent list, optional limit" );
  puts("  -T, --content-type=\"text\" Sets Content-Type in request" ); 
  puts("  -j, --json-output         JSON OUTPUT, print final stats to stdout as JSON");
  puts("      --wp-search=URL       Generate WordPress ?s= search probes for URL");
  puts("      --wp-search-terms=FILE Read WordPress search probe terms from FILE");
  puts("      --wp-litespeed-owasp  Add LiteSpeed/OWASP regression search probes");
  puts("      --nocache=NUM         Generate numbered nocache URL variants");
  puts("      --auto-tune           Optimize concurrency and generated URLs here");
  puts("      --thread-stack=KB     Worker thread stack size for high concurrency");
  puts("      --limit=NUM           Raise/lower the configured thread cap");
  puts("      --crash-guard         Print controlled fatal-signal diagnostics");
  puts("      --no-core-dump        Disable writing core dump files");
  puts("      --no-parser           NO PARSER, turn off the HTML page parser");
  puts("      --no-follow           NO FOLLOW, do not follow HTTP redirects");
  puts("");
  puts(copyright);
  /**
   * our work is done, exit nicely
   */
  exit(EXIT_SUCCESS);
}

/* Check the command line for the presence of the -R or --RC switch.  We
 * need to do this separately from the other command line switches because
 * the options are initialized from the .siegerc file before the command line
 * switches are parsed. The argument index is reset before leaving the
 * function. */
void 
parse_rc_cmdline(int argc, char *argv[])
{
  int a = 0;
  strcpy(my.rc, "");
  
  while( a > -1 ){
    a = getopt_long(argc, argv, "VhvqCDNFpgl::ibu::r:t:f:d:c:m:H:R:A:T:j", long_options, (int*)0);
    if(a == 'R'){
      strcpy(my.rc, optarg);
      a = -1;
    }
  }
  optind = 0;
} 

/**
 * parses command line arguments and assigns
 * values to run time variables. relies on GNU
 * getopts included with this distribution.  
 */ 
void 
parse_cmdline(int argc, char *argv[])
{
  int c = 0;
  int nargs;
  while ((c = getopt_long(argc, argv, "VhvqCDNFpgl::ibu::r:t:f:d:c:m:H:R:A:T:j", long_options, (int *)0)) != EOF) {
  switch (c) {
      case 'V':
        display_version(TRUE);
        break;
      case 'h':
        display_help();
        exit(EXIT_SUCCESS);
      case 'D':
        my.debug = TRUE;
        break;
      case 'C':
        my.config = TRUE;
        my.get    = FALSE;
        break;
      case 'c':
        my.cusers  = atoi(optarg);
        my.cusers_set = TRUE;
        break;
      case 'i':
        my.internet = TRUE;
        break;
      case 'b':
        my.bench    = TRUE;
        break;
      case 'd':
	/* XXX range checking? use strtol? */
        my.delay   = atof(optarg);
	if(my.delay < 0){
	  my.delay = 0; 
	}
        break;
      case 'g':
        my.get = TRUE;
        break;
      case 'p':
        my.print  = TRUE;
        my.cusers = 1;
        my.cusers_set = TRUE;
        my.reps   = 1;
        break;
      case 'l':
        my.logging = TRUE;
        if (optarg) {
          if (strlen(optarg) > sizeof(my.logfile)) {
            fprintf(stderr, "ERROR: -l/--logfile is limited to %ld in length", sizeof(my.logfile));
            exit(1);
          }
          xstrncpy(my.logfile, optarg, strlen(optarg)+1);
        } 
        break;
      case 'm':
        my.mark    = TRUE;
        my.markstr = optarg;
        my.logging = TRUE; 
        break;
      case 'q':
        my.quiet   = TRUE;
        break;
      case 'v':
        my.verbose = TRUE;
        break;
      case 'r':
        if(strmatch(optarg, "once")){
           my.reps = -1;
        } else {
          my.reps = atoi(optarg);
        }
        break;
      case 't':
        parse_time(optarg);
        break;
      case 'f':
        if(optarg == NULL) break; /*paranoia*/
        xstrncpy(my.file, optarg, strlen(optarg)+1);
        break;
      case 'A':
        strncpy(my.uagent, optarg, 255);
        break;
      case 1000:
        xstrncpy(my.uafile, optarg, sizeof(my.uafile));
        my.uamode = UA_ROUND_ROBIN;
        break;
      case 1001:
        my.uamode_set = TRUE;
        if (strmatch(optarg, "random")) {
          my.uamode = UA_RANDOM;
        } else if (strmatch(optarg, "round-robin")) {
          my.uamode = UA_ROUND_ROBIN;
        } else if (strmatch(optarg, "fixed")) {
          my.uamode = UA_FIXED;
        } else {
          NOTIFY(FATAL, "unknown user-agent mode: %s", optarg);
        }
        break;
      case 'u':
        my.ualimit = optarg == NULL ? 0 : atoi(optarg);
        if (my.ualimit < 0) my.ualimit = 0;
        my.uadefault = TRUE;
        if (my.uamode_set == FALSE) my.uamode = UA_ROUND_ROBIN;
        break;
      case 'T':
        strncpy(my.conttype, optarg, 255);
        break;
      case 'N':
        my.parser = FALSE;
        break;
      case 'F':
        my.follow = FALSE;
        break;
      case 'R':  
        /**
         * processed above 
         */
        break; 
      case 'H':
        {
          if(!strchr(optarg,':')) NOTIFY(FATAL, "no ':' in http-header");
          if((strlen(optarg) + strlen(my.extra) + 3) > sizeof(my.extra))  // sizeof(*my.extra) == 1, so this is accurate
              NOTIFY(FATAL, "header is too large");
          strcat(my.extra,optarg);
          strcat(my.extra,"\015\012");
        }
        break;
      case 'j':
        my.json_output = TRUE;
        break;
      case 1002:
        my.wp_search = xstrdup(optarg);
        break;
      case 1003:
        xstrncpy(my.wp_terms, optarg, sizeof(my.wp_terms));
        break;
      case 1004:
        my.wp_litespeed = TRUE;
        break;
      case 1005:
        my.nocache = atoi(optarg);
        if (my.nocache < 0) my.nocache = 0;
        break;
      case 1006:
        my.autotune = TRUE;
        break;
      case 1007:
        my.thread_stack_kb = atoi(optarg);
        if (my.thread_stack_kb < 0) my.thread_stack_kb = 0;
        my.thread_stack_set = TRUE;
        break;
      case 1008:
        my.limit = atoi(optarg);
        if (my.limit < 1) my.limit = 1;
        break;
      case 1009:
        my.crash_guard = TRUE;
        my.core_dumps = FALSE;
        break;
      case 1010:
        my.core_dumps = FALSE;
        break;

    } /* end of switch( c )           */
  }   /* end of while c = getopt_long */
  nargs = argc - optind;
  if (nargs)
    my.url = xstrdup(argv[argc-1]); 
  if (my.wp_litespeed == TRUE && my.wp_search == NULL && my.url != NULL) {
    my.wp_search = xstrdup(my.url);
  }
  if (my.get && my.url==NULL) {
    puts("ERROR: -g/--get requires a commandline URL");
    exit(1);
  }
  return;
} /* end of parse_cmdline */

private const char *
__signal_name(int sig)
{
  switch (sig) {
    case SIGSEGV: return "SIGSEGV";
    case SIGABRT: return "SIGABRT";
#ifdef SIGBUS
    case SIGBUS:  return "SIGBUS";
#endif
#ifdef SIGILL
    case SIGILL:  return "SIGILL";
#endif
#ifdef SIGFPE
    case SIGFPE:  return "SIGFPE";
#endif
    default:      return "fatal signal";
  }
}

private void
__fatal_signal_handler(int sig)
{
  const char prefix[] = "\nsiege: caught ";
  const char suffix[] = ". Core dump disabled; reduce concurrency or raise VPS limits if this repeats.\n";
  const char *name = __signal_name(sig);

  write(STDERR_FILENO, prefix, sizeof(prefix) - 1);
  write(STDERR_FILENO, name, strlen(name));
  write(STDERR_FILENO, suffix, sizeof(suffix) - 1);
  _exit(128 + sig);
}

private void
__crash_guard_setup()
{
  struct sigaction sa;

  if (my.core_dumps == FALSE) {
#ifdef RLIMIT_CORE
    struct rlimit lim;
    lim.rlim_cur = 0;
    lim.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &lim);
#endif
  }

  if (my.crash_guard == FALSE) return;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = __fatal_signal_handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGABRT, &sa, NULL);
#ifdef SIGBUS
  sigaction(SIGBUS, &sa, NULL);
#endif
#ifdef SIGILL
  sigaction(SIGILL, &sa, NULL);
#endif
#ifdef SIGFPE
  sigaction(SIGFPE, &sa, NULL);
#endif
}

private void
__signal_setup()
{
  sigset_t sigs;

  sigemptyset(&sigs);
  sigaddset(&sigs, SIGHUP);
  sigaddset(&sigs, SIGINT);
  sigaddset(&sigs, SIGALRM);
  sigaddset(&sigs, SIGTERM);
  sigaddset(&sigs, SIGPIPE);
  sigprocmask(SIG_BLOCK, &sigs, NULL);
}

private long
__sysconf_or_zero(int name)
{
  long value = sysconf(name);
  return value > 0 ? value : 0;
}

private long
__online_cpus()
{
#ifdef _SC_NPROCESSORS_ONLN
  long cpus = __sysconf_or_zero(_SC_NPROCESSORS_ONLN);
  if (cpus > 0) return cpus;
#endif
  return 1;
}

private unsigned long long
__available_memory_mb()
{
  long pages = 0;
  long size  = 0;
  FILE *fp;
  char line[256];
  unsigned long long kb;

  fp = fopen("/proc/meminfo", "r");
  if (fp != NULL) {
    while (fgets(line, sizeof(line), fp) != NULL) {
      if (sscanf(line, "MemAvailable: %llu kB", &kb) == 1) {
        fclose(fp);
        return kb / 1024ULL;
      }
    }
    fclose(fp);
  }

#if defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGESIZE)
  pages = __sysconf_or_zero(_SC_AVPHYS_PAGES);
  size  = __sysconf_or_zero(_SC_PAGESIZE);
  if (pages > 0 && size > 0) {
    return ((unsigned long long)pages * (unsigned long long)size) / (1024ULL * 1024ULL);
  }
#endif

#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
  pages = __sysconf_or_zero(_SC_PHYS_PAGES);
  size  = __sysconf_or_zero(_SC_PAGESIZE);
  if (pages > 0 && size > 0) {
    return ((unsigned long long)pages * (unsigned long long)size) / (1024ULL * 1024ULL);
  }
#endif

  return 0;
}

private long
__soft_limit(int resource)
{
  struct rlimit lim;

  if (getrlimit(resource, &lim) != 0) return 0;
  if (lim.rlim_cur == RLIM_INFINITY) return LONG_MAX;
  if (lim.rlim_cur > (rlim_t)LONG_MAX) return LONG_MAX;
  return (long)lim.rlim_cur;
}

private int
__min_positive_int(int current, long candidate)
{
  if (candidate < 1) return current;
  if (candidate > INT_MAX) candidate = INT_MAX;
  return current < (int)candidate ? current : (int)candidate;
}

private void
__auto_tune()
{
  long cpus;
  long nofile;
  long nproc;
  int  original_cusers;
  int  original_nocache;
  int  original_stack;
  int  max_cusers;
  int  recommended_cusers;
  int  nocache_cap;
  int  worker_mb;
  unsigned long long memory_mb;

  if (my.autotune == FALSE) return;

  my.crash_guard = TRUE;
  my.core_dumps = FALSE;

  cpus              = __online_cpus();
  memory_mb         = __available_memory_mb();
  nofile            = 0;
  nproc             = 0;
  original_cusers   = my.cusers;
  original_nocache  = my.nocache;
  original_stack    = my.thread_stack_kb;
  max_cusers        = my.limit > 0 ? my.limit : 255;

#ifdef RLIMIT_NOFILE
  nofile = __soft_limit(RLIMIT_NOFILE);
#endif
#ifdef RLIMIT_NPROC
  nproc = __soft_limit(RLIMIT_NPROC);
#endif

  if (my.cusers_set == TRUE && my.cusers > max_cusers) {
    max_cusers = my.cusers;
    my.limit = my.cusers;
  }

  if (my.thread_stack_set == FALSE && my.cusers_set == TRUE && my.cusers >= 300) {
    my.thread_stack_kb = my.cusers >= 1000 ? 384 : 512;
  }

  if (my.cusers_set == TRUE) {
    max_cusers = __min_positive_int(max_cusers, cpus * 500);
  } else {
    max_cusers = __min_positive_int(max_cusers, cpus * 100);
  }

  if (memory_mb > 0) {
    unsigned long long usable = memory_mb > 128 ? memory_mb - 128 : memory_mb / 2;
    worker_mb = my.thread_stack_kb > 0 ? ((my.thread_stack_kb + 1023) / 1024) : 4;
    if (worker_mb < 1) worker_mb = 1;
    max_cusers = __min_positive_int(max_cusers, (long)(usable / worker_mb));
  }
  if (nofile > 128 && nofile != LONG_MAX) {
    max_cusers = __min_positive_int(max_cusers, (nofile - 64) / 2);
  }
  if (nproc > 64 && nproc != LONG_MAX) {
    max_cusers = __min_positive_int(max_cusers, (nproc - 16) / 2);
  }
  max_cusers = __min_positive_int(max_cusers, 2000);
  if (max_cusers < 1) max_cusers = 1;

  recommended_cusers = (int)(cpus * 50);
  if (recommended_cusers < 10) recommended_cusers = 10;
  if (recommended_cusers > max_cusers) recommended_cusers = max_cusers;

  if (my.cusers > max_cusers) {
    my.cusers = max_cusers;
  } else if (my.cusers_set == FALSE && my.cusers < recommended_cusers) {
    my.cusers = recommended_cusers;
  }

  if (memory_mb > 0 && memory_mb <= 1024) {
    nocache_cap = 250;
  } else if (memory_mb > 0 && memory_mb <= 2048) {
    nocache_cap = 500;
  } else if (memory_mb > 0 && memory_mb <= 4096) {
    nocache_cap = 1000;
  } else {
    nocache_cap = 2500;
  }

  if (my.nocache > nocache_cap) {
    my.nocache = nocache_cap;
  }

  if (!my.quiet) {
    fprintf(stderr, "Auto-tune: cpus=%ld memory=%lluMB", cpus, memory_mb);
    if (nofile > 0 && nofile != LONG_MAX) fprintf(stderr, " nofile=%ld", nofile);
    if (nproc > 0 && nproc != LONG_MAX) fprintf(stderr, " nproc=%ld", nproc);
    fprintf(stderr, " concurrent=%d", my.cusers);
    if (original_cusers != my.cusers) fprintf(stderr, " (from %d)", original_cusers);
    if (my.thread_stack_kb > 0) {
      fprintf(stderr, " thread-stack=%dKB", my.thread_stack_kb);
      if (original_stack != my.thread_stack_kb) fprintf(stderr, " (auto)");
    }
    if (original_nocache > 0) {
      fprintf(stderr, " nocache=%d", my.nocache);
      if (original_nocache != my.nocache) fprintf(stderr, " (from %d)", original_nocache);
    }
    fprintf(stderr, "\n");
  }
}

private void
__runtime_preflight(int urls_count)
{
  long nofile = 0;
  long nproc  = 0;
  unsigned long long memory_mb = __available_memory_mb();
  unsigned long long stack_mb;
  unsigned long long generated;
  int stack_kb = my.thread_stack_kb > 0 ? my.thread_stack_kb : 8192;

#ifdef RLIMIT_NOFILE
  nofile = __soft_limit(RLIMIT_NOFILE);
#endif
#ifdef RLIMIT_NPROC
  nproc = __soft_limit(RLIMIT_NPROC);
#endif

  if (my.cusers > my.limit) {
    NOTIFY(FATAL, "concurrency %d exceeds limit %d; use --limit=%d if this is intentional", my.cusers, my.limit, my.cusers);
  }

  if (nofile > 0 && nofile != LONG_MAX && nofile < ((long)my.cusers * 2L + 64L)) {
    NOTIFY(FATAL, "open-file limit %ld is too low for %d users; run: ulimit -n %ld", nofile, my.cusers, ((long)my.cusers * 2L + 64L));
  }

  if (nproc > 0 && nproc != LONG_MAX && nproc < ((long)my.cusers + 16L)) {
    NOTIFY(FATAL, "process/thread limit %ld is too low for %d users; raise ulimit -u", nproc, my.cusers);
  }

  stack_mb = ((unsigned long long)stack_kb * (unsigned long long)my.cusers) / 1024ULL;
  if (memory_mb > 0 && stack_mb > (memory_mb * 3ULL / 4ULL)) {
    NOTIFY(FATAL, "worker stacks reserve %lluMB on %lluMB available RAM; use --thread-stack=512 or lower concurrency", stack_mb, memory_mb);
  }

  generated = (unsigned long long)urls_count * (unsigned long long)(my.reps > 0 && my.reps != MAXREPS ? my.reps : 1);
  if (generated > 10000000ULL && !my.quiet) {
    fprintf(stderr, "WARNING: this run schedules a very large request set (%llu URL/repetition slots)\n", generated);
  }
}

private int
__load_lines(LINES *lines, const char *filename, int limit)
{
  FILE *fp;
  char  buf[8192];

  lines->index = 0;
  lines->line  = NULL;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    NOTIFY(FATAL, "unable to open file: %s", filename);
  }

  while (fgets(buf, sizeof(buf), fp) != NULL) {
    char *p = strchr(buf, '\n');
    char **tmp;
    if (p != NULL) *p = '\0';
    p = strchr(buf, '\r');
    if (p != NULL) *p = '\0';
    if (strlen(buf) == 0 || buf[0] == '#') continue;

    tmp = (char**)realloc(lines->line, sizeof(char *) * (lines->index + 1));
    if (tmp == NULL) {
      fclose(fp);
      NOTIFY(FATAL, "unable to allocate memory for lines from: %s", filename);
    }
    lines->line = tmp;
    lines->line[lines->index] = xstrdup(buf);
    lines->index++;
    if (limit > 0 && lines->index >= limit) break;
  }
  fclose(fp);
  return lines->index;
}

private char *
__default_user_agents_file()
{
  static char path[4096];
  char *env  = getenv("SIEGE_USER_AGENTS_FILE");
  char *home = getenv("HOME");

  if (env != NULL && strlen(env) > 0 && access(env, R_OK) == 0) {
    return env;
  }

  if (home != NULL && strlen(home) > 0) {
    snprintf(path, sizeof(path), "%s/.siege/useragents.txt", home);
    if (access(path, R_OK) == 0) return path;
  }

  snprintf(path, sizeof(path), "data/useragents.txt");
  if (access(path, R_OK) == 0) return path;

  snprintf(path, sizeof(path), "/usr/local/share/siege/useragents.txt");
  if (access(path, R_OK) == 0) return path;

  snprintf(path, sizeof(path), "/opt/homebrew/share/siege/useragents.txt");
  if (access(path, R_OK) == 0) return path;

  return NULL;
}

private char *
__url_encode_query(const char *src)
{
  const char hex[] = "0123456789ABCDEF";
  size_t i, j = 0;
  size_t len = strlen(src);
  char *out = xmalloc((len * 3) + 1);

  for (i = 0; i < len; i++) {
    unsigned char c = (unsigned char)src[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out[j++] = c;
    } else if (c == ' ') {
      out[j++] = '+';
    } else {
      out[j++] = '%';
      out[j++] = hex[c >> 4];
      out[j++] = hex[c & 15];
    }
  }
  out[j] = '\0';
  return out;
}

private char *
__build_wp_search_url(const char *base, const char *term)
{
  char *encoded = __url_encode_query(term);
  const char *sep = strchr(base, '?') == NULL ? "?s=" : "&s=";
  size_t len = strlen(base) + strlen(sep) + strlen(encoded) + 1;
  char *url = xmalloc(len);

  snprintf(url, len, "%s%s%s", base, sep, encoded);
  xfree(encoded);
  return url;
}

private char *
__build_nocache_url(const char *base, int value)
{
  const char *sep = strchr(base, '?') == NULL ? "?nocache=" : "&nocache=";
  size_t len = strlen(base) + strlen(sep) + 32;
  char *url = xmalloc(len);

  snprintf(url, len, "%s%s%d", base, sep, value);
  return url;
}

private void
__append_url(ARRAY urls, const char *line, int *id)
{
  URL tmp = new_url((char*)line);
  if (tmp != NULL) {
    url_set_ID(tmp, *id);
    array_npush(urls, tmp, URLSIZE);
    (*id)++;
  }
}

private ARRAY
__expand_nocache_urls(ARRAY urls)
{
  int i, n, id = 0;
  ARRAY expanded = new_array();

  if (my.nocache < 1) return urls;

  for (i = 0; i < (int)array_length(urls); i++) {
    URL source = array_get(urls, i);
    if (source == NULL) continue;

    for (n = 1; n <= my.nocache; n++) {
      char *url = __build_nocache_url(url_get_absolute(source), n);
      __append_url(expanded, url, &id);
      xfree(url);
    }
  }

  urls = array_destroyer(urls, (void*)url_destroy);
  return expanded;
}

private void
__append_wp_search_urls(ARRAY urls, int *id)
{
  int i;
  LINES terms;
  static const char *owasp_terms[] = {
    "wordpress",
    "' OR '1'='1",
    "<script>alert(1)</script>",
    "../wp-config.php",
    "../../etc/passwd",
    "wp-json/wp/v2/users",
    "union select null,user_pass from wp_users",
    "${jndi:ldap://127.0.0.1/a}",
    "admin'--",
    NULL
  };

  if (my.wp_search == NULL) return;

  if (strlen(my.wp_terms) > 0) {
    __load_lines(&terms, my.wp_terms, 0);
    for (i = 0; i < terms.index; i++) {
      char *url = __build_wp_search_url(my.wp_search, terms.line[i]);
      __append_url(urls, url, id);
      xfree(url);
      xfree(terms.line[i]);
    }
    xfree(terms.line);
  } else if (my.wp_litespeed == FALSE) {
    char *url = __build_wp_search_url(my.wp_search, "wordpress");
    __append_url(urls, url, id);
    xfree(url);
  }

  if (my.wp_litespeed == TRUE) {
    for (i = 0; owasp_terms[i] != NULL; i++) {
      char *url = __build_wp_search_url(my.wp_search, owasp_terms[i]);
      __append_url(urls, url, id);
      xfree(url);
    }
  }
}

private void
__config_setup(int argc, char *argv[])
{
  
  memset(&my, '\0', sizeof(struct CONFIG));

  parse_rc_cmdline(argc, argv); 
  if (init_config() < 0) { 
    exit(EXIT_FAILURE); 
  } 
  parse_cmdline(argc, argv);
  ds_module_check(); 
  __auto_tune();
  
  if (my.config) {
    show_config(TRUE);    
  }

  /** 
   * Let's tap the brakes and make sure the user knows what they're doing...
   */ 
  if (my.cusers > my.limit) {
    printf("\n");
    printf("================================================================\n");
    printf("WARNING: The number of users is capped at %d.%sTo increase this\n", my.limit, (my.limit>999)?" ":"  ");
    printf("         limit, search your siege.conf file for 'limit' and change\n");
    printf("         its value. Make sure you read the instructions there.\n");
    printf("         Siege will soon proceed with %d users (unless you abort...)\n", my.limit);
    printf("================================================================\n");
    sleep(10);
    my.cusers = my.limit;
  }
}

private LINES *
__urls_setup() 
{
  LINES * lines;

  lines          = xcalloc(1, sizeof(LINES));
  lines->index   = 0;
  lines->line    = NULL;

  if (my.url != NULL) {
    my.length = 1; 
  } else if (my.wp_search != NULL) {
    my.length = 0;
  } else { 
    my.length = read_cfg_file(lines, my.file); 
  }

  if (my.length == 0 && my.wp_search == NULL) {
    display_help();
  }

  return lines;
}

private BOOLEAN
__save_cookies(char *file, char *text)
{
  FILE * fp;

  fp  = fopen(file, "w");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: Unable to open cookies file: %s\n", file);
    return FALSE;
  }
  fputs("#\n", fp);
  fputs("# Siege cookies file. You may edit this file to add cookies\n",fp);
  fputs("# manually but comments and formatting will be removed.    \n",fp);
  fputs("# All cookies that expire in the future will be preserved. \n",fp);
  fputs("# ---------------------------------------------------------\n",fp);
  fputs(text, fp);
  fclose(fp);
  return TRUE; 
}


int 
main(int argc, char *argv[])
{
  int       i, j;
  int       result   = 0;
  void  *   status   = NULL;
  char      name[]   = "cookies.txt";
  char  *   home     = getenv("HOME");
  int       length   = home ? strlen(home)+strlen(name)+9 : 256;
  char  *   file     = xmalloc(length); 
  LINES *   lines    = NULL;
  CREW      crew     = NULL;
  DATA      data     = NULL;
  ARRAY     urls     = new_array();
  ARRAY     browsers = new_array();
  pthread_t cease; 
  pthread_t timer;  
  pthread_attr_t scope_attr;


  file = xmalloc(sizeof (char*) * length);
  memset(file, '\0', sizeof (char*) * length);
  snprintf(file, length, "%s/.siege/%s", home, name);
 
  __signal_setup();
  __config_setup(argc, argv);
  __crash_guard_setup();
  lines = __urls_setup();
  srand((unsigned int)time(NULL));
  if (my.uadefault == TRUE && strlen(my.uafile) == 0) {
    char *default_uafile = __default_user_agents_file();
    if (default_uafile == NULL) {
      NOTIFY(FATAL, "unable to find bundled useragents.txt; set SIEGE_USER_AGENTS_FILE or use --user-agent-file");
    }
    xstrncpy(my.uafile, default_uafile, sizeof(my.uafile));
  }
  if (strlen(my.uafile) > 0) {
    __load_lines(&my.uagents, my.uafile, my.ualimit);
    if (my.uagents.index < 1) {
      NOTIFY(FATAL, "user-agent file contains no usable entries: %s", my.uafile);
    }
    if (my.uamode_set == FALSE) {
      my.uamode = UA_ROUND_ROBIN;
    }
  }

  pthread_attr_init(&scope_attr);
  pthread_attr_setscope(&scope_attr, PTHREAD_SCOPE_SYSTEM);
#if defined(_AIX)
  /**
   * AIX, for whatever reason, defies the pthreads standard and  
   * creates threads detached by default. (see pthread.h on AIX) 
   */
  pthread_attr_setdetachstate(&scope_attr, PTHREAD_CREATE_JOINABLE);
#endif

#ifdef HAVE_SSL
  SSL_thread_setup();
#endif

  i = 0;
  if (my.url != NULL) {
    URL tmp = new_url(my.url);
    if (tmp == NULL) {
      NOTIFY(FATAL, "malformed URL: %s", my.url);
    }
    url_set_ID(tmp, 0);
    if (my.get && url_get_method(tmp) != POST && url_get_method(tmp) != PUT) {
      url_set_method(tmp, my.method); 
    }
    array_npush(urls, tmp, URLSIZE); // from cmd line
    i = 1;
  } else { 
    for (i = 0; i < my.length; i++) {
      URL tmp = new_url(lines->line[i]);
      if (tmp == NULL) {
        // fprintf(stderr, "new_url failed for index %zu\n", i);
        continue;
      }
      url_set_ID(tmp, i);
      array_npush(urls, tmp, URLSIZE);
    }
  } 
  __append_wp_search_urls(urls, &i);
  urls = __expand_nocache_urls(urls);
  my.length = array_length(urls);
  if (my.length == 0) {
    NOTIFY(FATAL, "no usable URLs were generated or loaded");
  }
  __runtime_preflight(my.length);

  for (i = 0; i < my.cusers; i++) {
    BROWSER B = new_browser(i+1, file);

    if (my.reps > 0 ) {
      browser_set_urls(B, urls);
    } else {
      /**
       * Scenario: -r once/--reps=once 
       */
      int n_urls = array_length(urls);
      int per_user = n_urls / my.cusers;
      int remainder = n_urls % my.cusers;
      int begin_url = i * per_user + ((i < remainder) ? i : remainder);
      int end_url = (i + 1) * per_user + ((i < remainder) ? (i + 1) : remainder);
      ARRAY url_slice = new_array();
      for (j = begin_url; j < end_url && j < n_urls; j++) {
        URL u = array_get(urls, j);
        if (u != NULL && url_get_hostname(u) != NULL && strlen(url_get_hostname(u)) > 1) {
          array_npush(url_slice, u, URLSIZE);
        }
      }
      browser_set_urls(B, url_slice);
    }
    array_npush(browsers, B, BROWSERSIZE);
  }

  if ((crew = new_crew(my.cusers, my.cusers, FALSE)) == NULL) {
    NOTIFY(FATAL, "unable to allocate memory for %d simulated browser", my.cusers);  
  } 

  /**
   * pthread_create retruns an errno (not necessarily a negative) on failure!
   * should keep it != not <.
   */
  if ((result = pthread_create(&cease, NULL, sig_handler, (void*)crew)) != 0) {
    NOTIFY(FATAL, "failed to create handler: %d\n", result);
  }
  if (my.secs > 0) {
    if ((result = pthread_create(&timer, NULL, siege_timer, (void*)&cease)) != 0) {
      NOTIFY(FATAL, "failed to create handler: %d\n", result);
    } 
  }

  /**
   * Display information about the siege to the user
   * and prepare for verbose output if necessary.
   */
  if (!my.get && !my.quiet) {
    fprintf(stderr, "** "); 
    display_version(FALSE);
    fprintf(stderr, "** Preparing %d concurrent users for battle.\n", my.cusers);
    fprintf(stderr, "The server is now under siege...");
    if (my.verbose) { fprintf(stderr, "\n"); }
  } 

  data = new_data();
  data_set_start(data);
  for (i = 0; i < my.cusers && crew_get_shutdown(crew) != TRUE; i++) {
    BROWSER B = (BROWSER)array_get(browsers, i);
    result = crew_add(crew, (void*)start, B);
    if (result == FALSE) { 
      my.verbose = FALSE;
      fprintf(stderr, "Unable to spawn additional threads; you may need to\n");
      fprintf(stderr, "upgrade your libraries or tune your system in order\n"); 
      fprintf(stderr, "to exceed %d users.\n", my.cusers);
      NOTIFY(FATAL, "system resources exhausted"); 
    }
  } 
  crew_join(crew, TRUE, &status);
  data_set_stop(data); 

  if ((result = pthread_kill(cease, SIGTERM)) != 0 && result != ESRCH) {
    NOTIFY(FATAL, "failed to signal handler thread: %d\n", result);
  }
  if ((result = pthread_join(cease, NULL)) != 0 && result != ESRCH) {
    NOTIFY(FATAL, "failed to join handler thread: %d\n", result);
  }

  if (my.secs > 0) {
    if ((result = pthread_cancel(timer)) != 0 && result != ESRCH) {
      NOTIFY(FATAL, "failed to cancel timer thread: %d\n", result);
    }
    if ((result = pthread_join(timer, NULL)) != 0 && result != ESRCH) {
      NOTIFY(FATAL, "failed to join timer thread: %d\n", result);
    }
  }

#ifdef HAVE_SSL
  SSL_thread_cleanup();
#endif

  for (i = 0; i < ((crew_get_total(crew) > my.cusers || 
                    crew_get_total(crew) == 0) ? my.cusers : crew_get_total(crew)); i++) {
    BROWSER B = (BROWSER)array_get(browsers, i);
    data_increment_count  (data, browser_get_hits(B));
    data_increment_bytes  (data, browser_get_bytes(B));
    data_increment_total  (data, browser_get_time(B));
    data_increment_code   (data, browser_get_code(B));
    data_increment_okay   (data, browser_get_okay(B));
    data_increment_fail   (data, browser_get_fail(B));
    data_set_highest      (data, browser_get_himark(B));
    data_set_lowest       (data, browser_get_lomark(B));
    data_increment_cookies(data, browser_get_cookies(B));
  } crew_destroy(crew);

  __save_cookies(file, data_get_cookies(data));

  pthread_usleep_np(10000);

  if (! my.quiet && ! my.get) {
    if (my.failures > 0 && my.failed >= my.failures) {
      fprintf(stderr, "%s aborted due to excessive socket failure; you\n", program_name);
      fprintf(stderr, "can change the failure threshold in $HOME/.%src\n", program_name);
    }
    fprintf(stderr, "\nTransactions:\t\t%9u    hits\n",        data_get_count(data));
    fprintf(stderr, "Availability:\t\t%12.2f %%\n",          data_get_count(data)==0 ? 0 :
                                                             (double)data_get_count(data) /
                                                             (data_get_count(data)+my.failed)*100
    );
    fprintf(stderr, "Elapsed time:\t\t%12.2f secs\n",        data_get_elapsed(data));
    fprintf(stderr, "Data transferred:\t%12.2f MB\n",        data_get_megabytes(data)); /*%12llu*/
    fprintf(stderr, "Response time:\t\t%12.2f ms\n",       1000.0f * data_get_response_time(data));
    fprintf(stderr, "Transaction rate:\t%12.2f trans/sec\n", data_get_transaction_rate(data));
    fprintf(stderr, "Throughput:\t\t%12.2f MB/sec\n",        data_get_throughput(data));
    fprintf(stderr, "Concurrency:\t\t%12.2f\n",              data_get_concurrency(data));
    fprintf(stderr, "Successful transactions:%9u\n",        data_get_code(data));
    if (my.debug) {
      fprintf(stderr, "HTTP OK received:\t%9u\n",             data_get_okay(data));
    }
    fprintf(stderr, "Failed transactions:\t%9u\n",          my.failed);
    fprintf(stderr, "Longest transaction:\t%12.2f ms\n",        1000.0f * data_get_highest(data));
    fprintf(stderr, "Shortest transaction:\t%12.2f ms\n",       1000.0f * data_get_lowest(data));
    fprintf(stderr, " \n");
  }

  if (my.json_output) {
    fprintf(stderr, "\n");
    printf("{\n");
    printf("\t\"transactions\":\t\t\t%12u,\n", data_get_count(data));

    double availability;
    if (data_get_count(data) == 0) {
      availability = 0;
    } else {
      availability = (double)data_get_count(data) / (data_get_count(data) + my.failed) * 100;
    }

    printf("\t\"availability\":\t\t\t%12.2f,\n", availability);
    printf("\t\"elapsed_time\":\t\t\t%12.2f,\n", data_get_elapsed(data));
    printf("\t\"data_transferred\":\t\t%12.2f,\n", data_get_megabytes(data)); /*%12llu*/
    printf("\t\"response_time\":\t\t%12.2f,\n", data_get_response_time(data));
    printf("\t\"transaction_rate\":\t\t%12.2f,\n", data_get_transaction_rate(data));
    printf("\t\"throughput\":\t\t\t%12.2f,\n", data_get_throughput(data));
    printf("\t\"concurrency\":\t\t\t%12.2f,\n", data_get_concurrency(data));
    printf("\t\"successful_transactions\":\t%12u,\n", data_get_code(data));

    if (my.debug) {
      printf("\t\"http_ok_received\":\t\t%12u,\n", data_get_okay(data));
    }

    printf("\t\"failed_transactions\":\t\t%12u,\n", my.failed);
    printf("\t\"longest_transaction\":\t\t%12.2f,\n", data_get_highest(data));
    printf("\t\"shortest_transaction\":\t\t%12.2f\n", data_get_lowest(data));
    puts("}");
  }

  if (my.mark)    mark_log_file(my.markstr);
  if (my.logging) {
    log_transaction(data);
    if (my.failures > 0 && my.failed >= my.failures) {
      mark_log_file("siege aborted due to excessive socket failure.");
    }
  }

  /**
   * Let's clean up after ourselves....
   */
  data       = data_destroy(data);
  urls       = array_destroyer(urls, (void*)url_destroy);
  browsers   = array_destroyer(browsers, (void*)browser_destroy);

  if (my.url == NULL) {
    for (i = 0; i < my.length; i++)
      xfree(lines->line[i]);
    xfree(lines->line);
    xfree(lines);
  } else {
    xfree(lines->line);
    xfree(lines);
  }
  for (i = 0; i < my.uagents.index; i++) {
    xfree(my.uagents.line[i]);
  }
  xfree(my.uagents.line);

  exit(EXIT_SUCCESS);  
} /* end of int main **/
