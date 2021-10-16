#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define min(a, b) ((a) < (b) ? (a) : (b)) // funtia minim

// structura pentru un numar complex
typedef struct _complex {
	double a;
	double b;
} complex;

// structura pentru parametrii unei rulari
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
    int width;
    int height;
} params;

// numele fisierelor de intrare, iesite
char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;
int P; // nr thred-uri
params par; // paramterii
pthread_barrier_t barrier; // bariera
int **result; // matricea

// citeste argumentele programului{
void get_args(int argc, char **argv) {
	if (argc < 6) { // mai putini parametri
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot P\n");
		exit(1);
	}
	// atribui parametrii
	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
    P = atoi(argv[5]);
}

// citeste fisierul de intrare
void read_input_file(char *in_filename, params* par) {
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}
	fscanf(file, "%d", &(par->is_julia));
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf %d", &par->resolution, &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}
    par->width = (par->x_max - par->x_min) / par->resolution;
	par->height = (par->y_max - par->y_min) / par->resolution;

	fclose(file);
}

// scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename) {
	int i, j;
	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", par.width, par.height);
	for (i = 0; i < par.height; i++) {
		for (j = 0; j < par.width; j++) {
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

// aloca memorie pentru rezultat
void allocate_memory(int width, int height, int id) {
	int i, start, end;
	if (id == 0) {
		result = NULL;
		result = malloc(height * sizeof(int*));
	}
	pthread_barrier_wait(&barrier);
	if (result == NULL) {
		printf("Eroare la malloc!\n");
		exit(1);
	}
	start = id * (double)height / P;
	end = min((id + 1) * (double)height / P, height);
	for (i = start; i < end; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}
}

// elibereaza memoria alocata
void free_memory(int **result, int height, int id) {
	int i, start, end;
	start = id * (double)height / P;
	end = min((id + 1) * (double)height / P, height);
	for (i = start; i < end; i++) {
		free(result[i]);
	}
	pthread_barrier_wait(&barrier);
	if (id == 0)
		free(result);
}

// transforma rezultatul din coordonate matematice in coordonate ecran
void cord_mat_to_cord_ecran(int id,  int len) {
    int i, start, end, *aux;
	start = id * (double)(len / 2) / P;
	end = min((id + 1) * (double)(len / 2) / P, (len / 2));

	for (i = start; i < end; i++) {
		aux = result[i];
		result[i] = result[len - i - 1];
		result[len - i - 1] = aux;
	}
}

// ruleaza algoritmul Julia
void run_julia(params *par, int id) {
	
	int w, h, wstart, wend, hstart, hend, step;
	
	// paralelizez valoarea mai mare intre width si height
	if (par->width > par->height) {
		wstart = id * (double)par->width / P;
		wend = min((id + 1) * (double)par->width / P, par->width);
		hstart = 0;
		hend = par->height;
	} else {
		hstart = id * (double)par->height / P;
		hend = min((id + 1) * (double)par->height / P, par->height);
		wstart = 0;
		wend = par->width;
	}
	// calculeaza multimea julia
	for (w = wstart; w < wend; w++) {
		for (h = hstart; h < hend; h++) {
			step = 0;
			complex z = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };
			
			while (z.a * z.a + z.b * z.b < 4.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = z_aux.a * z_aux.a - z_aux.b * z_aux.b + par->c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par->c_julia.b;
				step++;
			}
			result[h][w] = step % 256;
		}
	}
	pthread_barrier_wait(&barrier);
	// transforma coordonatele matricei in coordonate ecran
	cord_mat_to_cord_ecran(id, par->height);

}

// ruleaza algoritmul Mandelbrot
void run_mandelbrot(params *par, int id) {
	int w, h, wstart, wend, hstart, hend, step;
	
	// paralelizez valoarea mai mare intre width si height
	if (par->width > par->height) {
		wstart = id * (double)par->width / P;
		wend = min((id + 1) * (double)par->width / P, par->width);
		hstart = 0;
		hend = par->height;
	} else {
		hstart = id * (double)par->height / P;
		hend = min((id + 1) * (double)par->height / P, par->height);
		wstart = 0;
		wend = par->width;
	}
	// calcueaza multimea mandelbrot
	for (w = wstart; w < wend; w++) {
		for (h = hstart; h < hend; h++) {
			complex c = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };
			complex z = { .a = 0, .b = 0 };
			step = 0;

			while (z.a * z.a + z.b * z.b < 4.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = z_aux.a * z_aux.a - z_aux.b * z_aux.b + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;
				step++;
			}
			result[h][w] = step % 256;
		}
	}

	pthread_barrier_wait(&barrier);
	// transforma coordonatele matricei in coordonate ecran
	cord_mat_to_cord_ecran(id, par->height);

}

void *thread_function(void *arg) {

    int id = *(int *)arg;
    if (id == 0) { // citeste input-l din fiserul julia
        read_input_file(in_filename_julia, &par);
    }
	pthread_barrier_wait(&barrier);
	allocate_memory(par.width, par.height, id); // aloca rezultatul
    pthread_barrier_wait(&barrier);
    run_julia(&par, id); // calculeaza multimea julia
    pthread_barrier_wait(&barrier);
    if (id == 0) { // scrie multimea julia
        write_output_file(out_filename_julia);
    }
    pthread_barrier_wait(&barrier);
	free_memory(result, par.height, id); // dezaloca rezultatul
    pthread_barrier_wait(&barrier);

    if (id == 0) { // citeste input-l din fiserul mandelbrot
        read_input_file(in_filename_mandelbrot, &par);
    }
	pthread_barrier_wait(&barrier);
	allocate_memory(par.width, par.height, id); // aloca rezultatul
    pthread_barrier_wait(&barrier);
    run_mandelbrot(&par, id); // paralel treb
    pthread_barrier_wait(&barrier);
    if (id == 0) { // scrie multimea mandelbrot
        write_output_file(out_filename_mandelbrot);
    }
	pthread_barrier_wait(&barrier);
	free_memory(result, par.height, id); // dezaloca rezultatul

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    
	// se citesc argumentele programului
	get_args(argc, argv);

    pthread_t tid[P];
	int thread_id[P];
	int i;
	pthread_barrier_init(&barrier, NULL, P);
	
	for (i = 0 ; i < P; i++) { // se creeaza thred-urile
        thread_id[i] = i;
		pthread_create(&tid[i], NULL, thread_function, &thread_id[i]);
    }
    for (i = 0; i < P; i++) { // se asteapta thred-urile
		pthread_join(tid[i], NULL);
	}

    pthread_barrier_destroy(&barrier);
	return 0;
}
