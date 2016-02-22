#include <iostream>
#include "caf/all.hpp"

using std::endl;
using namespace caf;

using add_atom = atom_constant<atom("add")>;

using calc = typed_actor<replies_to<add_atom, int, int>::with<int>>;

void actor_a(event_based_actor* self, calc worker) {
  self->request(worker, add_atom::value, 1, 2).then(
    [=](int result) {
      aout(self) << "1 + 2 = " << result << endl;
      self->send_exit(worker, exit_reason::user_shutdown);
    }
  );
}

calc::behavior_type actor_b(calc::pointer self, calc worker) {
  self->link_to(worker);
  return {
    [=](add_atom add, int x, int y) {
      return self->delegate(worker, add, x, y);
    }
  };
}

calc::behavior_type actor_c() {
  return {
    [](add_atom, int x, int y) {
      return x + y;
    }
  };
}

int main() {
  actor_system system;
  system.spawn(actor_a, system.spawn(actor_b, system.spawn(actor_c)));
}
