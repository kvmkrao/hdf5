/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 * Created: H5timer.c
 *          Aug 21 2006
 *          Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose: Internal, platform-independent 'timer' support routines.
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/


/***********/
/* Headers */
/***********/

#include <time.h>

/* Generic functions, including H5_timer_t */
#include "H5private.h"

/* Needed to fill the user-mode and system time fields of H5_timer_t */
#if defined(H5_HAVE_GETRUSAGE) && defined(H5_HAVE_SYS_RESOURCE_H)
#   include <sys/resource.h>
#endif

/* Needed to fill the elapsed time field of H5_timer_t */
#if defined(H5_HAVE_GETTIMEOFDAY) && defined(H5_HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function: H5_timer_begin
 *
 * Purpose: Initialize a timer to time something.
 *
 * Return: void
 *
 * Programmer: Robb Matzke
 *             Thursday, April 16, 1998
 *-------------------------------------------------------------------------
 */
void
H5_timer_begin (H5_timer_OLD_t *timer)
{

#ifdef H5_HAVE_GETRUSAGE
    struct rusage     rusage;
#endif

#ifdef H5_HAVE_GETTIMEOFDAY
    struct timeval    etime;
#endif

    assert (timer);

#ifdef H5_HAVE_GETRUSAGE
    HDgetrusage (RUSAGE_SELF, &rusage);
    timer->utime = (double)rusage.ru_utime.tv_sec +
                   ((double)rusage.ru_utime.tv_usec / 1e6);
    timer->stime = (double)rusage.ru_stime.tv_sec +
                   ((double)rusage.ru_stime.tv_usec / 1e6);
#else
    timer->utime = 0.0;
    timer->stime = 0.0;
#endif

#ifdef H5_HAVE_GETTIMEOFDAY
    HDgettimeofday (&etime, NULL);
    timer->etime = (double)etime.tv_sec + ((double)etime.tv_usec / 1e6);
#else
    timer->etime = 0.0;
#endif

} /* end H5_timer_begin() */


/*-------------------------------------------------------------------------
 * Function:	H5_timer_end
 *
 * Purpose:	This function should be called at the end of a timed region.
 *		The SUM is an optional pointer which will accumulate times.
 *		TMS is the same struct that was passed to H5_timer_start().
 *		On return, TMS will contain total times for the timed region.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *              Thursday, April 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5_timer_end (H5_timer_OLD_t *sum/*in,out*/, H5_timer_OLD_t *timer/*in,out*/)
{
    H5_timer_OLD_t      now;

    assert (timer);
    H5_timer_begin (&now);

    timer->utime = MAX(0.0, now.utime - timer->utime);
    timer->stime = MAX(0.0, now.stime - timer->stime);
    timer->etime = MAX(0.0, now.etime - timer->etime);

    if (sum) {
        sum->utime += timer->utime;
        sum->stime += timer->stime;
        sum->etime += timer->etime;
    }
} /* end H5_timer_end() */


/*-------------------------------------------------------------------------
 * Function: H5_bandwidth
 *
 * Purpose: Prints the bandwidth (bytes per second) in a field 10
 *          characters wide widh four digits of precision like this:
 *
 *  NaN             If <=0 seconds
 *  1234.  TB/s
 *  123.4  TB/s
 *  12.34  GB/s
 *  1.234  MB/s
 *  4.000  kB/s
 *  1.000  B/s
 *  0.000  B/s      If NBYTES==0
 *  1.2345e-10      For bandwidth less than 1
 *  6.7893e+94      For exceptionally large values
 *  6.678e+106      For really big values
 *
 * Return: void
 *
 * Programmer: Robb Matzke
 *             Wednesday, August  5, 1998
 *-------------------------------------------------------------------------
 */
void
H5_bandwidth(char *buf/*out*/, double nbytes, double nseconds)
{
    double      bw;

    if(nseconds <= 0.0) {
        HDstrcpy(buf, "       NaN");
    } else {
        bw = nbytes/nseconds;
        if(fabs(bw) < 0.0000000001) {
            /* That is == 0.0, but direct comparison between floats is bad */
            HDstrcpy(buf, "0.000  B/s");
        } else if(bw < 1.0) {
            sprintf(buf, "%10.4e", bw);
        } else if(bw < 1024.0) {
            sprintf(buf, "%05.4f", bw);
            HDstrcpy(buf+5, "  B/s");
        } else if(bw < (1024.0 * 1024.0)) {
            sprintf(buf, "%05.4f", bw / 1024.0);
            HDstrcpy(buf+5, " kB/s");
        } else if(bw < (1024.0 * 1024.0 * 1024.0)) {
            sprintf(buf, "%05.4f", bw / (1024.0 * 1024.0));
            HDstrcpy(buf+5, " MB/s");
        } else if(bw < (1024.0 * 1024.0 * 1024.0 * 1024.0)) {
            sprintf(buf, "%05.4f", bw / (1024.0 * 1024.0 * 1024.0));
            HDstrcpy(buf+5, " GB/s");
        } else if(bw < (1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0)) {
            sprintf(buf, "%05.4f", bw / (1024.0 * 1024.0 * 1024.0 * 1024.0));
            HDstrcpy(buf+5, " TB/s");
        } else {
            sprintf(buf, "%10.4e", bw);
            if(HDstrlen(buf) > 10) {
                sprintf(buf, "%10.3e", bw);
            }
        }
    }
} /* end H5_bandwidth() */


