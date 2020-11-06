#ifndef ENHANCER_DL_HOOKS_H
#define ENHANCER_DL_HOOKS_H

void *(*enhancer_real_dlopen)(const char *filename, int flag);
void enhancer_dl_hooks();

#endif
