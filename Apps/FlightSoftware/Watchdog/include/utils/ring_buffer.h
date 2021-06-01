// ring_buffer.h

#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include "include/common.h"

// Forward definition of opaque ring buffer structure, defined in source file
// Other modules using this structure will not and do not need to know its contents
typedef struct RingBuffer RingBuffer;

#define MAX_NUM_RING_BUFFERS 8

typedef enum RingBuffer__Status {
    RB__STATUS__SUCCESS = 0, /* Operation suceeded. */
    RB__STATUS__ERROR_NULL = -1, /* A required argument or a member of an argument was NULL */
    RB__STATUS__ERROR_FULL = -2, /* Did not write byte because buffer is full */
    RB__STATUS__ERROR_EMPTY = -3, /* Could not read byte because buffer is empty */
    RB__STATUS__ERROR_NOT_POWER_OF_TWO = -4, /* Buffer size was not a power of two */
    RB__STATUS__ERROR_ZERO_SIZE = -5, /* Buffer size was zero */
    RB__STATUS__ERROR_ALL_BUFFERS_USED = -6, /* All statically allocated RingBuffers already in use */

    RB__STATUS__ERROR_INTERNAL = -255 /* An unexpected internal error occurred. */
} RingBuffer__Status;

// Note that bufferSize MUST be a power of two
RingBuffer__Status RingBuffer__init(RingBuffer** rb, volatile uint8_t* buffer, size_t bufferSize);

// These should only be called with interrupts disabled if the ring buffer in question is
// touched at all from any ISR
BOOL RingBuffer__full(const RingBuffer* rb);
BOOL RingBuffer__empty(const RingBuffer* rb);
size_t RingBuffer__freeCount(const RingBuffer* rb);
size_t RingBuffer__usedCount(const RingBuffer* rb);
RingBuffer__Status RingBuffer__peekAt(const RingBuffer* rb, size_t index, uint8_t* value);

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
RingBuffer__Status RingBuffer__put(RingBuffer* rb, uint8_t byte);
RingBuffer__Status RingBuffer__get(RingBuffer* rb, uint8_t* byte);

// putOverwrite() will overwrite the oldest byte in the buffer with the newest one if
// the buffer is full. getOverwrite() is the same as get(), but is implemented as a
// separate function to make it clear why interrupts need to be disabled (see below).
//
// If using either of these in an interrupt, that same interrupt needs to be disabled
// before calling the other one. This is because putOverwrite() may modify both head
// *and* tail, so not disabling the interrupt will cause a race condition.
RingBuffer__Status RingBuffer__putOverwrite(RingBuffer* rb, uint8_t byte);
RingBuffer__Status RingBuffer__getOverwrite(RingBuffer* rb, uint8_t* byte);

// This should only be called with interrupts disabled
RingBuffer__Status RingBuffer__clear(RingBuffer* rb);

#endif // #ifndef __RING_BUFFER_H__