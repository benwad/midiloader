#ifndef __EVENTLIST_H__
#define __EVENTLIST_H__

#include "events.h"

void PrintEventList(EventNode* head);
void PushEvent(EventNode* head, Event* event);
unsigned int GetNumEvents(EventNode* head);

#endif
