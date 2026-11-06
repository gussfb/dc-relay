// Sonda TCP conservadora: só "timeout" conta como morto.
// Conexão aceita OU recusada (RST) prova que o host está ligado.
#pragma once
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <fcntl.h>
#include <errno.h>

enum WdResult { WD_DEAD = 0, WD_ALIVE = 1, WD_ERROR = 2 };

static WdResult wd_probe(const char *host, uint16_t port, uint32_t timeout_ms) {
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_aton(host, &addr.sin_addr) == 0) return WD_ERROR;

  int fd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0) return WD_ERROR;
  lwip_fcntl(fd, F_SETFL, lwip_fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

  WdResult res = WD_DEAD;
  int rc = lwip_connect(fd, (struct sockaddr *) &addr, sizeof(addr));
  if (rc == 0) {
    res = WD_ALIVE;
  } else if (errno == EINPROGRESS) {
    fd_set wr, er;
    FD_ZERO(&wr); FD_ZERO(&er);
    FD_SET(fd, &wr); FD_SET(fd, &er);
    struct timeval tv = { (int) (timeout_ms / 1000), (int) ((timeout_ms % 1000) * 1000) };
    int sel = lwip_select(fd + 1, nullptr, &wr, &er, &tv);
    if (sel > 0) {
      int err = 0; socklen_t len = sizeof(err);
      lwip_getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
      // 0 = conectou, ECONNREFUSED = host vivo mas porta fechada
      res = (err == 0 || err == ECONNREFUSED) ? WD_ALIVE : WD_DEAD;
    }
  } else if (errno == ECONNREFUSED) {
    res = WD_ALIVE;
  }
  lwip_close(fd);
  return res;
}