void
H5_timer_start(H5_timer_t *timer/*in,out*/)
{
    /* System and user time (also elapsed for Win32) */
#if defined(_WIN32)
    FILETIME KernelTime;
    FILETIME UserTime;
    FILETIME CreationTime;
    FILETIME ExitTime;
    BOOL werr;
#elif defined(H5_HAVE_GETRUSAGE)
    struct rusage res;
    int err;
#endif

    /* elapsed time */
#if defined(_WIN32) || defined(H5_HAVE_MACH_TIME_H)
    /* Nothing */
#elif defined(H5_HAVE_CLOCK_GETTIME)
    struct timespec ts;
    int err;
#elif defined(H5_HAVE_GETTIMEOFDAY)
    struct timeval tv;
    int err;
#endif

    assert (timer);

    /*************************
     * System and user times *
     *************************/

#if defined(_WIN32)

    /* NOTE: This is just a pseudo handle and does not need to be closed. */
    timer->process_handle = GetCurrentProcess();

    werr = GetProcessTimes(timer->process_handle,
        &CreationTime,
        &ExitTime,
        &KernelTime,
        &UserTime);

    timer->kernel_start.HighPart = KernelTime.dwHighDateTime;
    timer->kernel_start.LowPart = KernelTime.dwLowDateTime;

    timer->user_start.HighPart = UserTime.dwHighDateTime;
    timer->user_start.LowPart = UserTime.dwLowDateTime;

#elif defined(H5_HAVE_GETRUSAGE)

    err = getrusage(RUSAGE_SELF, &res);
    timer->system_start = (double)((res.ru_stime.tv_sec * 1.0E9) + (res.ru_stime.tv_usec * 1.0E3));
    timer->user_start = (double)((res.ru_utime.tv_sec * 1.0E9) + (res.ru_utime.tv_usec * 1.0E3));

#else

    /* No suitable way to get system/user times */
    timer->system_start = -1.0;
    timer->user_start = -1.0;

#endif


    /****************
     * Elapsed time *
     ****************/

#if defined(_WIN32)

    werr = QueryPerformanceCounter(&(timer->counts_start));
    werr = QueryPerformanceFrequency(&(timer->counts_freq));

#elif defined(H5_HAVE_MACH_MACH_TIME_H)

    timer->elapsed_start = mach_absolute_time();

#elif defined(H5_HAVE_CLOCK_GETTIME)

    err = clock_gettime(CLOCK_MONOTONIC, &ts);
    timer->elapsed_start = (double)((ts.tv_sec * 1.0E9) + ts.tv_nsec);

#elif defined(H5_HAVE_GETTIMEOFDAY)

    err = gettimeofday(&tv, NULL);
    timer->elapsed_start = (double)((tv.tv_sec * 1.0E9) + (tv.tv_usec * 1.0E3));

#else

    timer->elapsed_start = (double)time(NULL);

#endif

    return;
}



