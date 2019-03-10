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

#include <cstddef>
#include <cstring>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#include <monome.h>

void handle_press(const monome_event_t *e, void *data);

class State {
public:
  State() = delete;
  State(const std::string &device, int delay) : started_(false), delay_(delay) {
    m_ = monome_open(device.c_str());
    if (m_ == nullptr) {
      exit(EXIT_FAILURE);
    }
    clear();

    std::cout << "device has " << rows() << " rows, " << cols() << " cols\n";
    const size_t sz = rows() * cols();
    world_a_.resize(sz, 0);
    world_b_.resize(sz, 0);
    active_ = &world_a_;

    monome_register_handler(m_, MONOME_BUTTON_DOWN, handle_press, (void *)this);
  }

  void run(void) { monome_event_loop(m_); }

  ~State() {
    clear();
    monome_close(m_);
  }

  // get the number of rows
  int rows() const { return monome_get_rows(m_); }

  // get the number of columns
  int cols() const { return monome_get_cols(m_); }

  void start(void) {
    std::cout << "starting\n";
    started_ = true;
    size_t living_count = 1;

    while (living_count) {
      poll_events();
      bool a_active = active_ == &world_a_;
      std::vector<uint8_t> &other = a_active ? world_b_ : world_a_;

      // update other vector
      living_count = 0;
      for (int i = 0; i < cols(); i++) {
        for (int j = 0; j < rows(); j++) {
          size_t nn = count_neighbors(i, j);
          bool live = at(i, j);
          if (live && (nn == 2 || nn == 3)) {
            other[i * cols() + j] = 1;
            living_count++;
          } else if (!live && nn == 3) {
            other[i * cols() + j] = 1;
            living_count++;
          } else {
            other[i * cols() + j] = 0;
          }
        }
      }
      std::cout << "living " << living_count << "\n";

      for (int x = 0; x < cols(); x++) {
        for (int y = 0; y < rows(); y++) {
          if (at(x, y) && !at(x, y, other)) {
            led_off(x, y);
          } else if (!at(x, y) && at(x, y, other)) {
            led_on(x, y);
          }
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(delay_));
      active_ = a_active ? &world_b_ : &world_a_;
    }
  }

  bool started() const { return started_; }

  uint8_t& at(int x, int y, std::vector<uint8_t> &vec) {
    const int c = cols();
    if (x < 0) {
      x += c;
    } else if (x >= c) {
      x -= c;
    }

    const int r = rows();
    if (y < 0) {
      y += r;
    } else if (y >= r) {
      y -= r;
    }
    return vec[x * c + y];
  }

  uint8_t& at(int x, int y) { return at(x, y, *active_); }

  size_t count_neighbors(int x, int y) {
    size_t count = 0;
    count += at(x - 1, y - 1);
    count += at(x - 1, y);
    count += at(x - 1, y + 1);
    count += at(x, y - 1);
    count += at(x, y + 1);
    count += at(x + 1, y - 1);
    count += at(x + 1, y);
    count += at(x + 1, y + 1);
    return count;
  }

  void led_on(int x, int y) { monome_led_on(m_, x, y); }

  void led_off(int x, int y) { monome_led_off(m_, x, y); }

private:
  monome_t *m_;
  bool started_;
  int delay_;
  std::vector<uint8_t> world_a_;
  std::vector<uint8_t> world_b_;
  std::vector<uint8_t> *active_;

  void clear() { monome_led_all(m_, 0); }

  void poll_events() {
    while (monome_event_handle_next(m_))
      ;
  }
};

void handle_press(const monome_event_t *e, void *data) {
  State *state = reinterpret_cast<State *>(data);
  if (e->event_type == MONOME_BUTTON_DOWN) {
    const int x = e->grid.x;
    const int y = e->grid.y;
    std::cout << "keypress at " << x << " " << y << "\n";
    if (state->started()) {
      std::cout << "ignore\n";
      return;
    } else if (x == 0 && y == 0) {
      state->start();
      return;
    }

    uint8_t &val = state->at(x, y);
    if (val) {
      val = 0;
      state->led_off(x, y);
      std::cout << "turning off\n";
    } else {
      val = 1;
      state->led_on(x, y);
      std::cout << "turning on\n";
    }
  }
}

int main(int argc, char **argv) {
  int opt;
  bool clear = false;
  int millis = 100;
  std::string device = "/dev/ttyUSB0";
  while ((opt = getopt(argc, argv, "cd:t:")) != -1) {
    switch (opt) {
    case 'c':
      clear = true;
      break;
    case 'd':
      device = optarg;
      break;
      case 't':
        millis = std::stoi(optarg);
        break;
    default: /* '?' */
      std::cerr << "Usage: " << argv[0] << "[-c] [-d DEVICE]\n";
      return 1;
    }
  }

  State state(device, millis);
  if (!clear) {
    state.run();
  }
  std::cout << "done\n";
  return 0;
}
