all: folders server client

server: bin/monitor

client: bin/tracer

folders:
	@mkdir -p src obj bin tmp

bin/monitor: obj/monitor.o
	gcc -g obj/monitor.o -o bin/monitor

obj/monitor.o: src/monitor.c inc/execute.h inc/pipeline.h inc/prog.h inc/stats.h inc/status.h inc/tokenize.h
	gcc -Wall -g -c -Iinc src/monitor.c -o obj/monitor.o

bin/tracer: obj/tracer.o
	gcc -g obj/tracer.o -o bin/tracer

-o obj/tracer.o: src/tracer.c inc/execute.h inc/pipeline.h inc/prog.h inc/stats.h inc/status.h inc/tokenize.h
	gcc -Wall -g -c -Iinc src/tracer.c -o obj/tracer.o

clean:
	rm -f obj/* tmp/* bin/{tracer,monitor}
