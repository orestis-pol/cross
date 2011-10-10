#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>


struct dict {
	int all_words_num;
	char **all_words;
	int max_len;
	char ***words_len;
	int *words_len_num;
};

void free_dict(struct dict w)
{
	int word, len;
	for (word = 0 ; word != w.all_words_num ; ++word)
		free(w.all_words[word]);
	free(w.all_words);
	for (len = 0 ; len <= w.max_len ; ++len)
		free(w.words_len[len]);
	free(w.words_len);
	free(w.words_len_num);
}

int read_dict(FILE *fp, struct dict *r)
{
	int cap, word, len, *temp_words_len_num;
	char buf[32];
	struct dict w;
	/* Read words */
	w.all_words_num = 0;
	cap = 16;
	w.all_words = (char**)malloc(cap*sizeof(char*));
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		buf[strlen(buf)-1] = 0;
		for (len = 0 ; buf[len] ; ++len)
			if (buf[len] < 'a' || buf[len] > 'z')
				return -1;
		if (w.all_words_num == cap) {
			cap += cap;
			w.all_words = (char**)realloc(w.all_words, cap*sizeof(char*));
		}
		w.all_words[w.all_words_num] = (char*)malloc((strlen(buf)+1)*sizeof(char));
		strcpy(w.all_words[w.all_words_num++], buf);
	}
	w.all_words = (char**)realloc(w.all_words, w.all_words_num*sizeof(char*));
	/* Find longest word */
	w.max_len = 0;
	for (word = 0 ; word != w.all_words_num ; ++word) {
		len = strlen(w.all_words[word]);
		if (len > w.max_len)
			w.max_len = len;
	}
	/* Seperate by lengths */
	w.words_len_num = (int*)malloc((w.max_len+1)*sizeof(int));
	for (len = 0 ; len <= w.max_len ; ++len)
		w.words_len_num[len] = 0;
	for (word = 0 ; word != w.all_words_num ; ++word)
		w.words_len_num[strlen(w.all_words[word])]++;
	w.words_len = (char***)malloc((w.max_len+1)*sizeof(char**));
	for (len = 0 ; len <= w.max_len ; ++len)
		if (w.words_len_num[len])
			w.words_len[len] = (char**)malloc(w.words_len_num[len]*sizeof(char*));
	temp_words_len_num = (int*)malloc((w.max_len+1)*sizeof(int));
	for (len = 0 ; len <= w.max_len ; ++len)
		temp_words_len_num[len] = 0;
	for (word = 0 ; word != w.all_words_num ; ++word) {
		len = strlen(w.all_words[word]);
		w.words_len[len][temp_words_len_num[len]++] = w.all_words[word];
	}
	free(temp_words_len_num);
	*r = w;
	return 0;
}

struct cross {
	char **table;
	int height;
	int width;
};

void free_cross(struct cross c)
{
	int i;
	for (i = 0 ; i != c.height ; ++i)
		free(c.table[i]);
	free(c.table);
}

int read_cross(FILE *fp, struct cross *r)
{
	struct cross m;
	char c, buf[32];
	int i, j;
	/* Read height & width */
	if (fgets(buf, sizeof(buf), fp) == NULL)
		return -1;
	if (sscanf(buf, "%d %d", &m.height, &m.width) != 2) {
		if (sscanf(buf, "%d", &m.height) != 1)
			return -1;
		m.width = m.height;
	}
	if (m.height <= 0 || m.width <= 0)
		return -1;
	m.table = (char**)malloc(m.height*sizeof(char*));
	for (i = 0 ; i != m.height ; ++i) {
		m.table[i] = (char*)malloc(m.width*sizeof(char));
		for (j = 0 ; j != m.width ; ++j)
			m.table[i][j] = ' ';
	}
	/* Read dots */
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		i = 0;
		j = 0;
		c = '#';
		sscanf(buf, "%d %d %c", &i, &j, &c);
		if (i < 1 || j < 1 || i > m.height || j > m.width ||
		   ((c < 'a' || c > 'z') && c != '#') ||
		   (m.table[i - 1][j - 1] != ' ' && m.table[i - 1][j - 1] != c))
			return -1;
		m.table[i - 1][j - 1] = c;
	}
	*r = m;
	return 0;
}

struct move {
	int start_i;
	int start_j;
	int len;
	int hor;
	int size;
	char **domain;
	int *conflict;
	int active;
	int *cut;
	int val;
};

void free_moves(struct move *m, int n)
{
	int mov;
	for (mov = 0 ; mov != n ; ++mov) {
		free(m[mov].domain);
		free(m[mov].conflict);
		free(m[mov].cut);
	}
	free(m);
}

