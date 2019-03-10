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

#include "./board.h"

// default event handler
void default_event_handler(const monome_event_t *e) {
  const int x = e->grid.x;
  const int y = e->grid.y;
  if (e->event_type == MONOME_BUTTON_DOWN) {
    std::cout << "KEY DOWN ";
  } else if (e->event_type == MONOME_BUTTON_UP) {
    std::cout << "KEY UP   ";
  }
  std::cout << x << " " << y << "\n";
}

void on_keypress(const monome_event_t *e, void *data) {
  reinterpret_cast<Board *>(data)->invoke(e);
}

void on_read(evutil_socket_t fd, short what, void *arg) {
  reinterpret_cast<Board *>(arg)->poll_events();
}
