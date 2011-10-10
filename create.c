#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int black(char **t, int n, int i, int j)
{
	if (i < 0 || i >= n || j < 0 || j >= n)
		return 1;
	return t[i][j] == '#';
}

int alone(char **t, int n, int i, int j)
{
	return black(t, n, i-1, j) &&
	       black(t, n, i+1, j) &&
	       black(t, n, i, j-1) &&
	       black(t, n, i, j+1);
}

void swap(int a[], int i, int j)
{
	int t = a[i];
	a[i] = a[j];
	a[j] = t;
}

int main(int argc, char **argv)
{
	int *a, i, j, n, m;
	char **t;
	if (argc != 3)
		return EXIT_FAILURE;
	n = atoi(argv[1]);
	m = atoi(argv[2]);
	if (n <= 0 || m > n*n || m < 0)
		return EXIT_FAILURE;
	srand((unsigned int)time(NULL));
	a = malloc(n*n*sizeof(int));
	t = malloc(n*sizeof(char*));
	for (i = 0 ; i != n ; ++i) {
		t[i] = malloc(n*sizeof(char));
		for (j = 0 ; j != n ; ++j)
			t[i][j] = ' ';
	}
	for (i = 0 ; i != n*n ; ++i)
		a[i] = i;
	printf("%d\n", n);
	for (i = 0 ; i != m ; ++i) {
		swap(a, i, rand()%(n*n-i)+i);
		t[a[i]/n][a[i]%n] = '#';
		printf("%d %d\n", a[i]/n+1, a[i]%n+1);
	}
	for (i = 0 ; i != n ; ++i)
		for (j = 0 ; j != n ; ++j)
			if (alone(t, n, i, j))
				printf("%d %d\n", i+1, j+1);
	for (i = 0 ; i != n ; ++i)
		free(t[i]);
	free(t);
	free(a);
	return EXIT_SUCCESS;
}
