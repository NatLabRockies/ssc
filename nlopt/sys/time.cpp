/********************************************************************************************************************************************

Note
-------------
The version of nlopt included in this repository has been modified as follows:
1. The original .c files have been modified to .cpp files to facilitate the use of c++ std library functions for abs, fabs, sqrt, etc.
2. The nlopt specific file modifications can be found at https://github.com/NatLabRockies/ssc/commits/patch/nlopt

The original version of nlopt can be found at https://github.com/stevengj/nlopt

********************************************************************************************************************************************/
#include <sys/times.h>
 
int gettimeofday(struct timeval* t,void*)
{       struct _timeb timebuffer;
        _ftime( &timebuffer );
        t->tv_sec=(long)timebuffer.time;
        t->tv_usec=1000*timebuffer.millitm;
		return 0;
}
 
clock_t times (struct tms *__buffer) {
 
	__buffer->tms_utime = clock();
	__buffer->tms_stime = 0;
	__buffer->tms_cstime = 0;
	__buffer->tms_cutime = 0;
	return __buffer->tms_utime;
}