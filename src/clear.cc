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

#include <iostream>
#include <string>

#include <unistd.h>

#include <monome.h>

// The default device to use.
const char kDefaultDevice[] = "/dev/ttyUSB0";

int main(int argc, char **argv) {
  int opt;
  int intensity = 0;
  std::string device = kDefaultDevice;
  while ((opt = getopt(argc, argv, "i:d:")) != -1) {
    switch (opt) {
    case 'd':
      device = optarg;
      break;
    case 'i':
      intensity = std::stod(optarg);
      break;
    default: /* '?' */
      std::cerr << "Usage: " << argv[0] << " [-d DEVICE] [-i INTENSITY]\n";
      return 1;
    }
  }
  monome_t *m = monome_open(device.c_str());
  if (m == nullptr) {
    std::cerr << "failed to open device: " << device << "\n";
    return 1;
  }
  if (intensity) {
    monome_led_intensity(m, intensity);
  }
  monome_led_all(m, 0);
  monome_close(m);
  return 0;
}
