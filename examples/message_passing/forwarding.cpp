#include <iostream>
#include "caf/all.hpp"

using std::endl;
using namespace caf;

using add_atom = atom_constant<atom("add")>;

void actor_a(event_based_actor* self, actor worker) {
  self->request(worker, add_atom::value, 1, 2).then(
    [=](int result) {
      aout(self) << "1 + 2 = " << result << endl;
      self->send_exit(worker, exit_reason::user_shutdown);
    }
  );
}

behavior actor_b(event_based_actor* self, actor worker) {
  self->link_to(worker);
  return {
    others >> [=] {
      return self->forward_to(worker);
    }
  };
}

behavior actor_c() {
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
