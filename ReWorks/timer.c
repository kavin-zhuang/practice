#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>

#define PTS_PASS 1
#define PTS_FAIL 0

#define SIGTOTEST SIGALRM
#define TIMERSEC 2
#define SLEEPDELTA 3
#define ACCEPTABLEDELTA 1
 
static void handler_1_1(int signo)
{
  printf("Caught signal\n");
}

int example_timer_create()
{
  struct sigevent ev;
  struct sigaction act;
  timer_t tid;
  struct itimerspec its;
  struct timespec ts, tsleft;

  ev.sigev_notify = SIGEV_SIGNAL;
  ev.sigev_signo = SIGTOTEST;

  act.sa_handler = handler_1_1;
  act.sa_flags = 0;

  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = 0;
  its.it_value.tv_sec = TIMERSEC;
  its.it_value.tv_nsec = 0;

  ts.tv_sec = TIMERSEC+SLEEPDELTA;
  ts.tv_nsec = 0;

  if (sigemptyset(&act.sa_mask) == -1)
  {
    perror("example_timer_create() failed: Error calling sigemptyset\n");
    return PTS_FAIL;
  }
  
  if (sigaction(SIGTOTEST, &act, 0) == -1)
  {
    perror("example_timer_create() failed: Error calling sigaction\n");
    return PTS_FAIL;
  }

  if (timer_create(CLOCK_REALTIME, &ev, &tid) != 0) 
  {
    perror("example_timer_create() failed: timer_create() did not return success\n");
    return PTS_FAIL;
  }

  if (timer_settime(tid, 0, &its, NULL) != 0)
  {
    perror("example_timer_create() failed: timer_settime() did not return success\n");
    timer_delete(tid);
    return PTS_FAIL;
  }

  if (nanosleep(&ts, &tsleft) != -1) 
  {
    perror("example_timer_create() failed: nanosleep() not interrupted\n");
    timer_delete(tid);
    return PTS_FAIL;
  }

  if (abs(tsleft.tv_sec-SLEEPDELTA) <= ACCEPTABLEDELTA)
  {
    timer_delete(tid);
    printf("example_timer_create() success!\n");
    return PTS_PASS;
  } 
  else
  {
    printf("example_timer_create() failed: Timer did not last for correct amount of time\n");
    printf("timer: %d != correct %d\n",
                    (int) ts.tv_sec- (int) tsleft.tv_sec,
                    TIMERSEC);
    
    timer_delete(tid);
    return PTS_FAIL;
  }
}