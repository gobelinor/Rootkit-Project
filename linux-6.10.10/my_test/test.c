#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/jiffies.h>

#define time_t ktime_t


SYSCALL_DEFINE1(uptime, time_t __user *, t);


SYSCALL_DEFINE1(uptime, time_t __user *, t)
{
    struct timespec64 uptime;
    time_t secs;

    jiffies_to_timespec64(jiffies - INITIAL_JIFFIES, &uptime);
    secs = uptime.tv_sec;

    if (t && put_user(secs, t))
        return (time_t)-1;

    return secs;
}
