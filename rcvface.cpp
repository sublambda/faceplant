#include <zmq.h>
#include <sys/time.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "defs.h"

using namespace cv;

long millis() {
  struct timeval tv;
  gettimeofday(&tv,0);
  return (long) (tv.tv_usec * 1000) + (long) (tv.tv_usec / 1000);
}       

#define W 1000
#define H 700

void curve(void* sub) {
  static char pubkey [41];
  static char secret [41];
  RC(zmq_curve_keypair(pubkey, secret));

  FILE* fp = fopen("client.pubkey", "w");
  fwrite(pubkey, 1, sizeof pubkey, fp);
  fclose(fp);
  fp = fopen("client.secret", "w");
  fwrite(pubkey, 1, sizeof secret, fp);
  fclose(fp);

  RC(zmq_setsockopt (sub, ZMQ_CURVE_PUBLICKEY, pubkey, 41));
  RC(zmq_setsockopt (sub, ZMQ_CURVE_SECRETKEY, secret, 41));
}

int main(int argc, char* argv[])
{
  Mat target = Mat(Size(W,H), CV_8UC3, Scalar(100,100,100));
  imshow("face", target);

  void* ctx = zmq_ctx_new();
  void* sub = zmq_socket(ctx, ZMQ_SUB);

  const char* host = argc > 1 ? argv[1] : "black-pearl";
  const char* proxy = argc > 2 ? argv[2] : NULL;
  char buf[100];

  if (proxy) {
    sprintf(buf, "tcp://%s:7771", proxy);
    printf("connect %s\n", buf);
    RC(zmq_connect(sub, buf));
  }

  for (int i = 0; i < 4; i++) {
    sprintf(buf, "tcp://%s:555%d", host, i);
    printf("connect %s\n", buf);
    RC(zmq_connect(sub, buf));
  }

  RC(zmq_setsockopt(sub, ZMQ_SUBSCRIBE, "", 0));
  //curve(sub);

  int x=0, y=0, w=0, h=0;

  for (;;) {
    cv::waitKey(33);
    std::vector<uchar> buf(1000000);

    int n = zmq_recv(sub, buf.data(), buf.max_size(), 0);

    if (n > 0) {
      buf.resize(n);
      Mat im = imdecode(buf,1);
      w = im.size().width;
      h = im.size().height;

      if (w >= W || h >= H)
        continue;

      fprintf(stderr, "%d %d %d %d\n", x, y, w, h);
      if (x + w + 10 >= target.cols) {
        x = 0;
        y += h + 10;
      }
      if (y + h + 10 >= target.rows) {
        y = 0;
      }

      im.copyTo(target(Rect(x,y,w,h)));
      imshow("face", target);
      x += w;
    }
  }
}
