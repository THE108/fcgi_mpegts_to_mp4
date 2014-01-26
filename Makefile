build:
	gcc -o conv mpegts2mp4.c -std=c99 -I/usr/local/include -L/usr/local/lib -lavformat -lavcodec -lavutil -lfdk-aac -lx264 -ldl -lpthread -lm -lva -lz
