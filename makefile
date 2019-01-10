mydhcpd: mydhcpd.o
		gcc -o mydhcpd mydhcpd.o
mydhcpd.o: mydhcpd.c
		gcc -c mydhcpd.c
mydhcpc: mydhcpc.o
		gcc -o mydhcpc mydhcpc.o
mydhcpc.o: mydhcpc.c
		gcc -c mydhcpc.c
clean:
		rm -f mydhcpd mydhcpc *.o
