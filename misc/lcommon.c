#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *lcommon(char *s, char *t) {
        int strlen1 = strlen(s);
        int strlen2 = strlen(t);
        int len = (strlen1 < strlen2) ? strlen2 : strlen1;
        int i, j, k;
        int longest = 0;
        int **ptr = (int **)malloc(2 * sizeof(int *));
        static int *ret;
        /*
         * Maximum length of the return list (considering intermediate steps).
         * It is the maximum length of the source strings + 1 (worst-case
         * intermediate length) + the value of the longest match + the
         * terminator value (-1).
         */
        ret = (int *)malloc((len + 3) * sizeof(int));
        for (i = 0; i < 2; i++)
                ptr[i] = (int *)calloc(strlen2, sizeof(int));

        ret[1] = -1;
        for (i = 0; i < strlen1; i++) {
                memcpy(ptr[0], ptr[1], strlen2 * sizeof(int));
                for (j = 0; j < strlen2; j++) {
                        if (s[i] == t[j]) {
                                if (i == 0 || j == 0) {
                                        ptr[1][j] = 1;
                                } else {
                                        ptr[1][j] = ptr[0][j-1] + 1;
                                }
                                if (ptr[1][j] > longest) {
                                        longest = ptr[1][j];
                                        k = 1;
                                }
                                if (ptr[1][j] == longest) {
                                        ret[k++] = i;
                                        ret[k] = -1;
                                }
                        } else {
                                ptr[1][j] = 0;
                        }
                }
        }
        for (i = 0; i < 2; i++)
                free(ptr[i]);
        free(ptr);
        /* store the maximum length in ret[0] */
        ret[0] = longest;
        return ret;
}

int main(int argc, char *argv[]) {
        int i, longest, *ret;

        if (argc != 3) {
                printf("usage: longest-common-substring string1 string2\n");
                exit(1);
        }

        ret = lcommon(argv[1], argv[2]);
        if ((longest = ret[0]) == 0) {
                printf("There is no common substring\n");
                exit(2);
        }

        i = 0;
        while (ret[++i] != -1) {
                printf("%.*s\n", longest, &argv[1][ret[i]-longest+1]);
        }
        free(ret);

        exit(0);
}

