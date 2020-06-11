#ifndef CN_PARAM
#define CN_PARAM

//#define DEBUG
//#define EVERY_SEND_RTT
#define SEND_PACKAGE_INFORM

/* set parameter */
const uint32_t rtt = 150;  // ms
const uint16_t mss = 1024;  // bytes
const uint16_t threshold = 8 * mss;  // bytes
const uint32_t buffer_size = 32 * 1024;  // 32KB
// const uint32_t port = 10101;

const int gLOSS_RATE = 50; // %


enum {
      NO_STATE,
      SLOW_START,
      CONDESTION_AVOIDANCE
};

#endif
