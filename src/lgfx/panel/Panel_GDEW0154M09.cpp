#include "Panel_GDEW0154M09.hpp"
#include "../LGFX_Device.hpp"

namespace lgfx
{
  constexpr std::uint8_t Panel_GDEW0154M09::Bayer[16];

  void Panel_GDEW0154M09::post_init(LGFX_Device* gfx, bool use_reset)
  {
    // init DSRAM
    _range_old.top = 0;
    _range_old.left = 0;
    _range_old.right = panel_width - 1;
    _range_old.bottom = panel_height - 1;
    _range_new.top = 0;
    _range_new.left = 0;
    _range_new.right = panel_width - 1;
    _range_new.bottom = panel_height - 1;
    gfx->startWrite();
    _close_transfer(gfx);
    _exec_transfer(0x13, gfx, &_range_new);
    if (use_reset)
    {
      fillRect(this, gfx, 0, 0, gfx->width(), gfx->height(), 0);
      display(this, gfx);
      gfx->setBaseColor(TFT_WHITE);
    }
    gfx->endWrite();
  }

  void Panel_GDEW0154M09::_update_transferred_rect(LGFX_Device* gfx, std::int32_t &xs, std::int32_t &ys, std::int32_t &xe, std::int32_t &ye)
  {
    auto r = _internal_rotation;
    if (r & 1) { std::swap(xs, ys); std::swap(xe, ye); }
    switch (r) {
    default: break;
    case 1:  case 2:  case 6:  case 7:
      std::swap(xs, xe);
      xs = panel_width - 1 - xs;
      xe = panel_width - 1 - xe;
      break;
    }
    switch (r) {
    default: break;
    case 2: case 3: case 4: case 7:
      std::swap(ys, ye);
      ys = panel_height - 1 - ys;
      ye = panel_height - 1 - ye;
      break;
    }
    std::int32_t x1 = xs & ~7;
    std::int32_t x2 = (xe & ~7) + 7;

    if (_range_old.horizon.intersectsWith(x1, x2) && _range_old.vertical.intersectsWith(ys, ye))
    {
      _close_transfer(gfx);
    }
    _range_new.top = std::min(ys, _range_new.top);
    _range_new.left = std::min(x1, _range_new.left);
    _range_new.right = std::max(x2, _range_new.right);
    _range_new.bottom = std::max(ye, _range_new.bottom);
  }

  void Panel_GDEW0154M09::fillRect(PanelCommon* panel, LGFX_Device* gfx, std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, std::uint32_t rawcolor)
  {
    auto me = reinterpret_cast<Panel_GDEW0154M09*>(panel);
    std::int32_t xs = x, xe = x + w - 1;
    std::int32_t ys = y, ye = y + h - 1;
    me->_update_transferred_rect(gfx, xs, ys, xe, ye);

    swap565_t color;
    color.raw = rawcolor;
    std::uint32_t value = (color.R8() + (color.G8() << 1) + color.B8() + 3) >> 2;

    y = ys;
    do
    {
      x = xs;
      std::uint32_t idx = ((me->panel_width + 7) & ~7) * y + x;
      auto btbl = &me->Bayer[(y & 3) << 2];
      do
      {
        bool flg = 256 <= value + btbl[x & 3];
        if (flg) me->_buf[idx >> 3] |=   0x80 >> (idx & 7);
        else     me->_buf[idx >> 3] &= ~(0x80 >> (idx & 7));
        ++idx;
      } while (++x <= xe);
    } while (++y <= ye);
  }

  void Panel_GDEW0154M09::pushImage(PanelCommon* panel, LGFX_Device* gfx, std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, pixelcopy_t* param)
  {
    auto me = reinterpret_cast<Panel_GDEW0154M09*>(panel);
    std::int32_t xs = x, xe = x + w - 1;
    std::int32_t ys = y, ye = y + h - 1;
    me->_update_transferred_rect(gfx, xs, ys, xe, ye);

    swap565_t readbuf[w];
    auto sx = param->src_x32;
    h += y;
    do
    {
      std::int32_t prev_pos = 0, new_pos = 0;
      do
      {
        new_pos = param->fp_copy(readbuf, prev_pos, w, param);
        if (new_pos != prev_pos)
        {
          do
          {
            auto color = readbuf[prev_pos];
            me->_draw_pixel(x + prev_pos, y, (color.R8() + (color.G8() << 1) + color.B8() + 3) >> 2);
          } while (new_pos != ++prev_pos);
        }
      } while (w != new_pos && w != (prev_pos = param->fp_skip(new_pos, w, param)));
      param->src_x32 = sx;
      param->src_y++;
    } while (++y < h);
  }

  void Panel_GDEW0154M09::pushBlock(PanelCommon* panel, LGFX_Device* gfx, std::int32_t length, std::uint32_t rawcolor)
  {
    auto me = reinterpret_cast<Panel_GDEW0154M09*>(panel);
    {
      std::int32_t xs = me->_xs;
      std::int32_t xe = me->_xe;
      std::int32_t ys = me->_ys;
      std::int32_t ye = me->_ye;
      me->_update_transferred_rect(gfx, xs, ys, xe, ye);
    }
    std::int32_t xs = me->_xs;
    std::int32_t ys = me->_ys;
    std::int32_t xe = me->_xe;
    std::int32_t ye = me->_ye;
    std::int32_t xpos = me->_xpos;
    std::int32_t ypos = me->_ypos;

    swap565_t color;
    color.raw = rawcolor;
    std::uint32_t value = (color.R8() + (color.G8() << 1) + color.B8() + 3) >> 2;
    do
    {
      me->_draw_pixel(xpos, ypos, value);
      if (++xpos > xe)
      {
        xpos = xs;
        if (++ypos > ye)
        {
          ypos = ys;
        }
      }
    } while (--length);
    me->_xpos = xpos;
    me->_ypos = ypos;
//    me->_update_transferred_rect(xs, ys, xe, ye);
  }

