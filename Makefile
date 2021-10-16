build:
	gcc -o JandM_par JandM_par.c -lpthread -lm -Wall 
clean:
	rm -rf JandM_par
