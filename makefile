
CV=/usr/local/opencv3

# for brew:
CV = /usr/local/opt/opencv3

LIBS = -lopencv_highgui -lopencv_core -lopencv_objdetect \
		-lopencv_video -lopencv_videoio -lopencv_imgcodecs -lopencv_imgproc -lzmq

all: face rcvface zproxy

face: face.cpp
	g++ -o $@ $< -I$(CV)/include -L$(CV)/lib $(LIBS)

rcvface: rcvface.cpp
	g++ -o $@ $< -I$(CV)/include -L$(CV)/lib $(LIBS)

zproxy: zproxy.cpp
	g++ -o $@ $< -lzmq -lpthread

clean:
	rm -f face rcvface zproxy
