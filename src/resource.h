#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdio.h>
#include <stdlib.h>

void PAL_ConsolidateExtractedResources(void);
int PAL_LoadConsolidatedResources(void);
void PAL_FreeResourceIndex(void);

#endif // RESOURCE_H