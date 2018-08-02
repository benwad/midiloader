#include "eventlist.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>


void PrintEventList(EventNode* head)
{
	EventNode* current = head;

	while (current) {
		PrintEvent(current->event);
		current = current->next;
	}
}


void PushEvent(EventNode* head, Event* event)
{
	if (head == NULL) {
		head = (EventNode *)malloc(sizeof(EventNode));
		head->event = event;
		head->next = NULL;
	}

	EventNode* current = head;
	while (current->next != NULL) {
		current = current->next;
	}

	/* now we're at the end, add a new variable */
	current->next = (EventNode *)malloc(sizeof(EventNode));
	current->next->event = event;
	current->next->next = NULL;
}


unsigned int GetNumEvents(EventNode* head)
{
	EventNode* current = head;
	unsigned int count = 0;

	while (current) {
		count++;
		current = current->next;
	}

	return count;
}
