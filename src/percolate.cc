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
#include <random>
#include <set>
#include <string>
#include <vector>

#include <unistd.h>

#include <monome.h>

#include "./board.h"
#include "./running_average.h"

enum class State {
  GENERATE = 1,
  STEP = 2,
  VICTORY = 3,
  FAIL = 4,
};

// step callback
static void step_cb(evutil_socket_t fd, short what, void *arg);

// The board state.
class BoardState {
public:
  BoardState() = delete;
  explicit BoardState(const std::string &device)
      : board_(device), state_(State::GENERATE), gen_(std::random_device()()),
        dist_(0., 1.) {
    world_.resize(board_.rows() * board_.cols());

    // set a callback to handle button down events
    board_.set_event_fn([this](const monome_event_t *event) {
      if (event->event_type == MONOME_BUTTON_DOWN) {
        std::cout << "DOWN event at " << event->grid.x << " " << event->grid.y
                  << "\n";
        const int brightness = 16 * event->grid.y / board_.rows();
        std::cout << "setting brightness to " << brightness << "\n";
        board_.led_intensity(brightness);

        const int delay = 25 * (1 + event->grid.x);
        std::cout << "setting delay to " << delay << "\n";
        delay_ = delay;
      }
    });
  }

  ~BoardState() { board_.led_all(0); }

  void step() {
    switch (state_) {
    case State::GENERATE:
      std::cout << "step=" << threshold_.count()
                << " threshold=" << threshold_.val() << "\n";
      generate();
      state_ = State::STEP;
      break;
    case State::STEP:
      simulate_step();
      break;
    case State::VICTORY:
    case State::FAIL:
      state_ = State::GENERATE;
      break;
    }

    // re-schedule step() with the current delay
    const timeval tv = delay();
    evtimer_add(t_, &tv);
  }

  // generate a new board state
  void generate() {
    clear();
    std::fill(world_.begin(), world_.end(), 0);
    for (int i = 0; i < board_.cols(); i++) {
      for (int j = 0; j < board_.rows(); j++) {
        if (dist_(gen_) < threshold_.val()) {
          at(i, j) = 1;
          board_.led_on(i, j);
        }
      }
    }

    // set up the next set of leds
    next_.clear();
    for (int j = 0; j < board_.rows(); j++) {
      if (off(0, j)) {
        next_.insert({0, j});
      }
    }
  }

  void simulate_step() {
    bool reached_end = false;
    for (const auto &pr : next_) {
      board_.led_on(pr.first, pr.second);
      at(pr.first, pr.second) = 1;
      if (pr.first == board_.cols() - 1) {
        reached_end = true;
      }
    }

    std::set<std::pair<int, int>> new_next;
    for (const auto &pr : next_) {
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

    if (reached_end) {
      state_ = State::VICTORY;
      threshold_.update(1);
    } else if (new_next.empty()) {
      state_ = State::FAIL;
      threshold_.update(0);
    } else {
      next_ = new_next;
    }
  }

  // set the density threshold
  void set_threshold(const RunningAverage &avg) { threshold_ = avg; }

  void run(int millis) {
    board_.init_libevent();
    delay_ = millis;
    t_ = evtimer_new(board_.base(), step_cb, this);
    event_active(t_, 0, 0);
    board_.start_libevent();
  }

  Board &board() { return board_; }

private:
  Board board_;
  State state_;
  std::set<std::pair<int, int>> next_;
  RunningAverage threshold_;
  std::vector<uint8_t> world_;
  int delay_;
  event *t_;

  std::default_random_engine gen_;
  std::uniform_real_distribution<double> dist_;

  // clear all leds on the board
  void clear(int value = 0) { board_.led_all(value); }

  // is a light off?
  bool off(int x, int y) {
    if (x < 0 || y < 0) {
      return false;
    }
    if (x >= board_.cols() || y >= board_.rows()) {
      return false;
    }
    return world_[x * board_.rows() + y] == 0;
  }

  // get ref to a coordinate with wraparound
  uint8_t &at(int x, int y) {
    const int c = board_.cols();
    if (x < 0) {
      x += c;
    } else if (x >= c) {
      x -= c;
    }

    const int r = board_.rows();
    if (y < 0) {
      y += r;
    } else if (y >= r) {
      y -= r;
    }
    return world_[x * r + y];
  }

  // convert milliseconds to a timeval
  timeval delay(void) const { return {delay_ / 1000, (delay_ % 1000) * 1000}; }
};

int main(int argc, char **argv) {
  int opt;
  int millis = 100, intensity = 8;
  double threshold = 0.;
  std::string device;
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

  BoardState state(device);
  if (intensity) {
    state.board().led_intensity(intensity);
  }
  state.set_threshold(RunningAverage(threshold));
  state.run(millis);
  return 0;
}

static void step_cb(evutil_socket_t fd, short what, void *arg) {
  BoardState *state = (BoardState *)arg;
  state->step();
}
