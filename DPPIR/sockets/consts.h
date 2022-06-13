#ifndef DPPIR_SOCKETS_CONSTS_H_
#define DPPIR_SOCKETS_CONSTS_H_

// In bytes.
#define BUFFER_SIZE 140000
#define POLL_RATE 140000
#define PROGRESS_RATE 750000

// TCP socket configs.
// 10 MB
#define RCVBUF 12328960
#define SNDBUF 12328960

// These go beyond the default maximum buffer sizes, which can be changed with
// sudo sysctl -w net.core.rmem_max=123289600
// sudo sysctl -w net.core.wmem_max=123289600

#endif  // DPPIR_SOCKETS_CONSTS_H_
