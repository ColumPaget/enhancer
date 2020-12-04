#include "actions.h"
#include "config.h"


void enhancer_atexit()
{
int Flags;

Flags=enhancer_checkconfig_default(FUNC_ONEXIT, "onexit", "", "", 0, 0);
if (enhancer_log_fd > -1)
{
posix_fadvise(enhancer_log_fd, 0, 0, POSIX_FADV_DONTNEED);
enhancer_real_close(enhancer_log_fd);
}
}
