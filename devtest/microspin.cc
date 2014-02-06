#include <time.h>
#include <stdio.h>

    long long int timeDiff(timespec* end, timespec* start) {
      long long int diff;
      diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
      diff += end->tv_nsec;
      diff -= start->tv_nsec;
      return diff;
    }

    int main(int an, char** ac) {
      long long unsigned m = 1000000;
      long long unsigned gap = 0;
      if (an > 1) {
	sscanf(ac[1], "%lld", &m);
      }
      
      timespec start, now;
      clock_gettime(CLOCK_REALTIME, &start);
      while (gap < m) {
        clock_gettime(CLOCK_REALTIME, &now);
        gap = timeDiff(&now, &start) / 1000LL;
      }
    return 0;
    }