H5_timevals_t
H5_timer_get_times(H5_timer_t timer)
{
    H5_timevals_t tvs;

    /* System and user time (also elapsed for Win32) */
#if defined(_WIN32)
    LARGE_INTEGER CurrCounts;
    LARGE_INTEGER delta_e;

    ULARGE_INTEGER delta_su;
    ULARGE_INTEGER temp_su;

    FILETIME CreationTime;
    FILETIME ExitTime;
    FILETIME KernelTime;
    FILETIME UserTime;

    BOOL werr;
#elif defined(H5_HAVE_GETRUSAGE)
    struct rusage res;
    int err;
#endif

    /* Elapsed time */
#if defined(_WIN32)
    /* Nothing */
#elif defined(H5_HAVE_MACH_MACH_TIME_H)
    static double conversion = 0.0;
    mach_timebase_info_data_t info;
    kern_return_t kerr;
    uint64_t now;
#elif defined(H5_HAVE_CLOCK_GETTIME)
    struct timespec ts;
    int err;
#elif defined(H5_HAVE_GETTIMEOFDAY)
    struct timeval tv;
    int err;
#endif


    tvs.elapsed_ns = 0.0;
    tvs.system_ns  = 0.0;
    tvs.user_ns    = 0.0;

    /*************************
     * System and user times *
     *************************/

#if defined(_WIN32)

    werr = GetProcessTimes(timer.process_handle,
        &CreationTime,
        &ExitTime,
        &KernelTime,
        &UserTime);

    temp_su.HighPart = KernelTime.dwHighDateTime;
    temp_su.LowPart = KernelTime.dwLowDateTime;
    delta_su.QuadPart = temp_su.QuadPart - timer.kernel_start.QuadPart;
    tvs.system_ns = (double)(delta_su.QuadPart * 1.0E2);

    temp_su.HighPart = UserTime.dwHighDateTime;
    temp_su.LowPart = UserTime.dwLowDateTime;
    delta_su.QuadPart = temp_su.QuadPart - timer.user_start.QuadPart;
    tvs.user_ns = (double)(delta_su.QuadPart * 1.0E2);

#elif defined(H5_HAVE_GETRUSAGE)

    err = getrusage(RUSAGE_SELF, &res);

    tvs.system_ns = (double)((res.ru_stime.tv_sec * 1.0E9) + (res.ru_stime.tv_usec * 1.0E3) - timer.system_start);
    tvs.user_ns = (double)((res.ru_utime.tv_sec * 1.0E9) + (res.ru_utime.tv_usec * 1.0E3) - timer.user_start);

#else

    tvs.system_ns  = -1.0;
    tvs.user_ns    = -1.0;

#endif

    /****************
     * Elapsed time *
     ****************/

#if defined(_WIN32)

    werr = QueryPerformanceCounter(&CurrCounts);

    delta_e.QuadPart = CurrCounts.QuadPart - timer.counts_start.QuadPart;
    tvs.elapsed_ns = (double)(delta_e.QuadPart * 1.0E9) / (double)timer.counts_freq.QuadPart;

#elif defined(H5_HAVE_MACH_MACH_TIME_H)

    if (0.0 == conversion) {
        kerr = mach_timebase_info(&info);
        if (0 == kerr)
            conversion = (double)info.numer / (double)info.denom;
    }

    now = mach_absolute_time();
    tvs.elapsed_ns = (double)(now - timer.elapsed_start) * conversion;

#elif defined(H5_HAVE_CLOCK_GETTIME)

    err = clock_gettime(CLOCK_MONOTONIC, &ts);
    tvs.elapsed_ns = (double)((ts.tv_sec * 1.0E9) + ts.tv_nsec) - timer.elapsed_start;

#elif defined(H5_HAVE_GETTIMEOFDAY)

    err = gettimeofday(&tv, NULL);
    tvs.elapsed_ns = (double)((tv.tv_sec * 1.0E9) + (tv.tv_usec * 1.0E3) - timer.elapsed_start);

#else

    tvs.elapsed_ns = ((double)time(NULL) - timer.elapsed_start) * 1.0E9;

#endif

    return tvs;
}



#define H5TIMER_TIME_STRING_LEN 256

char *
H5_timer_get_time_string(double ns)
{
    double hours    = 0;
    double minutes  = 0;
    double seconds  = 0;

    char *s;                /* output string */

    /* Initialize */
    s = calloc(H5TIMER_TIME_STRING_LEN, sizeof(char));
    seconds = ns / 1.0E9;
    minutes = seconds / 60.0;
    hours   = seconds / 3600.0;

    /* Do we need a format string? Some people might like a certain 
     * number of milliseconds or s before spilling to the next highest
     * time unit.  Perhaps this could be passed as an integer.
     * (name? round_up_size? ?)
     */
    if(ns < 0.0) {
        sprintf(s, "N/A");
    }else if(ns < 1.0E3) {
        /* t < 1 us, Print time in ns */
        sprintf(s, "%.f ns", ns);
    } else if (ns < 1.0E6) {
        /* t < 1 ms, Print time in us */
        sprintf(s, "%.1f us", ns / 1.0E3);
    } else if (ns < 1.0E9) {
        /* t < 1 s, Print time in ms */
        sprintf(s, "%.1f ms", ns / 1.0E6);
    } else if (minutes < 1.0) {
        /* t < 1 m, Print time in s */
        sprintf(s, "%.2f s", seconds);
    } else if (hours < 0) {
        /* t < 1 h, Print time in m and s */
        sprintf(s, "%.f m %.1f s", minutes, seconds);
    } else {
        /* Print time in h, m and s */
        sprintf(s, "%.f h %.f m %.1f s", hours, minutes, seconds);
    }

    return s;
}
