
all:
	gcc -Wall -fPIC -shared -o libawrap.so exec.c
	gcc -ldl -o test main.c

clean:
	rm -f libawrap.so test
