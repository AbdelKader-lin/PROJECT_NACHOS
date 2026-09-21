#include "utils.h"
#include <string.h>

int split_tokens(char **tokens, char* cmd, int max_token) {
	int nb_tokens = 0;
	if (max_token <= 0) return -1;

	tokens[nb_tokens++] = strtok(cmd , "/");
	if (tokens[0] == NULL) return 0;

	while (nb_tokens < max_token) {
		char*token = strtok(NULL, "/");
        if (token == NULL) break;
		tokens[nb_tokens++]=token;
	}

	tokens[nb_tokens]=NULL;
	return nb_tokens;
}