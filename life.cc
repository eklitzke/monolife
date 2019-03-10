#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#include <monome.h>

struct cell_t {
  unsigned int alive;
  unsigned int mod_next;

  unsigned int x;
  unsigned int y;

  cell_t *neighbors[8];
  unsigned int nnum;
};

void handle_press(const monome_event_t *e, void *data);

class State {
public:
  State() = delete;
  explicit State(const std::string &device) : started_(false) {
    m_ = monome_open(device.c_str());
    if (m_ == nullptr) {
      exit(EXIT_FAILURE);
    }
    clear();

    const int rows = get_rows();
    const int cols = get_cols();

    for (int i = 0; i < cols; i++) {
      world_.push_back(std::vector<cell_t>(rows));
    }

    for (int x = 0; x < cols; x++) {
      for (int y = 0; y < rows; y++) {
        cell_t *c = &world_[x][y];

        c->mod_next = 0;
        c->alive = 0;
        c->nnum = 0;
        c->x = x;
        c->y = y;

        c->neighbors[0] = &world_[(x - 1) % cols][(y - 1) % rows];
        c->neighbors[1] = &world_[(x - 1) % cols][(y + 1) % rows];
        c->neighbors[2] = &world_[(x - 1) % cols][y];

        c->neighbors[3] = &world_[(x + 1) % cols][(y - 1) % rows];
        c->neighbors[4] = &world_[(x + 1) % cols][(y + 1) % rows];
        c->neighbors[5] = &world_[(x + 1) % cols][y];

        c->neighbors[6] = &world_[x][(y - 1) % rows];
        c->neighbors[7] = &world_[x][(y + 1) % rows];
      }
    }

    monome_register_handler(m_, MONOME_BUTTON_DOWN, handle_press, (void *)this);
  }

  void run(void) { monome_event_loop(m_); }

  ~State() {
    clear();
    monome_close(m_);
  }

  int get_rows() const { return monome_get_rows(m_); }
  int get_cols() const { return monome_get_cols(m_); }

  void start(void) {
    std::cout << "starting\n";
    started_ = true;
    int tick = 0;
    bool any_living = true;
    while (any_living) {
      std::cout << "tick " << tick << std::endl;
      tick++;
      if (!(tick %= 3))
        while (monome_event_handle_next(m_))
          ;

      const int rows = get_rows();
      const int cols = get_cols();

      for (int x = 0; x < cols; x++) {
        for (int y = 0; y < rows; y++) {
          cell_t &c = world(x, y);

          if (c.mod_next) {
            if (c.alive) {
              c.alive = 0;
              mod_neighbors(&c, -1);

              monome_led_off(m_, x, y);
            } else {
              c.alive = 1;
              mod_neighbors(&c, 1);

              monome_led_on(m_, x, y);
            }

            c.mod_next = 0;
          }
        }
      }

      any_living = false;
      for (int x = 0; x < cols; x++) {
        for (int y = 0; y < rows; y++) {
          cell_t &c = world(x, y);

          switch (c.nnum) {
          case 3:
            if (!c.alive)
              c.mod_next = 1;

          case 2:
            break;

          default:
            if (c.alive) {
              c.mod_next = 1;
            }

            break;
          }

          if (c.mod_next) {
            any_living = true;
          }
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  }

  void mod_neighbors(cell_t *c, int delta) {
    for (int i = 0; i < 8; i++)
      c->neighbors[i]->nnum += delta;
  }

  bool started() const { return started_; }

  cell_t &world(int x, int y) { return world_[x][y]; }

  void led_on(int x, int y) { monome_led_on(m_, x, y); }

  void led_off(int x, int y) { monome_led_off(m_, x, y); }

private:
  monome_t *m_;
  bool started_;
  std::vector<std::vector<cell_t>> world_;

  void clear() { monome_led_all(m_, 0); }
};

void handle_press(const monome_event_t *e, void *data) {
  State *STATE = reinterpret_cast<State *>(data);
  if (e->event_type == MONOME_BUTTON_DOWN) {
    const int x = e->grid.x;
    const int y = e->grid.y;
    std::cout << "keypress at " << x << " " << y << "\n";
    if (STATE->started()) {
      std::cout << "ignore\n";
      return;
    } else if (x == 0 && y == 0) {
      STATE->start();
      return;
    }

    auto cell = STATE->world(x, y);
    if (cell.mod_next) {
      cell.alive = 0;
      cell.mod_next = 0;
      STATE->mod_neighbors(&cell, -1);
      STATE->led_off(x, y);
      std::cout << "turning off\n";
    } else {
      cell.alive = 1;
      cell.mod_next = 1;
      STATE->mod_neighbors(&cell, 1);
      STATE->led_on(x, y);
      std::cout << "turning on\n";
    }
  }
}

int main(int argc, char **argv) {
  int opt;
  bool clear = false;
  std::string device = "/dev/ttyUSB0";
  while ((opt = getopt(argc, argv, "cd:")) != -1) {
    switch (opt) {
    case 'c':
      clear = true;
      break;
    case 'd':
      device = optarg;
      break;
    default: /* '?' */
      std::cerr << "Usage: " << argv[0] << "[-c] [-d DEVICE]\n";
      return 1;
    }
  }

  State state(device);
  state.run();
  return 0;
}
