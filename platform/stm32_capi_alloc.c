/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_alloc.c
 * @brief  CAPI heap backend. Maps the CAPI allocator to the C library heap.
 *         The build skips this file when CONFIG_FREERTOS is set.
 */

#include "capi_alloc.h"
#include <stdlib.h>

void *capi_malloc_impl(size_t size)
{
	return malloc(size);
}

void capi_free_impl(void *ptr)
{
	free(ptr);
}

void *capi_calloc_impl(size_t num, size_t size)
{
	return calloc(num, size);
}

void *capi_realloc_impl(void *ptr, size_t size)
{
	return realloc(ptr, size);
}
