﻿/**
 * File:   istream_buffered.c
 * Author: AWTK Develop Team
 * Brief:  buffered input stream
 *
 * Copyright (c) 2019 - 2019  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2019-10-08 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "tkc/mem.h"
#include "streams/istream_buffered.h"

static int32_t tk_istream_buffered_read(tk_istream_t* stream, uint8_t* buff, uint32_t max_size) {
  tk_istream_buffered_t* istream_buffered = TK_ISTREAM_BUFFERED(stream);
  tk_istream_t* real_istream = istream_buffered->real_istream;
  ring_buffer_t* rb = istream_buffered->rb;

  if (ring_buffer_is_empty(rb)) {
    int32_t ret = tk_istream_read(real_istream, rb->data, rb->capacity);
    if (ret <= 0) {
      return ret;
    }

    ring_buffer_set_write_cursor(rb, ret);
  }

  return ring_buffer_read(rb, buff, max_size);
}

static ret_t tk_istream_buffered_set_prop(object_t* obj, const char* name, const value_t* v) {
  return RET_NOT_FOUND;
}

static ret_t tk_istream_buffered_get_prop(object_t* obj, const char* name, value_t* v) {
  tk_istream_buffered_t* istream_buffered = TK_ISTREAM_BUFFERED(obj);
  tk_istream_t* real_istream = istream_buffered->real_istream;

  return object_get_prop(OBJECT(real_istream), name, v);
}

static ret_t tk_istream_buffered_on_destroy(object_t* obj) {
  tk_istream_buffered_t* istream_buffered = TK_ISTREAM_BUFFERED(obj);

  ring_buffer_destroy(istream_buffered->rb);
  object_unref(OBJECT(istream_buffered->real_istream));

  return RET_OK;
}

static const object_vtable_t s_tk_istream_buffered_vtable = {
    .type = "tk_istream_buffered",
    .desc = "tk_istream_buffered",
    .size = sizeof(tk_istream_buffered_t),
    .on_destroy = tk_istream_buffered_on_destroy,
    .get_prop = tk_istream_buffered_get_prop,
    .set_prop = tk_istream_buffered_set_prop};

tk_istream_t* tk_istream_buffered_create(tk_istream_t* real_istream, uint32_t buffer_size) {
  object_t* obj = NULL;
  ring_buffer_t* rb = NULL;
  tk_istream_buffered_t* istream_buffered = NULL;
  return_value_if_fail(real_istream != NULL, NULL);

  rb = ring_buffer_create(buffer_size, buffer_size);
  return_value_if_fail(rb != NULL, NULL);

  obj = object_create(&s_tk_istream_buffered_vtable);
  istream_buffered = TK_ISTREAM_BUFFERED(obj);
  return_value_if_fail(istream_buffered != NULL, NULL);

  istream_buffered->rb = rb;
  istream_buffered->real_istream = real_istream;
  TK_ISTREAM(obj)->read = tk_istream_buffered_read;

  return TK_ISTREAM(obj);
}
