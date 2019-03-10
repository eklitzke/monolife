/*
 * Copyright (C) 2019  Evan Klitzke <evan@eklitzke.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstring>
#include <string>

#include <monome.h>

#include "./config.h"

class Board {
public:
  explicit Board(const char *device) {
    if (device == nullptr || strlen(device) == 0) {
      m_ = monome_open(MONOLIFE_DEFAULT_DEVICE);
    } else {
      m_ = monome_open(device);
    }
  }
  explicit Board(const std::string &device) : Board(device.c_str()) {}
  Board() : Board(nullptr) {}
  ~Board() { monome_close(m_); }

  // set all leds to a color
  void led_all(unsigned int val) const { monome_led_all(m_, val); }

  // set the led intensity
  void led_intensity(unsigned int intensity) const {
    monome_led_intensity(m_, intensity);
  }

  // is the board ok?
  bool ok() const { return m_ != nullptr; }

private:
  monome_t *m_;
};
