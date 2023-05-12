all: folders server client

server: bin/monitor

client: bin/tracer

folders:
	@mkdir -p obj bin tmp

bin/monitor: monitor.c
	gcc -Wall -g src/monitor.c -o bin/monitor

bin/tracer: tracer.c
	gcc -Wall -g src/tracer.c -o bin/tracer

clean:
	rm -f obj/* tmp/* bin/{tracer,monitor}
