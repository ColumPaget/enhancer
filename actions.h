#ifndef ENHANCER_ACTIONS_H
#define ENHANCER_ACTIONS_H

#include "common.h"

const char *enhancer_ConvertPathToChroot(const char *Path);
int enhancer_actions(TEnhancerConfig *Conf, const char *FuncName, const char *Str1, const char *Str2, char **Redirect);

#endif
