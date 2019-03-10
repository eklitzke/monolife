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
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#include <monome.h>

#include "./running_average.h"

// The default device to use.
const char kDefaultDevice[] = "/dev/ttyUSB0";

// The board state.
class State {
public:
  State() = delete;
  State(const std::string &device, int millis)
      : delay_(std::chrono::milliseconds(millis)), gen_(std::random_device()()),
        dist_(0., 1.) {
    m_ = monome_open(device.c_str());
    if (m_ == nullptr) {
      exit(EXIT_FAILURE);
    }
    world_.resize(rows() * cols());
  }

  ~State() {
    clear();
    monome_close(m_);
  }

  // generate a new board state
  void generate() {
    clear();
    std::fill(world_.begin(), world_.end(), 0);
    for (int i = 0; i < cols(); i++) {
      for (int j = 0; j < rows(); j++) {
        if (dist_(gen_) < threshold_.val()) {
          at(i, j) = 1;
          led_on(i, j);
        }
      }
    }
  }

  // simulate a percolation
  void simulate() {
    bool percolated = true;
    std::set<std::pair<int, int>> next;
    for (int j = 0; j < rows(); j++) {
      if (at(0, j)) {
        next.insert({0, j});
      }
    }

    for (;;) {
      chill();
      bool reached_end = false;
      for (const auto &pr : next) {
        led_on(pr.first, pr.second);
        at(pr.first, pr.second) = 1;
        if (pr.first == cols() - 1) {
          reached_end = true;
        }
      }
      if (reached_end) {
        break;
      }

      std::set<std::pair<int, int>> new_next;
      for (const auto &pr : next) {
        if (off(pr.first - 1, pr.second)) {
          new_next.insert({pr.first - 1, pr.second});
        }
        if (off(pr.first + 1, pr.second)) {
          new_next.insert({pr.first + 1, pr.second});
        }
        if (off(pr.first, pr.second - 1)) {
          new_next.insert({pr.first, pr.second - 1});
        }
        if (off(pr.first, pr.second + 1)) {
          new_next.insert({pr.first, pr.second + 1});
        }
      }
      if (new_next.empty()) {
        percolated = false;
        break;
      }
      next = new_next;
    }

    threshold_.update(percolated);
    std::cout << "step=" << (threshold_.count() - 1)
              << " threshold=" << threshold_.val() << "\n";
    chill();
  }

  // set the density threshold
  void set_threshold(const RunningAverage &avg) { threshold_ = avg; }

  // get the number of rows
  int rows() const { return monome_get_rows(m_); }

  // get the number of columns
  int cols() const { return monome_get_cols(m_); }

  void led_intensity(unsigned int brightness) {
    monome_led_intensity(m_, brightness);
  }

private:
  monome_t *m_;
  RunningAverage threshold_;
  std::chrono::milliseconds delay_;
  std::vector<uint8_t> world_;

  std::default_random_engine gen_;
  std::uniform_real_distribution<double> dist_;

  // clear all leds on the board
  void clear(int value = 0) { monome_led_all(m_, value); }

  // poll the monome for events
  void poll_events() {
    while (monome_event_handle_next(m_))
      ;
  }

  // chill for a bit
  void chill() const { std::this_thread::sleep_for(delay_); }

  // is a light off?
  bool off(int x, int y) {
    if (x < 0 || y < 0) {
      return false;
    }
    if (x >= cols() || y >= rows()) {
      return false;
    }
    return world_[x * rows() + y] == 0;
  }

  // get ref to a coordinate with wraparound
  uint8_t &at(int x, int y) {
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
    return world_[x * r + y];
  }

  // turn an led on
  void led_on(int x, int y) { monome_led_on(m_, x, y); }

  // turn an led off
  void led_off(int x, int y) { monome_led_off(m_, x, y); }
};

int main(int argc, char **argv) {
  int opt;
  int millis = 100, intensity = 8;
  double threshold = 0.;
  std::string device = kDefaultDevice;
  while ((opt = getopt(argc, argv, "i:d:s:t:")) != -1) {
    switch (opt) {
    case 'd':
      device = optarg;
      break;
    case 'i':
      intensity = std::stod(optarg);
      break;
    case 's':
      millis = std::stoi(optarg);
      break;
    case 't':
      threshold = std::stod(optarg);
      break;
    default: /* '?' */
      std::cerr << "Usage: " << argv[0]
                << "[-c] [-d DEVICE] [-i INTENSITY] [-s SLEEPMILLIS] [-t "
                   "THRESHOLD]\n";
      return 1;
    }
  }

  State state(device, millis);
  if (intensity) {
    state.led_intensity(intensity);
  }
  if (threshold) {
    state.set_threshold(RunningAverage(threshold));
  }
  for (;;) {
    state.generate();
    state.simulate();
  }
  return 0;
}
