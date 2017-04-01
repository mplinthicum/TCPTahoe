all:
	gcc sender.c helper.c -o sender
	gcc receiver.c helper.c -o receiver

clean:
	rm sender receiver cwnd_output.txt
