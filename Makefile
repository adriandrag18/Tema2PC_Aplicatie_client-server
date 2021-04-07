CFLAGS = -Wall -g
PORT = 8080
all:
	gcc $(CFLAGS) server.c helpers.c struct.c -o server
	gcc $(CFLAGS) subscriber.c helpers.c -o subscriber
run_server:
	./server $(PORT)
run_subscriber:
	./subscriber cli 127.0.0.1 $(PORT)
run_subscriber2:
	./subscriber cli2 127.0.0.1 $(PORT)
run_subscriber3:
	./subscriber cli3 127.0.0.1 $(PORT)
manual:
	python3 udp_client.py --source-port 1234 --input_file three_topics_payloads.json --mode manual 127.0.0.1 $(PORT)
once:
	python3 udp_client.py --source-port 1234 --input_file three_topics_payloads.json --mode all_once 127.0.0.1 $(PORT)
random:
	python3 udp_client.py --source-port 1234 --input_file three_topics_payloads.json --mode random --count 500 127.0.0.1 $(PORT)
clean:
	rm -f server subscriber client