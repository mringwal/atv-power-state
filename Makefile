all: atv-listener

atv-listener: atv.c atv-listener.c
	gcc -limobiledevice atv.c atv-listener.c -o atv-listener

clean:
	rm atv-listener