void copy_moves(struct move *o, int n, struct move **r)
{
	int mov;
	struct move *m = (struct move*)malloc(n*sizeof(struct move));
	for (mov = 0 ; mov != n ; ++mov) {
		m[mov] = o[mov];
		m[mov].domain = (char**)malloc(m[mov].size*sizeof(char*));
		memcpy(m[mov].domain, o[mov].domain, m[mov].size*sizeof(char*));
		m[mov].conflict = (int*)malloc(m[mov].len*sizeof(int));
		memcpy(m[mov].conflict, o[mov].conflict, m[mov].len*sizeof(int));
		m[mov].cut = (int*)malloc(m[mov].len*sizeof(int));
	}
	*r = m;
}

int read_moves(struct cross c, struct dict d, struct move **r)
{
	struct move *m;
	int num, cap, prev, mov, word, len, i, j, l, **temp_table;
	/* Count and allocate moves */
	num = 0;
	cap = 16;
	m = (struct move*)malloc(cap*sizeof(struct move));
	for (i = 0 ; i != c.height ; ++i) {
		prev = -1;
		for (j = 0 ; j <= c.width ; ++j)
			if (j == c.width || c.table[i][j] == '#') {
				if (j - prev > 2) {
					if (num == cap) {
						cap += cap;
						m = (struct move*)realloc(m,
					             cap*sizeof(struct move));
					}
					m[num].start_i = i;
					m[num].start_j = prev + 1;
					m[num].len = j - prev - 1;
					m[num++].hor = 1;
				}
				prev = j;
			}
	}
	for (j = 0 ; j != c.width ; ++j) {
		prev = -1;
		for (i = 0 ; i <= c.height ; ++i)
			if (i == c.height || c.table[i][j] == '#') {
				if (i - prev > 2) {
					if (num == cap) {
						cap += cap;
						m = (struct move*)realloc(m,
						     cap*sizeof(struct move));
					}
					m[num].start_j = j;
					m[num].start_i = prev + 1;
					m[num].len = i - prev - 1;
					m[num++].hor = 0;
				}
				prev = i;
			}
	}
	m = (struct move*)realloc(m, num*sizeof(struct move));
	/* Copy domains with words fitting filled letter */
	for (mov = 0 ; mov != num ; ++mov) {
		cap = 16;
		len = m[mov].len;
		m[mov].size = 0;
		m[mov].domain = (char**)malloc(cap * sizeof(char*));
		if (len > d.max_len)
			return -1;
		for (word = 0 ; word != d.words_len_num[len] ; ++word) {
			i = m[mov].start_i;
			j = m[mov].start_j;
			if (m[mov].hor) {
				for (l = 0 ; l != len ; ++l)
					if (c.table[i][j + l] != ' ' &&
					    c.table[i][j + l] != d.words_len[len][word][l])
						break;
			} else {
				for (l = 0 ; l != len ; ++l)
					if (c.table[i + l][j] != ' ' &&
					    c.table[i + l][j] != d.words_len[len][word][l])
						break;
			}
			if (l != len)
				continue;
			if (m[mov].size == cap) {
				cap += cap;
				m[mov].domain = (char**)realloc(m[mov].domain, cap * sizeof(char*));
			}
			m[mov].domain[m[mov].size++] = d.words_len[len][word];
		}
		m[mov].domain = (char**)realloc(m[mov].domain, m[mov].size * sizeof(char*));
		m[mov].cut = (int*)malloc(len*sizeof(int));
		if (m[mov].size == 0)
			return -1;
	}
	/* Calculate conflicts */
	temp_table = (int**)malloc(c.height*sizeof(int*));
	for (i = 0 ; i != c.height ; ++i) {
		temp_table[i] = (int*)malloc(c.width*sizeof(int));
		for (j = 0 ; j != c.width ; ++j)
			temp_table[i][j] = -1;
	}
	for (mov = 0 ; mov != num ; ++mov)
		if (m[mov].hor)
			for (j = m[mov].start_j ; j != m[mov].start_j + m[mov].len ; ++j)
				temp_table[m[mov].start_i][j] = mov;
	for (mov = 0 ; mov != num ; ++mov)
		if (!m[mov].hor) {
			m[mov].conflict = (int*)malloc(m[mov].len*sizeof(int));
			for (i = 0 ; i != m[mov].len ; ++i)
				m[mov].conflict[i] = temp_table[m[mov].start_i + i]
				                               [m[mov].start_j];
		}
	for (i = 0 ; i != c.height ; ++i)
		for (j = 0 ; j != c.width ; ++j)
			temp_table[i][j] = -1;
	for (mov = 0 ; mov != num ; ++mov)
		if (!m[mov].hor)
			for (i = m[mov].start_i ; i != m[mov].start_i + m[mov].len ; ++i)
				temp_table[i][m[mov].start_j] = mov;
	for (mov = 0 ; mov != num ; ++mov)
		if (m[mov].hor) {
			m[mov].conflict = (int*)malloc(m[mov].len*sizeof(int));
			for (j = 0 ; j != m[mov].len ; ++j)
				m[mov].conflict[j] = temp_table[m[mov].start_i]
				                               [m[mov].start_j + j];
		}
	for (i = 0 ; i != c.height ; ++i)
		free(temp_table[i]);
	free(temp_table);
	*r = m;
	return num;
}