  void Panel_GDEW0154M09::writePixels(PanelCommon* panel, LGFX_Device* gfx, std::int32_t length, pixelcopy_t* param)
  {
    auto me = reinterpret_cast<Panel_GDEW0154M09*>(panel);
    {
      std::int32_t xs = me->_xs;
      std::int32_t xe = me->_xe;
      std::int32_t ys = me->_ys;
      std::int32_t ye = me->_ye;
      me->_update_transferred_rect(gfx, xs, ys, xe, ye);
    }
    std::int32_t xs   = me->_xs  ;
    std::int32_t ys   = me->_ys  ;
    std::int32_t xe   = me->_xe  ;
    std::int32_t ye   = me->_ye  ;
    std::int32_t xpos = me->_xpos;
    std::int32_t ypos = me->_ypos;

    static constexpr int32_t buflen = 16;
    swap565_t colors[buflen];
    int bufpos = buflen;
    do
    {
      if (bufpos == buflen) {
        param->fp_copy(colors, 0, std::min(length, buflen), param);
        bufpos = 0;
      }
      auto color = colors[bufpos++];
      me->_draw_pixel(xpos, ypos, (color.R8() + (color.G8() << 1) + color.B8() + 3) >> 2);
      if (++xpos > xe)
      {
        xpos = xs;
        if (++ypos > ye)
        {
          ypos = ys;
        }
      }
    } while (--length);
    me->_xpos = xpos;
    me->_ypos = ypos;
//    me->_update_transferred_rect(xs, ys, xe, ye);
  }

  void Panel_GDEW0154M09::readRect(PanelCommon* panel, LGFX_Device*, std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, void* dst, pixelcopy_t* param)
  {
    auto me = reinterpret_cast<Panel_GDEW0154M09*>(panel);

    swap565_t readbuf[w];
    std::int32_t readpos = 0;
    h += y;
    do
    {
      std::int32_t idx = 0;
      do
      {
        readbuf[idx] = me->_read_pixel(x + idx, y) ? ~0u : 0;
      } while (++idx != w);
      param->src_x32 = 0;
      readpos = param->fp_copy(dst, readpos, readpos + w, param);
    } while (++y < h);
  }

  void Panel_GDEW0154M09::_exec_transfer(std::uint32_t cmd, LGFX_Device* gfx, range_rect_t* range, bool invert)
  {
    std::int32_t xs = range->left & ~7;
    std::int32_t xe = range->right & ~7;

    if (gpio_busy >= 0) while (!gpio_in(gpio_busy)) delay(1);

    gfx->writeCommand(0x91);
    gfx->writeCommand(0x90);
    gfx->writeData16(xs << 8 | xe);
    gfx->writeData16(range->top);
    gfx->writeData16(range->bottom);
    gfx->writeData(1);

    if (gpio_busy >= 0) while (!gpio_in(gpio_busy)) delay(1);

    gfx->writeCommand(cmd);
    std::int32_t w = ((xe - xs) >> 3) + 1;
    std::int32_t y = range->top;
    std::int32_t add = ((panel_width + 7) & ~7) >> 3;
    auto b = &_buf[xs >> 3];
    if (invert)
    {
      b += y * add;
      do
      {
        std::int32_t i = 0;
        do
        {
          gfx->writeData(~b[i]);
        } while (++i != w);
        b += add;
      } while (++y <= range->bottom);
    }
    else
    do
    {
      gfx->writeBytes(&b[add * y], w);
    } while (++y <= range->bottom);
    range->top = INT_MAX;
    range->left = INT_MAX;
    range->right = 0;
    range->bottom = 0;
  }

  void Panel_GDEW0154M09::_close_transfer(LGFX_Device* gfx)
  {
    if (_range_old.empty()) return;
    _exec_transfer(0x10, gfx, &_range_old);
    gfx->waitDMA();
  }

  void Panel_GDEW0154M09::display(PanelCommon* panel, LGFX_Device* gfx)
  {
    auto me = reinterpret_cast<Panel_GDEW0154M09*>(panel);
    me->_close_transfer(gfx);
    if (me->_range_new.empty()) return;
    me->_range_old = me->_range_new;
    me->_exec_transfer(0x13, gfx, &me->_range_new);
    if (me->gpio_busy >= 0) while (!gpio_in(me->gpio_busy)) delay(1);
    gfx->writeCommand(0x12);
  }

  void Panel_GDEW0154M09::waitDisplay(PanelCommon* panel, LGFX_Device* gfx)
  {
    auto me = reinterpret_cast<Panel_GDEW0154M09*>(panel);
    gfx->waitDMA();
    if (me->gpio_busy >= 0) while (!gpio_in(me->gpio_busy)) delay(1);
  }

  /*
  void Panel_GDEW0154M09::beginTransaction(PanelCommon* panel, LGFX_Device* gfx)
  {
    auto me = reinterpret_cast<Panel_GDEW0154M09*>(panel);
    me->_close_transfer(gfx);
  }

  void Panel_GDEW0154M09::endTransaction(PanelCommon* panel, LGFX_Device* gfx)
  {
    display(panel, gfx);
  }
  //*/
}
