#ifndef STACKTRACE_H
#define STACKTRACE_H

#if !defined(MINIMIZE_CODE) && defined(__GNUC__)
#define STACKTRACE_NITEMS 4
#else
#define STACKTRACE_NITEMS 0
#endif
typedef struct {
    void *pc[STACKTRACE_NITEMS] ;
} stacktrace_t ;

static inline
void stacktrace_get(stacktrace_t *s) {
#if STACKTRACE_NITEMS >= 1
    s->pc[0] = __builtin_return_address(0) ;
#endif
#if STACKTRACE_NITEMS >= 2
    if (s->pc[0]) {
	s->pc[1] = __builtin_return_address(1) ;
    } else {
	s->pc[1] = NULL ;
    }
#endif
#if STACKTRACE_NITEMS >= 3
    if (s->pc[1]) {
	s->pc[2] = __builtin_return_address(2) ;
    } else {
	s->pc[2] = NULL ;
    }
#endif
#if STACKTRACE_NITEMS >= 4
    if (s->pc[2]) {
	s->pc[3] = __builtin_return_address(3) ;
    } else {
	s->pc[3] = NULL ;
    }
#endif
#if STACKTRACE_NITEMS >= 5
    if (s->pc[3]) {
	s->pc[4] = NULL ; //__builtin_return_address(4) ;
    } else {
	s->pc[4] = NULL ;
    }
#endif
}

void stacktrace_get(stacktrace_t *) ;
void stacktrace_dump(stacktrace_t *) ;

#define STACKTRACE() \
  do { \
    stacktrace_t stack ; \
    stacktrace_get(&stack) ; \
    stacktrace_dump(&stack) ; \
  } while(0)

#endif /*STACKTRACE_H*/
