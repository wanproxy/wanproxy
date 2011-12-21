#ifndef	EVENT_EVENT_CONDITION_H
#define	EVENT_EVENT_CONDITION_H

#include <event/event.h>
#include <event/typed_condition.h>

typedef	class TypedCondition<Event> EventCondition;
typedef	class TypedConditionVariable<Event> EventConditionVariable;

#endif /* !EVENT_EVENT_CONDITION_H */
