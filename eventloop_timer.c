/*
 * Copyright (c) 2026 Aleksandr Cherednikov
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "php_eventloop.h"

#define TIMER_HEAP_INITIAL_CAPACITY 16

static inline double timer_expiry(const eventloop_callback *cb)
{
	if (cb->type == EVENTLOOP_CB_DELAY) {
		return cb->delay.expiry;
	}
	return cb->repeat.expiry;
}

static inline void heap_swap(eventloop_timer_heap *heap, uint32_t i, uint32_t j)
{
	eventloop_callback *tmp;

	tmp = heap->data[i];
	heap->data[i] = heap->data[j];
	heap->data[j] = tmp;
	heap->data[i]->heap_index = i;
	heap->data[j]->heap_index = j;
}

static void heap_sift_up(eventloop_timer_heap *heap, uint32_t index)
{
	uint32_t parent;

	while (index > 0) {
		parent = (index - 1) / 2;
		if (timer_expiry(heap->data[index]) < timer_expiry(heap->data[parent])) {
			heap_swap(heap, index, parent);
			index = parent;
		} else {
			break;
		}
	}
}

static void heap_sift_down(eventloop_timer_heap *heap, uint32_t index)
{
	uint32_t smallest;
	uint32_t left;
	uint32_t right;

	while (true) {
		smallest = index;
		left = 2 * index + 1;
		right = 2 * index + 2;

		if (left < heap->size &&
		    timer_expiry(heap->data[left]) < timer_expiry(heap->data[smallest])) {
			smallest = left;
		}
		if (right < heap->size &&
		    timer_expiry(heap->data[right]) < timer_expiry(heap->data[smallest])) {
			smallest = right;
		}

		if (smallest != index) {
			heap_swap(heap, index, smallest);
			index = smallest;
		} else {
			break;
		}
	}
}

void eventloop_timer_heap_init(eventloop_timer_heap *heap)
{
	heap->data = emalloc(sizeof(eventloop_callback *) * TIMER_HEAP_INITIAL_CAPACITY);
	heap->size = 0;
	heap->capacity = TIMER_HEAP_INITIAL_CAPACITY;
}

void eventloop_timer_heap_destroy(eventloop_timer_heap *heap)
{
	if (heap->data) {
		efree(heap->data);
		heap->data = NULL;
	}
	heap->size = 0;
	heap->capacity = 0;
}

void eventloop_timer_heap_push(eventloop_timer_heap *heap, eventloop_callback *cb)
{
	ZEND_ASSERT(heap->data != NULL);

	if (heap->size >= heap->capacity) {
		heap->capacity *= 2;
		heap->data = erealloc(heap->data, sizeof(eventloop_callback *) * heap->capacity);
	}

	cb->heap_index = heap->size;
	heap->data[heap->size] = cb;
	heap->size++;

	heap_sift_up(heap, cb->heap_index);
}

eventloop_callback *eventloop_timer_heap_peek(const eventloop_timer_heap *heap)
{
	if (heap->size == 0) {
		return NULL;
	}
	return heap->data[0];
}

eventloop_callback *eventloop_timer_heap_pop(eventloop_timer_heap *heap)
{
	eventloop_callback *cb;

	if (heap->size == 0) {
		return NULL;
	}

	cb = heap->data[0];
	cb->heap_index = UINT32_MAX;

	heap->size--;
	if (heap->size > 0) {
		heap->data[0] = heap->data[heap->size];
		heap->data[0]->heap_index = 0;
		heap_sift_down(heap, 0);
	}

	return cb;
}

void eventloop_timer_heap_remove(eventloop_timer_heap *heap, eventloop_callback *cb)
{
	uint32_t index;

	if (cb->heap_index == UINT32_MAX || cb->heap_index >= heap->size) {
		return;
	}

	ZEND_ASSERT(heap->data[cb->heap_index] == cb);

	index = cb->heap_index;
	cb->heap_index = UINT32_MAX;

	heap->size--;
	if (index < heap->size) {
		heap->data[index] = heap->data[heap->size];
		heap->data[index]->heap_index = index;

		/* May need to sift up or down */
		if (index > 0 &&
		    timer_expiry(heap->data[index]) < timer_expiry(heap->data[(index - 1) / 2])) {
			heap_sift_up(heap, index);
		} else {
			heap_sift_down(heap, index);
		}
	}
}

void eventloop_timer_heap_update(eventloop_timer_heap *heap, eventloop_callback *cb)
{
	uint32_t index;

	if (cb->heap_index == UINT32_MAX || cb->heap_index >= heap->size) {
		return;
	}

	ZEND_ASSERT(heap->data[cb->heap_index] == cb);

	index = cb->heap_index;
	if (index > 0 &&
	    timer_expiry(heap->data[index]) < timer_expiry(heap->data[(index - 1) / 2])) {
		heap_sift_up(heap, index);
	} else {
		heap_sift_down(heap, index);
	}
}
