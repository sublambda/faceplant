#include <iostream>
#include <unistd.h>
#include <zmq.h>
#include <sys/time.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/objdetect.hpp>
#if PICAM
#include <raspicam/raspicam_cv.h>
#endif
#include "defs.h"

using namespace cv;

#define W 1280
#define H 720
#define SF 4

//#define W 1920
//#define H 1080
//#define SF 8

//#define W 640
//#define H 480
//#define SF 2

long millis() {
  struct timeval tv;
  gettimeofday(&tv,0);
  return (long) (tv.tv_sec * 1000) + (long) (tv.tv_usec / 1000);
}       

CascadeClassifier faceclass, bodyclass;

void* pubsock;

void sendimage(Mat im) {
  std::vector<uchar> buf;
  imencode(".jpg", im, buf);
  zmq_send(pubsock, buf.data(), buf.size(), 0);
}

void detect(CascadeClassifier cc, Mat im, int min_neighbors = 2) {
  Mat dim;
  float f = SF;
  std::vector<Rect> faces;

  resize(im, dim, Size(), 1/f, 1/f, INTER_LINEAR);
  cvtColor(dim, dim, COLOR_BGR2GRAY);
  equalizeHist(dim, dim);

  cc.detectMultiScale(dim, faces, 1.1, min_neighbors);

  if (faces.size())
    fprintf(stderr,"[%lu]", faces.size());
  else
    return;

  for (int i = 0; i < faces.size(); i++) {
    Rect r = faces[i];
    Rect fr(r.x * f, r.y * f, r.width * f, r.height * f);
    rectangle(im, fr, Scalar(0,150,255), 2);
    // paranoia
    if (fr.x < 0 || fr.y < 0 || fr.x + fr.width >= im.cols || fr.y + fr.height >= im.rows) {
      fprintf(stderr, "derp %d %d %d %d\n", fr.x, fr.y, fr.width, fr.height);
      continue;
    }
    im = im(fr);                // crop

    if (im.cols > 120) {
      float scale = 120.0 / im.cols;
      resize(im, im, Size(), scale, scale);
    }
    sendimage(im);
  }
}

int main(int argc, char* argv[])
{
  Mat im; 
#if PICAM
  raspicam::RaspiCam_Cv vc;
#else
  VideoCapture vc;
#endif
  int dev = argc > 1 ? atoi(argv[1]) : 0;

  void* ctx = zmq_ctx_new();
  void* pub = zmq_socket(ctx, ZMQ_PUB);
  pubsock = pub;

  char buf[100];
  sprintf(buf, "tcp://*:555%d", dev);
  RC(zmq_bind(pub, buf));

  const char* relayhost = argc > 2 ? argv[2] : getenv("FACERELAY");
  if (relayhost != NULL) {
    sprintf(buf, "tcp://%s:7770", relayhost);
    fprintf(stderr, "relay %s\n", buf);
    RC(zmq_connect(pub, buf));
  }

  RC(bodyclass.load("haarcascade_upperbody.xml"));
  RC(faceclass.load("haarcascade_frontalface_alt.xml"));

#if PICAM
  RC(vc.open());
#else
  RC(vc.open(dev));
#endif

  //RC(vc.set(CAP_PROP_FOURCC,CV_FOURCC('H','2','6','4')));

  RC(vc.set(CAP_PROP_FPS , 10.0));
  printf("fps=%f\n", vc.get(CAP_PROP_FPS));
  RC(vc.set(CAP_PROP_FRAME_WIDTH, W));
  RC(vc.set(CAP_PROP_FRAME_HEIGHT, H));

  int w = 0, h = 0;
  long t, pt = millis(), ts = millis();

  while (1) {
    Mat im, imx;
    t = millis();

    if (t - ts < 100)
      continue;
    ts = t;

    vc.grab();
    vc.retrieve(im);

    // last argument, min_neighbors, controls threshold of detector
    // values 0-4, higher is stricter
    detect(bodyclass, im, 3);
    detect(faceclass, im, 1);

#if 0
    if (t - pt > 20000) {
      float scale = 150.0 / im.cols;
      resize(im, im, Size(), scale, scale);
      sendimage(im);
      pt = t;
    }
#endif

    fprintf(stderr,".");
  }
}

