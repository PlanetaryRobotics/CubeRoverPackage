#ifndef __WATCHDOG_EVENT_QUEUE_H__
#define __WATCHDOG_EVENT_QUEUE_H__

#include "include/event/event.h"
#include "include/utils/ring_buffer.h"

// The event queue is a global singleton RingBuffer

typedef enum EventQueue__Status {
    EQ__STATUS__SUCCESS = RB__STATUS__SUCCESS, /* Operation suceeded. */
    EQ__STATUS__ERROR_NULL = RB__STATUS__ERROR_NULL, /* A required argument or a member of an argument was NULL */
    EQ__STATUS__ERROR_FULL = RB__STATUS__ERROR_FULL, /* Did not write byte because buffer is full */
    EQ__STATUS__ERROR_EMPTY = RB__STATUS__ERROR_EMPTY, /* Could not read byte because buffer is empty */
    EQ__STATUS__ERROR_NOT_POWER_OF_TWO = RB__STATUS__ERROR_NOT_POWER_OF_TWO, /* Buffer size was not a power of two */
    EQ__STATUS__ERROR_ZERO_SIZE = RB__STATUS__ERROR_ZERO_SIZE, /* Buffer size was zero */
    EQ__STATUS__ERROR_ALL_BUFFERS_USED = RB__STATUS__ERROR_ALL_BUFFERS_USED, /* All statically allocated RingBuffers already in use */

    EQ__STATUS__ERROR_NOT_INITIALIZED = -100,
    EQ__STATUS__ERROR_ALREADY_INITIALIZED = -101,

    EQ__STATUS__ERROR_INTERNAL = RB__STATUS__ERROR_INTERNAL /* An unexpected internal error occurred. */
} EventQueue__Status;

EventQueue__Status EventQueue__initialize(volatile uint8_t* buffer, size_t bufferSize);

// put() will fail to write data if the buffer is full. get() will get the oldest byte
// from the buffer if the buffer is not empty.
//
// Either of these is safe to call in any context (in ISR or not) without needing to
// disable any interrupts.
//
// If put() is called in an interrupt and get() called outside of an interrupt, then
// these two functions can be used safely without disabling any interrupts. put() calls
// full(), which reads both the head and tail, and then may modify head. get() calls
// empty(), which reads both the head and tail, and then may modify tail. In this scenario
// calling put() is safe because it is called within the interrupt, so get() can't modify
// the head or tail while it is executed. Additionally, calling get() is safe because even
// though it *is* possible for head to be modified, this modification will only cause get()
// to not catch that a new element to be added to the buffer, but the next time get() is
// called it will be noticed.
//
// If get() is called in interrupt and put() is called outside an interrupt, then these
// functions are still safe to use. The worst thing that can happen is that during a
// a call to put(), the get() is invoked from the interrupt. In this case, the get()
// call may free a slot in the buffer, but the call to put() won't see this free slot
// and the put() call will fail to write the byte to the buffer. Similarly, the get()
// call may not see a byte in the process of being written if the put() call is interrupted,
// but it is expected that after calling put() the code should make sure that whatever
// code calls get() is invoked (i.e. by enabling the appropriate interrupt).
EventQueue__Status EventQueue__put(Event__Type event);
EventQueue__Status EventQueue__get(Event__Type* event);

// This should only be called with interrupts disabled
EventQueue__Status EventQueue__clear(void);

#endif /* __WATCHDOG_EVENT_QUEUE_H__ */