#pragma once
#include <stdint.h>

class RateLimter {
public:
  RateLimter(){};
  virtual ~RateLimter(){};
  virtual void SetRate(uint32_t permitsPerSecond);
  virtual void Aquire(uint32_t permits = 1) = 0;
  virtual int64_t TryAquire(uint32_t permits = 1) = 0;

protected:
  virtual void doSetRate(uint32_t permitsPerSecond) = 0;
  int64_t getNowUs();

protected:
  uint32_t m_storedPermits;
  uint32_t m_maxPermits;
  uint32_t m_intervalUs;
  int64_t m_nextFreeTicketUs;
};

/*
 * refer to Guava's SmoothBursty rate limter
 */
class SmoothBursty final : public RateLimter {
public:
  SmoothBursty(uint32_t maxBurstySeconds);
  virtual void Aquire(uint32_t permits = 1) override;
  virtual int64_t TryAquire(uint32_t permits = 1) override;

protected:
  virtual void doSetRate(uint32_t permitsPerSecond) override;
  void resync(int64_t nowUs);
  int64_t reserveEarliestAvailable(uint32_t requiredPermits, int64_t nowUs);
  bool canAquire(int64_t nowUs);

private:
  uint32_t m_maxBurstSeconds;
};
