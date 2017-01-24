
CV=/usr/local/opencv3

# for brew:
#CV = /usr/local/opt/opencv3

CFLAGS = -O

PILIBS = -L $(CV)/lib -L /opt/vc/lib -L /usr/local/lib \
	-lraspicam_cv -lraspicam -lmmal -lmmal_core -lmmal_util

LIBS = -L $(CV)/lib -lopencv_highgui -lopencv_core -lopencv_objdetect \
	-lopencv_video -lopencv_videoio -lopencv_imgcodecs -lopencv_imgproc -lzmq

all: face rcvface zproxy piface

face: face.cpp
	g++ $(CFLAGS) -o $@ $< -I$(CV)/include -L$(CV)/lib $(LIBS)

piface: face.cpp
	g++ -DPICAM=1 $(CFLAGS) -o $@ $< -I$(CV)/include -L$(CV)/lib $(PILIBS) $(LIBS)

rcvface: rcvface.cpp
	g++ $(CFLAGS) -o $@ $< -I$(CV)/include -L$(CV)/lib $(LIBS)

zproxy: zproxy.cpp
	g++ $(CFLAGS) -o $@ $< -lzmq -lpthread

clean:
	rm -f face piface rcvface zproxy
