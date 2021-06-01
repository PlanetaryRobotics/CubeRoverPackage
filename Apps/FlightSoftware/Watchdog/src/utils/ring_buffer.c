// ring_buffer.c

#include "include/utils/ring_buffer.h"

struct RingBuffer {
    volatile uint8_t* buffer;
    size_t bufferSize;
    volatile size_t head;
    volatile size_t tail;
}

static RingBuffer ALL_RING_BUFFERS[MAX_NUM_RING_BUFFERS];

RingBuffer__Status RingBuffer__init(RingBuffer** rb, volatile uint8_t* buffer, size_t bufferSize)
{
    static size_t rbIndex = 0;

    if (rb == NULL || buffer == NULL) {
        return RB__STATUS__ERROR_NULL;
    }

    if (bufferSize == 0U) {
        return RB__STATUS__ERROR_ZERO_SIZE;
    }

    // This condition is just a way of checking that bufferSize is a power of two
    if (((bufferSize - 1U) & bufferSize) != 0U) {
        return RB__STATUS__ERROR_NOT_POWER_OF_TWO;
    }

    if (rbIndex >= MAX_NUM_RING_BUFFERS) {
        return RB__STATUS__ERROR_ALL_BUFFERS_USED;
    }

    RingBuffer* thisRb = ALL_RING_BUFFERS + rbIndex;
    thisRb->buffer = buffer;
    thisRb->bufferSize = bufferSize;
    thisRb->head = 0U;
    thisRb->tail = 0U;
    *rb = thisRb;

    return RB__STATUS__SUCCESS;
}

BOOL RingBuffer__full(const RingBuffer* rb)
{
    if (rb == NULL) {
        return FALSE;
    }

    return ((rb->head - rb->tail) == rb->bufferSize) ? TRUE : FALSE;
}

BOOL RingBuffer__empty(const RingBuffer* rb)
{
    if (rb == NULL) {
        return FALSE;
    }

    return ((rb->head - rb->tail) == 0U) ? TRUE : FALSE;
}

size_t RingBuffer__freeCount(const RingBuffer* rb)
{
    if (rb == NULL) {
        return 0;
    }

    return (rb->bufferSize - (rb->head - rb->tail));
}

size_t RingBuffer__usedCount(const RingBuffer* rb)
{
    if (rb == NULL) {
        return 0;
    }

    return (rb->head - rb->tail);
}

RingBuffer__Status RingBuffer__peekAt(const RingBuffer* rb, size_t index, uint8_t* value)
{
    if (rb == NULL || rb->buffer == NULL) {
        return RB__STATUS__ERROR_NULL;
    }

    if (index >= (rb->head - rb->tail)) {
        // Do not have an element at the request index
        return RB__STATUS__ERROR_EMPTY;
    }

    // Get the index of the head byte, wrapping if necessary
    // The operation below is equivalent to:
    //    (rb->head + index) % rb->bufferSize
    // and works because rb->bufferSize is guaranteed to be
    // a power of two.
    size_t wrappedIndex = ((rb->head + index) & (rb->bufferSize - 1));

    *value = rb->buffer[wrappedIndex];

    return RB__STATUS__SUCCESS;
}

RingBuffer__Status RingBuffer__put(RingBuffer* rb, uint8_t byte)
{
    if (rb == NULL || rb->buffer == NULL) {
        return RB__STATUS__ERROR_NULL;
    }

    if (RingBuffer__full(rb)) {
        return RB__STATUS__ERROR_FULL;
    }

    // Get the index of the head byte, wrapping if necessary
    // The operation below is equivalent to:
    //    rb->head % rb->bufferSize
    // and works because rb->bufferSize is guaranteed to be
    // a power of two.
    size_t index = (rb->head & (rb->bufferSize - 1));

    rb->buffer[index] = byte;
    rb->head++;

    return RB__STATUS__SUCCESS;
}

RingBuffer__Status RingBuffer__get(RingBuffer* rb, uint8_t* byte)
{
    if (rb == NULL || rb->buffer == NULL) {
        return RB__STATUS__ERROR_NULL;
    }

    if (RingBuffer__empty(rb)) {
        return RB__STATUS__ERROR_EMPTY;
    }

    // Get the index of the head byte, wrapping if necessary
    // The operation below is equivalent to:
    //    rb->tail % rb->bufferSize
    // and works because rb->bufferSize is guaranteed to be
    // a power of two.
    size_t index = (rb->tail & (rb->bufferSize - 1));

    *byte = rb->buffer[index];
    rb->tail++;

    return RB__STATUS__SUCCESS;
}

RingBuffer__Status RingBuffer__putOverwrite(RingBuffer* rb, uint8_t byte)
{
    if (rb == NULL || rb->buffer == NULL) {
        return RB__STATUS__ERROR_NULL;
    }

    int wasFull = RingBuffer__full(rb);

    // Get the index of the head byte, wrapping if necessary
    // The operation below is equivalent to:
    //    rb->head % rb->bufferSize
    // and works because rb->bufferSize is guaranteed to be
    // a power of two.
    size_t index = (rb->head & (rb->bufferSize - 1));

    rb->buffer[index] = byte;
    rb->head++;

    // If the buffer was full before writing the byte, we need to
    // also increment the tail
    if (wasFull) {
        rb->tail++;
    }

    return RB__STATUS__SUCCESS;
}

RingBuffer__Status RingBuffer__getOverwrite(RingBuffer* rb, uint8_t* byte)
{
    return RingBuffer__get(rb, byte);
}

RingBuffer__Status RingBuffer__clear(RingBuffer* rb)
{
    if (rb == NULL) {
        return RB__STATUS__ERROR_NULL;
    }

    rb->head = 0U;
    rb->tail = 0U;
}

