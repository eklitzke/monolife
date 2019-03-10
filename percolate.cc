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
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#include <monome.h>

// The default device to use.
const char kDefaultDevice[] = "/dev/ttyUSB0";

class State {
public:
  State() = delete;
  State(const std::string &device, int delay) : started_(false), delay_(delay) {
    m_ = monome_open(device.c_str());
    if (m_ == nullptr) {
      exit(EXIT_FAILURE);
    }
    clear();

    const size_t sz = rows() * cols();
    world_.resize(sz, 0);
  }

  void generate(double threshold) {
    std::random_device r;
    std::default_random_engine gen{r()};
    std::uniform_real_distribution<double> dist(0., 1.);
    for (int i = 0; i < cols(); i++) {
      for (int j = 0; j < rows(); j++) {
        bool on = dist(gen) < threshold;
        if (on) {
          at(i, j) = 1;
          led_on(i, j);
        } else {
          at(i, j) = 0;
          led_off(i, j);
        }
      }
    }
  }

  void simulate() {
    bool percolated = false;
    std::vector<int> prev(rows(), 1);
    for (int i = 0; i < cols(); i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_));
      std::vector<int> next(rows(), 0);
      for (int j = 0; j < rows(); j++) {
        if (prev[j] && !at(i, j)) {
          next[j] = 1;
          for (int k = j - 1; k >= 0 && !at(i, k); k--) {
            next[k] = 1;
          }
          for (int k = j + 1; k < rows() && !at(i, k); k++) {
            next[k] = 1;
          }
        }
      }

      percolated = false;
      for (size_t j = 0; j < next.size(); j++) {
        if (next[j]) {
        at(i, j) = 1;
        led_on(i, j);
        percolated = true;
        }
      }

      if (!percolated) {
        break;
      }
      prev = next;
    }

    // victory strobe
    if (percolated) {
      clear(1);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    } else {
      clear();
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
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

  void pause() { started_ = false; }

  // hack to force break ot of event loop
  void force_stop() {
    clear();
    close(monome_get_fd(m_));
    exit(EXIT_SUCCESS);
  }

  bool started() const { return started_; }

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
    return world_[x * c + y];
  }

  void led_on(int x, int y) { monome_led_on(m_, x, y); }

  void led_off(int x, int y) { monome_led_off(m_, x, y); }

  void led_intensity(unsigned int brightness) {
    monome_led_intensity(m_, brightness);
  }

private:
  monome_t *m_;
  bool started_;
  int delay_;
  std::vector<uint8_t> world_;

  void clear(int value=0) { monome_led_all(m_, value); }

  void poll_events() {
    while (monome_event_handle_next(m_))
      ;
  }
};

int main(int argc, char **argv) {
  int opt;
  bool clear = false;
  int millis = 500, intensity = 8;
  double probability = 0.45;
  std::string device = kDefaultDevice;
  while ((opt = getopt(argc, argv, "ci:d:t:p:")) != -1) {
    switch (opt) {
    case 'c':
      clear = true;
      break;
    case 'd':
      device = optarg;
      break;
    case 'i':
      intensity = std::stod(optarg);
      break;
    case 't':
      millis = std::stoi(optarg);
      break;
    case 'p':
      probability = std::stod(optarg);
      break;
    default: /* '?' */
      std::cerr << "Usage: " << argv[0]
                << "[-c] [-d DEVICE] [-i INTENSITY] [-t MILLIS]\n";
      return 1;
    }
  }

  State state(device, millis);
  if (intensity) {
    state.led_intensity(intensity);
  }
  for (;;) {
    state.generate(probability);
    state.simulate();
  }
  return 0;
}
