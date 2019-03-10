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
#include <iostream>
#include <string>

#include <event2/event.h>
#include <monome.h>

#include "./config.h"

// Board represents a monome board.
class Board {
public:
  explicit Board(const char *device) : base_(nullptr) {
    if (device == nullptr || strlen(device) == 0) {
      m_ = monome_open(MONOLIFE_DEFAULT_DEVICE);
    } else {
      m_ = monome_open(device);
    }
  }

  explicit Board(const std::string &device) : Board(device.c_str()) {}

  Board() : Board(nullptr) {}

  ~Board() {
    if (m_ != nullptr) {
      monome_close(m_);
    }
    if (base_ != nullptr) {
      event_base_free(base_);
    }
  }

  // initialize libevent
  void init_libevent() {
    base_ = event_base_new();
  }

  // start libevent poll loop
  void start_libevent() {
    // callback to invoke when there's data to be read from the board
    auto read_cb = [](evutil_socket_t fd, short what, void *arg) {
      Board *board = (Board *)arg;
      std::cerr << "got a board event\n";
      board->poll_events();
    };

    // read event
    int fd = monome_get_fd(m_);
    auto r = event_new(base_, fd, EV_READ | EV_PERSIST, read_cb, this);
    event_add(r, NULL);
    event_base_dispatch(base_);
  }

  // poll for events
  void poll_events() {
    while (monome_event_handle_next(m_))
      ;
  }

  // set all leds to a color
  void led_all(unsigned int val) const { monome_led_all(m_, val); }

  // get the number of rows
  int rows() const { return monome_get_rows(m_); }

  // get the number of columns
  int cols() const { return monome_get_cols(m_); }

  // set the led intensity
  void led_intensity(unsigned int intensity) const {
    monome_led_intensity(m_, intensity);
  }

  // get the libevent base
  event_base* base() { return base_; }

  // is the board ok?
  bool ok() const { return m_ != nullptr; }

private:
  monome_t *m_;
  event_base *base_;
};
