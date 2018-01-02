all:
	newclang -fPIC -shared -O3 shared.c -o shared.so
	newclang -O3 test.c -o test ./shared.so

clean:
	rm shared.so
	rm test
