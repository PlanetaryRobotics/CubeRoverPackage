#include "include/event/event_queue.h"
#include "include/common.h"

//###########################################################
// Private types
//###########################################################

typedef struct EventQueue
{
    BOOL initialized;
    RingBuffer* rb;
} EventQueue;

//###########################################################
// Private globals and constants
//###########################################################

EventQueue theQueue = {
    .initialized = FALSE,
    .rb = NULL
};

//###########################################################
// Public function definitions
//###########################################################

EventQueue__Status EventQueue__initialize(volatile uint8_t* buffer, size_t bufferSize)
{
    if (NULL == buffer) {
        return EQ__STATUS__ERROR_NULL;
    }

    if (theQueue.initialized) {
        return EQ__STATUS__ERROR_ALREADY_INITIALIZED;
    }

    RingBuffer__Status rbStatus = RingBuffer__init(&(theQueue.rb), buffer, bufferSize);

    if (RB__STATUS__SUCCESS != rbStatus) {
        return (EventQueue__Status) rbStatus;
    }

    theQueue.initialized = TRUE;
    return EQ__STATUS__SUCCESS;
}

EventQueue__Status EventQueue__put(Event__Type event)
{
    if (!(theQueue.initialized)) {
        return EQ__STATUS__ERROR_NOT_INITIALIZED;
    }

    return (EventQueue__Status) RingBuffer__put(theQueue.rb, (uint8_t) event);
}

EventQueue__Status EventQueue__get(Event__Type* event)
{
    if (NULL == event) {
        return EQ__STATUS__ERROR_NULL;
    }

    if (!(theQueue.initialized)) {
        return EQ__STATUS__ERROR_NOT_INITIALIZED;
    }

    uint8_t data = 0;
    RingBuffer__Status rbStatus = RingBuffer__get(theQueue.rb, &data);

    if (RB__STATUS__SUCCESS == rbStatus) {
        *event = (Event__Type) data;
    }

    return (EventQueue__Status) rbStatus;
}

EventQueue__Status EventQueue__clear(void)
{
    if (!(theQueue.initialized)) {
        return EQ__STATUS__ERROR_NOT_INITIALIZED;
    }

    return (EventQueue__Status) RingBuffer__clear(theQueue.rb);
}
