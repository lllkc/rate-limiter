#include "rate_limiter.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int main() {
  SmoothBursty rateLimiter(2);
  rateLimiter.SetRate(4);

  for (auto i = 0; i < 20; ++i) {
    rateLimiter.Aquire();
    cout << "got token successfuly!" << endl;
  }

  cout << "===============" << endl;

  for (auto i = 0; i < 20; ++i) {
    auto waitUs = rateLimiter.TryAquire();
    usleep(waitUs);
    cout << "got token successfuly!" << endl;
  }
  return 0;
}
