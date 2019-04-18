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
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>

#include <event2/event.h>
#include <monome.h>

// the prefix for the board device
static const std::string devicePrefix = "/dev/ttyUSB";

// callback function for monome events
using event_fn = std::function<void(const monome_event_t *)>;

// default event handler
static void default_event_handler(const monome_event_t *);

// default keypress handler
static void on_keypress(const monome_event_t *, void *data);

// handler to read events from the socket
static void on_read(evutil_socket_t fd, short what, void *arg);

// find an appropriate board device
static std::string findBoardDevice(const std::string &dev);

// Board represents a monome board.
class Board {
public:
  explicit Board(const std::string &device)
      : m_(monome_open(findBoardDevice(device).c_str())), base_(nullptr),
        event_fn_(default_event_handler) {}

  // default ctor uses the default device
  Board() : Board("") {}

  // delete copy ctor
  Board(const Board &other) = delete;

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
    monome_register_handler(m_, MONOME_BUTTON_DOWN, on_keypress, this);
    monome_register_handler(m_, MONOME_BUTTON_UP, on_keypress, this);
  }

  // start libevent poll loop
  void start_libevent() {
    // read event
    int fd = monome_get_fd(m_);
    event *r = event_new(base_, fd, EV_READ | EV_PERSIST, on_read, this);
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

  // turn an led on
  void led_on(int x, int y) { monome_led_on(m_, x, y); }

  // turn an led off
  void led_off(int x, int y) { monome_led_off(m_, x, y); }

  // get the number of rows
  int rows() const { return monome_get_rows(m_); }

  // get the number of columns
  int cols() const { return monome_get_cols(m_); }

  // set the led intensity
  void led_intensity(unsigned int intensity) const {
    monome_led_intensity(m_, intensity);
  }

  // set an event function
  void set_event_fn(event_fn fn) { event_fn_ = fn; }

  // invoke the event fn
  void invoke(const monome_event_t *event) { event_fn_(event); }

  // get the libevent base
  event_base *base() { return base_; }

  // is the board ok?
  bool ok() const { return m_ != nullptr; }

private:
  monome_t *m_;
  event_base *base_;
  event_fn event_fn_;
};

// default event handler
static void default_event_handler(const monome_event_t *e) {
  const int x = e->grid.x;
  const int y = e->grid.y;
  if (e->event_type == MONOME_BUTTON_DOWN) {
    std::cout << "KEY DOWN ";
  } else if (e->event_type == MONOME_BUTTON_UP) {
    std::cout << "KEY UP   ";
  }
  std::cout << x << " " << y << "\n";
}

static void on_keypress(const monome_event_t *e, void *data) {
  reinterpret_cast<Board *>(data)->invoke(e);
}

static void on_read(evutil_socket_t fd, short what, void *arg) {
  reinterpret_cast<Board *>(arg)->poll_events();
}

static std::string findBoardDevice(const std::string &dev) {
  if (!dev.empty()) {
    if (!std::filesystem::exists(dev)) {
      throw std::runtime_error("no such device: " + dev);
    }
    return dev;
  }
  std::string device;
  for (int i = 10; i >= 0; i--) {
    device = devicePrefix + std::to_string(i);
    if (std::filesystem::exists(device)) {
      return device;
    }
  }
  throw std::runtime_error("failed to autodetect monome TTY device");
}
