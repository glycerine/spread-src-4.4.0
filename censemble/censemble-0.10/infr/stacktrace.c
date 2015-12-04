#if !defined(MINIMIZE_CODE) && defined(__GNUC__)
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "infr/stacktrace.h"

static FILE *child_read ;
static FILE *child_write ;
static int initing ;

static
void addr2line_init(void) {
    int ret ;
    char exec[1024] ;
    int fdr[2] ;
    int fdw[2] ;
    if (initing) {
	abort() ;
    }
    initing = 1 ;
#ifdef __linux__
    sprintf(exec, "/proc/%d/exe", getpid()) ;
#else
    sprintf(exec, "/proc/%d/file", getpid()) ;
#endif
    ret = pipe(fdr) ;
    ret = pipe(fdw) ;

    ret = fork() ;
    if (ret == -1) {
	perror("fork") ;
	exit(1) ;
    }

    if (ret > 0) {
	child_read = fdopen(fdr[0], "r") ;
	if (!child_read) {
	    perror("fdopen(r)") ;
	    exit(1) ;
	}
	child_write = fdopen(fdw[1], "w") ;
	if (!child_write) {
	    perror("fdopen(w)") ;
	    exit(1) ;
	}
	initing = 0 ;
	return ;
    }

    dup2(fdr[1], 1) ;
    dup2(fdw[0], 0) ;

    ret = execl("/usr/bin/addr2line", "addr2line", "--functions", "-e", exec, NULL) ;
    if (ret == -1) {
	perror("execl") ;
	exit(1) ;
    }
}

static
void addr2line_dump(void *pc_ptr, char *info) {
    unsigned long pc = (unsigned long) pc_ptr ;
    char file[1024] ;
    char func[1024] ;

    if (!child_read) {
	addr2line_init() ;
    }
    
    fprintf(child_write, "0x%08lx\n", pc) ;
    fflush(child_write) ;
    fgets(func, sizeof(func), child_read) ;
    fgets(file, sizeof(file), child_read) ;
    if (strchr(func, '\n')) {
	*strchr(func, '\n') = 0 ;
    }
    if (strchr(file, '\n')) {
	*strchr(file, '\n') = 0 ;
    }

    sprintf(info, "%s %s", file, func) ;
}

void stacktrace_dump(stacktrace_t *s) {
    int i ;
    char info[1024] ;
    for (i=0;i<STACKTRACE_NITEMS;i++) {
	if (!s->pc[i]) {
	    continue ;
	}
	addr2line_dump(s->pc[i], info) ;
	fprintf(stderr, "  frame=%02d pc=%08lx %s\n",
		i, (unsigned long) s->pc[i], info) ;
    }
}
#endif