void print_cross(struct cross c, struct move *m, int n)
{
	int i, j, mov;
	/* Fill crossword */
	for (mov = 0 ; mov != n ; ++mov)
		if (!m[mov].hor && m[mov].val >= 0)
			for (i = 0 ; i != m[mov].len ; ++i)
				c.table[m[mov].start_i + i][m[mov].start_j] = 
				    m[mov].domain[m[mov].val][i];
		else if (m[mov].hor && m[mov].val >= 0)
			for (j = 0 ; j != m[mov].len ; ++j)
				c.table[m[mov].start_i][m[mov].start_j + j] =
				    m[mov].domain[m[mov].val][j];
	/* Print crossword */
	for (i = 0 ; i != c.height ; ++i) {
		for (j = 0 ; j != c.width ; ++j)
			if (c.table[i][j] == '#')
				printf("###");
			else
				printf(" %c ", c.table[i][j]);
		printf("\n");
	}
}

void swap_s(char **a, int i, int j)
{
	char *t = a[i];
	a[i] = a[j];
	a[j] = t;
}

void swap_i(int *a, int i, int j)
{
	int t = a[i];
	a[i] = a[j];
	a[j] = t;
}

long solve(struct move *m, int n, long blim, int vlim, int *rem, unsigned int *seed)
{
	int mov, pos, word, ncut, min;
	int depth = 0, add = 1;
	long nbt = 0l;
	/* Initialize variables */
	for (mov = 0 ; mov != n ; ++mov) {
		rem[mov] = mov;
		m[mov].active = m[mov].size;
		m[mov].val = -1;
	}
	/* Backtracking search iteration */
	do {
		/* Pick move */
		if (!add) {
			min = rem[depth];
			for (ncut = 0 ; ncut != m[min].len ; ++ncut)
				m[m[min].conflict[ncut]].active += m[min].cut[ncut];
		} else {
			min = m[rem[depth]].active;
			pos = 1;
			for (mov = depth+1 ; mov != n ; ++mov)
				if (m[rem[mov]].active == min)
					pos++;
				else if (m[rem[mov]].active < min) {
					min = m[rem[mov]].active;
					pos = 1;
				}
			pos = rand_r(seed) % pos;
			for (mov = depth ;; ++mov)
				if (m[rem[mov]].active == min && !pos--)
					break;
			swap_i(rem, mov, depth);
			min = rem[depth];
		}
		/* Assign variable and propagate constraints */
		retry:
		if (++m[min].val == m[min].active || m[min].val == vlim) {
			/* Check upper backtrack limit */
			if (nbt++ == blim)
				return -2;
			/* Backtrack */
			m[min].val = -1;
			depth--;
			add = 0;
		} else {
			/* Pick random value */
			swap_s(m[min].domain, m[min].val,
			       rand_r(seed) % (m[min].active - m[min].val) + m[min].val);
			/* Cut values from conflicting domains */
			for (ncut = 0 ; ncut != m[min].len ; ++ncut) {
				mov = m[min].conflict[ncut];
				m[min].cut[ncut] = 0;
				if (m[mov].val >= 0)
					continue;
				if (m[min].hor)
					pos = m[min].start_i - m[mov].start_i;
				else
					pos = m[min].start_j - m[mov].start_j;
				for (word = 0 ; word != m[mov].active ;)
					if (m[mov].domain[word][pos] ==
					    m[min].domain[m[min].val][ncut])
						word++;
					else {
						swap_s(m[mov].domain, word, --m[mov].active);
						m[min].cut[ncut]++;
					}
				if (!m[mov].active) {
					for (pos = 0 ; pos <= ncut ; ++pos)
						m[m[min].conflict[pos]].active += m[min].cut[pos];
					goto retry;
				}
			}
			/* Continue to next slot */
			depth++;
			add = 1;
		}
	} while (depth != -1 && depth != n);
	return (depth == n ? nbt : -1l);
}

struct thread_data {
	int pos;
	int num;
	pthread_t *ids;
	sem_t *sem;
	struct move *moves;
	int nmov;
	int *rem;
	long blim;
	int vlim;
	int *win;
};

