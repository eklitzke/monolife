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

#include <cstddef>

class RunningAverage {
public:
  RunningAverage() : count_(0), val_(0.5) {}
  explicit RunningAverage(double start) : count_(1), val_(start) {}

  void update(double input) {
    double scale = 1. / static_cast<double>(++count_);
    val_ = input * scale + val_ * (1. - scale);
  }

  double val() const { return val_; }

private:
  size_t count_;
  double val_;
};
