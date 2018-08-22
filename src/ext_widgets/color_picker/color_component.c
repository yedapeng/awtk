/**
 * File:   color_component.
 * Author: AWTK Develop Team
 * Brief:  color_component
 *
 * Copyright (c) 2018 - 2018  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2018-08-21 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "base/mem.h"
#include "color_picker/color.h"
#include "base/image_manager.h"
#include "base/pixel_pack_unpack.h"
#include "color_picker/color_component.h"

static ret_t color_component_update_h(widget_t* widget);
static ret_t color_component_update_sv(widget_t* widget);
static ret_t color_component_set_type(widget_t* widget, const char* type);

static ret_t color_component_update_pressed(widget_t* widget, pointer_event_t* e) { 
  point_t p = {e->x, e->y};
  color_component_t* color_component = COLOR_COMPONENT(widget);

  widget_to_local(widget, &p);
  color_component->pressed_x = p.x;
  color_component->pressed_y = p.y;
  widget_invalidate_force(widget);

  return RET_OK;
}

static ret_t color_component_on_event(widget_t* widget, event_t* e) {
  uint16_t type = e->type;

  switch (type) {
    case EVT_POINTER_DOWN: {
        pointer_event_t* evt = (pointer_event_t*)e;
        color_component_update_pressed(widget, evt);
        widget_grab(widget->parent, widget);
      }
      break;
    case EVT_POINTER_MOVE: {
      pointer_event_t* evt = (pointer_event_t*)e;
      if(evt->pressed) {
        color_component_update_pressed(widget, evt);
      }
      break;
    }
    case EVT_POINTER_UP: {
      widget_ungrab(widget->parent, widget);
      break;
    }
    default:
      break;
  }

  return RET_OK;
}

static ret_t color_component_on_paint_self(widget_t* widget, canvas_t* c) {
  rect_t src;
  rect_t dst;
  color_component_t* color_component = COLOR_COMPONENT(widget);
  bitmap_t* image = &(color_component->image);

  if(color_component->update == NULL) {
    color_component_set_type(widget, widget->name);
  }

  if(color_component->need_update) {
    color_component->update(widget);
    color_component->need_update = FALSE;
  }
  
  src = rect_init(0, 0, image->w, image->h);
  dst = rect_init(0, 0, widget->w, widget->h);

  canvas_draw_image(c, image, &src, &dst);

  if(color_component->update == color_component_update_sv) {
    canvas_set_stroke_color(c, color_init(0, 0, 0, 0xff));
    canvas_draw_hline(c, 0, color_component->pressed_y, widget->w);
    canvas_draw_vline(c, color_component->pressed_x, 0, widget->h);
  } else {
    canvas_set_stroke_color(c, color_init(0, 0, 0, 0xff));
    canvas_draw_hline(c, 0, color_component->pressed_y, widget->w);
  }

  return RET_OK;
}

static ret_t color_component_destroy(widget_t* widget) {
  color_component_t* color_component = COLOR_COMPONENT(widget);

  bitmap_destroy(&(color_component->image));

  return RET_OK;
}

static const widget_vtable_t s_color_component_vtable = {
    .size = sizeof(color_component_t), 
    .type = WIDGET_TYPE_COLOR_COMPONENT, 
    .create = color_component_create,
    .destroy = color_component_destroy,
    .on_event = color_component_on_event,
    .on_paint_self = color_component_on_paint_self
    };

static ret_t bitmap_destroy_data(bitmap_t* bitmap) {
  void* data = (void*)bitmap->data;

  TKMEM_FREE(data);

  return RET_OK;
}

static ret_t color_component_init_image(bitmap_t* image, const char* name, int32_t w, int32_t h) {
  int32_t size = w * h * 4;

  image->w = w;
  image->h = h;
  image->flags = 0; 
  image->name = name;
#ifdef WITH_BITMAP_BGRA
  image->format = BITMAP_FMT_BGRA;
#else
  image->format = BITMAP_FMT_RGBA;
#endif/*WITH_BITMAP_BGRA*/
  image->data = (uint8_t*)TKMEM_ALLOC(size);
  return_value_if_fail(image->data != NULL, RET_OOM);
  image->destroy = bitmap_destroy_data;

  memset((void*)(image->data), 0xff, size);

  return RET_OK;
}

static ret_t color_component_update_sv(widget_t* widget) {
  color_component_t* color_component = COLOR_COMPONENT(widget);
  rgba_t rgba = color_component->c.rgba;
  bitmap_t* image = &(color_component->image);

  float H = 0;
  float S = 0;
  float V = 0;
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  int32_t x = 0;
  int32_t y = 0;
  int32_t w =  image->w;
  int32_t h =  image->h;
  uint32_t* dst = (uint32_t*)(image->data);

  convertRGBtoHSV(rgba.r, rgba.g, rgba.b, &H, &S, &V);
  for(y = 0; y < h; y++) {
    for(x = 0; x < w; x++) {
      V = (float)x/(float)w;
      S = 1 - (float)y/(float)h;
      convertHSVtoRGB(H, S, V, &r, &g, &b);
      *dst++ = rgb_to_image8888(r, g, b);
    }
  }

  return RET_OK;
}

static ret_t color_component_update_h(widget_t* widget) {
  color_component_t* color_component = COLOR_COMPONENT(widget);
  rgba_t rgba = color_component->c.rgba;
  bitmap_t* image = &(color_component->image);

  float H = 0;
  float S = 1;
  float V = 1;
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  int32_t x = 0;
  int32_t y = 0;
  uint32_t v = 0;
  int32_t w =  image->w;
  int32_t h =  image->h;
  uint32_t* dst = (uint32_t*)(image->data);

  for(y = 0; y < h; y++) {
    H = (1-(float)y/(float)h) * 360;
    convertHSVtoRGB(H, S, V, &r, &g, &b);
    v = rgb_to_image8888(r, g, b);
    for(x = 0; x < w; x++) {
      *dst++ = v;
    }
  }

  return RET_OK;
}

widget_t* color_component_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  color_component_t* color_component = TKMEM_ZALLOC(color_component_t);
  widget_t* widget = WIDGET(color_component);
  return_value_if_fail(color_component != NULL, NULL);

  widget_init(widget, parent, &s_color_component_vtable, x, y, w, h);
  color_component_init_image(&(color_component->image), "", w, h);
  color_component->c = color_init(0xff, 0xff, 0xff, 0xff);
  color_component->need_update = TRUE;

  return widget;
}

ret_t color_component_set_color(widget_t* widget, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  color_component_t* color_component = COLOR_COMPONENT(widget);
  return_value_if_fail(widget != NULL, RET_BAD_PARAMS);

  color_component->c = color_init(r, g, b, a);
  color_component->need_update = TRUE;

  return RET_OK;
}

widget_t* color_component_cast(widget_t* widget) {
  return_value_if_fail(widget != NULL && widget->vt == &s_color_component_vtable, NULL);

  return widget;
}

static ret_t color_component_set_type(widget_t* widget, const char* type) {
  color_component_t* color_component = COLOR_COMPONENT(widget);
  return_value_if_fail(widget != NULL && type != NULL, RET_BAD_PARAMS);

  color_component->image.name = type; 

  if(tk_str_eq(type, "sv")) {
    color_component->update = color_component_update_sv;
  } else if(tk_str_eq(type, "h")) {
    color_component->update = color_component_update_h;
  } else {
    log_debug("not supported color type\n");
  }

  return RET_OK;
}