void *run(void *vp)
{
	int i;
	unsigned int seed;
	struct thread_data *d;
	/* Set asynchronous cancel mode */
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	/* Initialize random number generator seed */
	d = (struct thread_data*)vp;
	seed = (int)time(NULL)+(int)pthread_self();
	/* Wait until all have started */
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	sem_wait(d->sem);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	/* Run until solution is found */
	do {
		i = solve(d->moves, d->nmov, d->blim, d->vlim, d->rem, &seed);
	} while (i == -2 || (i == -1 && d->vlim >= 0));
	/* Return id of terminated thread */
	*(d->win) = d->pos;
	/* Cancel all others and exit */
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	for (i = 0 ; i != d->num ; ++i)
		if (d->pos != i)
			pthread_cancel(d->ids[i]);
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	FILE *fp;
	sem_t sem;
	pthread_t *ids;
	struct dict w;
	struct cross c;
	struct move *m;
	struct thread_data *data;
	int i, n, win, res, tnum = 0;
	char *dictf = NULL, *crossf = NULL;
	/* Thread specific search parameters */
	int blim[8] = {-1, 128, 1024, 8192, -1, 1024, 8129, 65536};
	int vlim[8] = {-1,  -1,   -1,   -1,  3,    3,    3,     3};
	/* Check arguments */
	for (i = 1 ; i != argc ; ++i) {
		if (i+1 == argc)
			goto argerror;
		if (!strcmp(argv[i], "-d")) {
			if (dictf != NULL)
				goto argerror;
			dictf = argv[++i];
		} else if (!strcmp(argv[i], "-c")) {
			if (crossf != NULL)
				goto argerror;
			crossf = argv[++i];
		} else if (!strcmp(argv[i], "-t")) {
			if (tnum)
				goto argerror;
			tnum = ++i;
		} else {
			argerror:
			fprintf(stderr, "%s: Invalid arguments\n", argv[0]);
			return EXIT_FAILURE;
		}
	}
	if (!tnum)
		tnum = sizeof(blim) / sizeof(int);
	else
		tnum = atoi(argv[tnum]);
	if (tnum <= 0 || tnum > (int)(sizeof(blim) / sizeof(int))) {
		fprintf(stderr, "%s: Invalid number of threads\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (dictf == NULL)
		dictf = (char*)"Words.txt";
	if (crossf == NULL)
		crossf = (char*)"Crossword.txt";
	/* Read & load dictionary file */
	fp = fopen(dictf, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s: Invalid dictionary file\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (read_dict(fp, &w)) {
		fprintf(stderr, "%s: Error in dictionary file\n", argv[0]);
		return EXIT_FAILURE;
	}
	fclose(fp);
	/* Read & load crossword file */
	fp = fopen(crossf, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s: Invalid crossword file\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (read_cross(fp, &c)) {
		fprintf(stderr, "%s: Error in crossword file\n", argv[0]);
		return EXIT_FAILURE;
	}
	fclose(fp);
	/* Extract moves from crossword */
	n = read_moves(c, w, &m);
	if (n < 0) res = 0;
	else if (!n) res = 1;
	else {
		/* Initialize & run threads */
		ids = (pthread_t*)malloc(tnum*sizeof(pthread_t));
		data = (struct thread_data*)malloc(tnum*sizeof(struct thread_data));
		sem_init(&sem, 0, 0);
		m[0].val = -1;
		for (i = 0 ; i != tnum ; ++i) {
			data[i].num = tnum;
			data[i].ids = ids;
			data[i].pos = i;
			data[i].sem = &sem;
			data[i].nmov = n;
			data[i].rem = (int*)malloc(n*sizeof(int));
			if (!i) data[i].moves = m;
			else copy_moves(m, n, &(data[i].moves));
			data[i].blim = blim[i];
			data[i].vlim = vlim[i];
			data[i].win = &win;
			if (pthread_create(&ids[i], NULL, run, (void*)&data[i])) {
				tnum = i;
				break;
			}
		}
		for (i = 0 ; i != tnum ; ++i)
			sem_post(&sem);
		/* Wait threads to terminate & clear thread memory */
		for (i = 0 ; i != tnum ; ++i)
			pthread_join(ids[i], NULL);
		sem_destroy(&sem);
		if (!tnum) win = 0;
		m = data[win].moves;
		data[win].moves = NULL;
		for (i = 0 ; i != tnum ; ++i) {
			if (data[i].moves != NULL)
				free_moves(data[i].moves, n);
			free(data[i].rem);
		}
		free(data);
		free(ids);
		res = (m[0].val == -1 ? 0 : 1);
	}
	/* Print result & clear memory */
	if (res) print_cross(c, m, n);
	else printf("No solution\n");
	free_dict(w);
	free_cross(c);
	if (n >= 0) free_moves(m, n);
	return EXIT_SUCCESS;
}
