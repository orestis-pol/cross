#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv)
{
	FILE *fp;
	int n, i, j;
	char **a;
	if (argc != 2)
		return EXIT_FAILURE;
	fp = fopen(argv[1], "r");
	if (fp == NULL)
		return EXIT_FAILURE;
	fscanf(fp, "%d", &n);
	a = malloc(n*sizeof(char*));
	for (i = 0 ; i != n ; ++i) {
		a[i] = malloc(n*sizeof(char));
		for (j = 0 ; j != n ; ++j)
			a[i][j] = ' ';
	}
	while (fscanf(fp, "%d %d", &i, &j) == 2)
		a[i-1][j-1] = '#';
	fclose(fp);
	for (i = 0 ; i != n ; ++i) {
		for (j = 0 ; j != n ; ++j)
			if (a[i][j] == '#')
				printf("###");
			else
				printf(" %c ", a[i][j]);
		printf("\n");
		free(a[i]);
	}
	free(a);
	return EXIT_FAILURE;
}
