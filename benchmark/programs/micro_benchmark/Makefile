all:
	newclang simple.c -O3 -c -o simple.o -lm
	newclang simple.c -O3 -fuse-ld=/home/daniele/llvm/ldorig/ld.lld -o simple -lm
	newclang simple.c -O3 -fuse-ld=/home/daniele/llvm/build/bin/ld.lld -o fixed -lm
	newclang simple.c -O3 -static -o static -lm
	newclang -fuse-ld=/home/daniele/llvm/ldorig/ld.lld -mllvm -enable-stitching -mllvm -enable-stitching-calls -O3 simple.c -o stitched_cb /usr/lib/stitcher.so -lm
	newclang -fuse-ld=/home/daniele/llvm/ldorig/ld.lld -mllvm -enable-stitching -mllvm -enable-trampolined-stitching -O3 simple.c -o stitched_tramp /usr/lib/stitcher.so -lm
	makewritable stitched_cb
	makewritable stitched_tramp

clean:
	rm fixed
	rm simple
	rm stitched_cb
	rm stitched_tramp
	rm static
