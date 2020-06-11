#ifndef CN_PARAM
#define CN_PARAM
/* set parameter */
const uint32_t rtt = 150;  // ms
const uint16_t mss = 1024;  // bytes
const uint16_t threshold = 4 * mss;  // bytes
const uint32_t buffer_size = 32 * 1024;  // 32KB
const uint32_t port = 10101;

const int gLOSS_RATE = 5; // %
#endif
