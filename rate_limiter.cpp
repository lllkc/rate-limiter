#include "rate_limiter.h"
#include <algorithm>
#include <assert.h>
#include <climits>
#include <mutex>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

void RateLimter::SetRate(uint32_t permitsPerSecond) {
  assert(permitsPerSecond >= 1);
  doSetRate(permitsPerSecond);
}

int64_t RateLimter::getNowUs() {
  timeval now;
  gettimeofday(&now, nullptr);
  return now.tv_sec * 1e6 + now.tv_usec;
};

std::mutex g_mutex;

SmoothBursty::SmoothBursty(uint32_t maxBurstySeconds)
    : m_maxBurstSeconds(maxBurstySeconds) {
  assert(maxBurstySeconds > 0);
  m_storedPermits = 0;
  m_maxPermits = 0;
  m_intervalUs = INT_MAX;
  m_nextFreeTicketUs = 0;
}

void SmoothBursty::doSetRate(uint32_t permitsPerSecond) {
  std::lock_guard<std::mutex> guard(g_mutex);

  m_intervalUs = 1e6 / permitsPerSecond;
  resync(getNowUs());

  auto oldMaxPermits = m_maxPermits;
  m_maxPermits = m_maxBurstSeconds * permitsPerSecond;
  m_storedPermits =
      (oldMaxPermits == 0) ? 0 : m_storedPermits * m_maxPermits / oldMaxPermits;
}

void SmoothBursty::resync(int64_t nowUs) {
  // printf("in resync nowUs:%ld\n", nowUs);
  if (nowUs > m_nextFreeTicketUs) {
    m_storedPermits =
        std::min((int64_t)m_maxPermits,
                 m_storedPermits + (nowUs - m_nextFreeTicketUs) / m_intervalUs);
    m_nextFreeTicketUs = nowUs;
  }
}

int64_t SmoothBursty::reserveEarliestAvailable(uint32_t requiredPermits,
                                               int64_t nowUs) {
  resync(nowUs);
  int64_t returnVal = m_nextFreeTicketUs;
  uint32_t storePermitsToSpend =
      std::min(requiredPermits, (uint32_t)m_storedPermits);
  auto freshPermits = requiredPermits - storePermitsToSpend;
  auto waitUs = freshPermits * m_intervalUs;
  m_nextFreeTicketUs += waitUs;
  m_storedPermits -= storePermitsToSpend;
  return returnVal;
}

bool SmoothBursty::canAquire(int64_t nowUs) {
  return nowUs >= m_nextFreeTicketUs;
}

void SmoothBursty::Aquire(uint32_t permits) {
  int64_t momentsAvailable = 0;
  int64_t now = 0;
  {
    std::lock_guard<std::mutex> guard(g_mutex);
    now = getNowUs();
    momentsAvailable = reserveEarliestAvailable(permits, now);
    // printf("momentsAvailable:%ld\n", momentsAvailable);
  }
  if (momentsAvailable > now) {
    int64_t waitSeconds = (momentsAvailable - now) / 1e6;
    int64_t waitUs = (momentsAvailable - now) - waitSeconds * 1e6;
    sleep(waitSeconds);
    usleep(waitUs);
  }
}

int64_t SmoothBursty::TryAquire(uint32_t permits) {
  std::lock_guard<std::mutex> guard(g_mutex);
  auto now = getNowUs();
  if (!canAquire(now)) {
    return m_nextFreeTicketUs - now;
  } else {
    reserveEarliestAvailable(permits, now);
  }
  return 0;
}
