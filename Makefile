all: atv-listener

atv-listener: atv.c atv-listener.c
	gcc atv.c atv-listener.c -limobiledevice-1.0 -o atv-listener

clean:
	rm atv-listener
