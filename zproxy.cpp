#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <zmq.h>
#include <pthread.h>
#include <limits.h>
#include <sys/socket.h>
#include "defs.h"

char* timestring() {
  time_t t;
  static char tbuf[1000];
  time(&t);
  strftime(tbuf, sizeof tbuf, "%D %T", localtime(&t));
  // note static buffer
  return tbuf;
}

void* reply(void* reps) {
  unsigned char buf[128];
  while (1)
    zmq_send(reps, buf, zmq_recv(reps, buf, sizeof buf, 0), 0);
}

void* capture(void* cx) {
  void* cs = zmq_socket(cx, ZMQ_PAIR);

  int v = 0;
  zmq_setsockopt(cs, ZMQ_SNDHWM, &v, sizeof v);
  zmq_setsockopt(cs, ZMQ_RCVHWM, &v, sizeof v);

  RC(zmq_connect(cs, "inproc://cap"));
  while (1) {
    int more, n;
    size_t ms = sizeof more;
    unsigned char key[1000000];

    key[n = zmq_recv(cs, key, sizeof key, 0)] = 0;
    if (n < 0) {
      printf("fuck");
      continue;
    }
    zmq_getsockopt(cs, ZMQ_RCVMORE, &more, &ms);
    if (more) {
      unsigned char body[1000000];
      body[n = zmq_recv(cs, body, sizeof body, 0)] = 0;
      if (n < 0) printf("fuck");
    }
    else {
      printf("%s : \"%s\"\n", timestring(), key);
    }
    fflush(stdout);
  }
}

extern void *rep_socket_monitor (void *ctx);

int main(int argc, char* argv[])
{
  void *cx = zmq_ctx_new();
  RC(zmq_ctx_set(cx, ZMQ_IO_THREADS, 4));

  void *xsub = zmq_socket(cx, ZMQ_XSUB),
    *xpub = zmq_socket(cx, ZMQ_XPUB),
    *reps = zmq_socket(cx, ZMQ_REP),
    *caps = zmq_socket(cx, ZMQ_PAIR);

#if 0
  int val = 0;
  zmq_setsockopt(xpub, ZMQ_SNDHWM, &val, sizeof val);
  zmq_setsockopt(xpub, ZMQ_RCVHWM, &val, sizeof val);
  zmq_setsockopt(xsub, ZMQ_SNDHWM, &val, sizeof val);
  zmq_setsockopt(xsub, ZMQ_RCVHWM, &val, sizeof val);
  zmq_setsockopt(caps, ZMQ_SNDHWM, &val, sizeof val);
  zmq_setsockopt(caps, ZMQ_RCVHWM, &val, sizeof val);
#endif

  RC(zmq_bind(xsub, "tcp://*:7770")); /* publishers connect here */
  RC(zmq_bind(xpub, "tcp://*:7771")); /* subscribers connect here */
  RC(zmq_bind(reps, "tcp://*:7772")); /* req/resp to synchronize(?) */
  RC(zmq_bind(caps, "inproc://cap"));

  pthread_t mt;
  RC(zmq_socket_monitor (xpub, "inproc://monitor", ZMQ_EVENT_ALL));
  pthread_create (&mt, NULL, rep_socket_monitor, cx);

  pthread_t rt, hbt, ct;
  pthread_create(&rt, 0, reply, reps);
  pthread_create(&ct, 0, capture, cx);

  RC(zmq_proxy(xsub, xpub, caps));
}



#if 1
struct evt {
  uint16_t id;
  uint32_t value;
};

int read_msg(void* s, struct evt* e, char* ep) {
  char buf[1024];
  int n = zmq_recv(s, buf, sizeof buf, 0);
  memcpy(&(e->id), buf, sizeof(e->id));
  memcpy(&(e->value), buf+sizeof(e->id), sizeof(e->value));

  n = zmq_recv(s, buf, sizeof buf, 0);
  memcpy(ep, buf, n);
  ep[n] = 0;
  return 0 ;
}

void *rep_socket_monitor (void *ctx) {
  struct evt e;
  static char addr[1025] ;

  void *s = zmq_socket (ctx, ZMQ_PAIR);

  RC(zmq_connect (s, "inproc://monitor"));
  while (!read_msg(s, &e, addr)) {
    switch (e.id) {
    case ZMQ_EVENT_LISTENING:
      printf ("listening  %d ", e.value);
      printf ("%s\n", addr);
      break;
    case ZMQ_EVENT_ACCEPTED:
      printf ("%s: accepted %d ", timestring(), e.value);
      printf ("%s\n", addr);
      struct sockaddr saddr;
      socklen_t saddrlen;
      getsockname(e.value, &saddr, &saddrlen);
      saddr.sa_data[saddrlen-1]=0;
      printf("sock %s\n", saddr.sa_data);
      break;
    case ZMQ_EVENT_CLOSE_FAILED:
      printf ("%s: close failure %d ", timestring(), e.value);
      printf ("%s\n", addr);
      break;
    case ZMQ_EVENT_CLOSED:
      printf ("%s: closed %d ", timestring(), e.value);
      printf ("%s\n", addr);
      break;
    case ZMQ_EVENT_DISCONNECTED:
      printf ("%s: disconnected %d ", timestring(), e.value);
      printf ("%s\n", addr);
      break;
    }
  }
  zmq_close (s);
  return NULL;
}
#endif


