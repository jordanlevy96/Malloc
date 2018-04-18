main:
	gcc -Wall -g -fpic malloc.c -o malloc
	./malloc

send:
	scp malloc.c jlevy07@unix3.csc.calpoly.edu:cpe453
