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

#include <event2/event.h>
#include <sys/time.h>

class PersistentMutableTimer {
public:
  PersistentMutableTimer() : ev_(nullptr), tv_{0, 0}, scheduled_at_{0, 0} {}
  PersistentMutableTimer(event_base *base, event_callback_fn callback,
                         void *data, int millis)
      : ev_(evtimer_new(base, callback, data)),
        tv_{millis / 1000, (millis % 1000) * 1000}, scheduled_at_{0, 0} {
    event_active(ev_, 0, 0);
  }

  void Reschedule() {
    gettimeofday(&scheduled_at_, nullptr);
    evtimer_add(ev_, &tv_);
  }

  void UpdateTimeout(const timeval &tv) {
    evtimer_del(ev_);
    tv_ = tv;

    timeval now, res;
    gettimeofday(&now, nullptr);
    timersub(&scheduled_at_, &now, &res);
    if (res.tv_sec < 0 || res.tv_usec < 0) {
      event_active(ev_, 0, 0);
    } else {
      evtimer_add(ev_, &res);
    }
  }

  void UpdateTimeout(int millis) {
    UpdateTimeout({millis / 1000, (millis % 1000) * 1000});
  }

private:
  event *ev_;
  timeval tv_;
  timeval scheduled_at_;
};
