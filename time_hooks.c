#include "common.h"
#include "config.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/time.h>
#include "hooks.h"

time_t (*enhancer_real_time)(time_t *RetVal)=NULL;
int (*enhancer_real_gettimeofday)(struct timeval *restrict tv, void *restrict tz)=NULL;
int (*enhancer_real_settimeofday)(const struct timeval *tv, const struct timezone *tz)=NULL;
int (*enhancer_real_setitimer)(__itimer_which_t Type, const struct itimerval *new, struct itimerval *curr)=NULL;


time_t ChangeTime(time_t tval, const char *TimeMod)
{
    time_t mod=0;
    const char *ptr;

    if (TimeMod)
    {
        ptr=TimeMod;
        while (strvalid(ptr))
        {
            mod=strtol(ptr, (char **) &ptr, 10);
            if (mod==0) break; //something went wrong
            switch (*ptr)
            {
            case 'm':
                mod *= 60;
                break;
            case 'h':
                mod *= 3600;
                break;
            case 'd':
                mod *= 3600 * 24;
                break;
            case 'w':
                mod *= 3600 * 24 * 7;
                break;
            case 'y':
                mod *= 3600 * 24 * 365;
                break;
            case 'Y':
                mod *= 3600 * 24 * 365;
                break;
            }
            tval+=mod;
            ptr++;
        }
    }

    return(tval);
}


time_t time(time_t *RetVal)
{
    int Flags;
    char *TimeMod=NULL;
    time_t tval;

    if (! enhancer_real_time) enhancer_get_real_functions();

    Flags=enhancer_checkconfig_with_redirect(FUNC_TIME, "time", "", "", 0, 0, &TimeMod);
    if (Flags & FLAG_DENY)
    {
        destroy(TimeMod);
        return(-1);
    }

    tval=enhancer_real_time(NULL);
    tval=ChangeTime(tval, TimeMod);

    if (RetVal) *RetVal=tval;

    destroy(TimeMod);
    return(tval);
}

time_t enhancer_gettime()
{
    struct timeval tv;

    enhancer_real_gettimeofday(&tv, NULL);
    return(tv.tv_sec);
}

#ifndef GETTIMEOFDAY_NONE

#ifdef GETTIMEOFDAY_RTZRVOID
int gettimeofday(struct timeval *restrict tv, void *restrict tz)
#elif defined GETTIMEOFDAY_TZVOID
int gettimeofday(struct timeval *tv, void *tz)
#elif defined GETTIMEOFDAY_VOIDVOID
int gettimeofday(void *tv, void *tz)
#else
int gettimeofday(struct timeval *restrict tv, struct timezone *restrict tz)
#endif
{
    int Flags;
    char *TimeMod=NULL;
    int result;

    if (! enhancer_real_gettimeofday) enhancer_get_real_functions();

    Flags=enhancer_checkconfig_with_redirect(FUNC_TIME, "time", "", "", 0, 0, &TimeMod);
    if (Flags & FLAG_DENY)
    {
        destroy(TimeMod);
        return(-1);
    }

    result=enhancer_real_gettimeofday(tv, tz);
    tv->tv_sec=ChangeTime(tv->tv_sec, TimeMod);

    destroy(TimeMod);
    return(result);
}
#endif

int settimeofday(const struct timeval *itv, const struct timezone *tz)
{
    int Flags;
    char *TimeMod=NULL;
    int result;
    struct timeval tv;

    tv.tv_sec=itv->tv_sec;
    tv.tv_usec=itv->tv_usec;
    if (! enhancer_real_settimeofday) enhancer_get_real_functions();

    Flags=enhancer_checkconfig_with_redirect(FUNC_TIME, "time", "", "", 0, 0, &TimeMod);
    if (Flags & FLAG_DENY)
    {
        destroy(TimeMod);
        return(-1);
    }

    tv.tv_sec=ChangeTime(tv.tv_sec, TimeMod);
    result=enhancer_real_settimeofday(&tv, tz);

    destroy(TimeMod);
    return(result);
}




int setitimer(__itimer_which_t Type, const struct itimerval * new, struct itimerval * curr)
{
    struct itimerval tval;

    tval.it_interval.tv_sec=0;
    tval.it_interval.tv_usec=40000;
    tval.it_value.tv_sec=0;
    tval.it_value.tv_usec=40000;

    if (! enhancer_real_setitimer) enhancer_get_real_functions();
    return(enhancer_real_setitimer(Type, &tval, curr));
}



void enhancer_time_hooks()
{
    if (! enhancer_real_time) enhancer_real_time = dlsym(RTLD_NEXT, "time");
    if (! enhancer_real_setitimer) enhancer_real_setitimer = dlsym(RTLD_NEXT, "setitimer");
    if (! enhancer_real_settimeofday) enhancer_real_settimeofday = dlsym(RTLD_NEXT, "settimeofday");
#ifndef GETTIMEOFDAY_NONE
    if (! enhancer_real_gettimeofday) enhancer_real_gettimeofday = dlsym(RTLD_NEXT, "gettimeofday");
#endif
}
