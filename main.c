#include "vma.h"

int main(void)
{
	char command[1000], copy[1000], *token, *data;
	arena_t *arena = NULL;
	uint64_t size, address;
	int8_t *perm = NULL;

	while (1) {
		fgets(command, sizeof(command), stdin);
		int flag = get_command(command);
		switch (flag) {
		case 1:
			size = atol(command + 12);
			arena = alloc_arena(size);
			break;

		case 2:
			dealloc_arena(arena);
			break;

		case 3:
			token = strtok(command, " ");
			token = strtok(NULL, " ");
			address = atol(token);
			token = strtok(NULL, " ");
			size = atol(token);
			alloc_block(arena, address, size);
			break;

		case 4:
			token = strtok(command, " ");
			token = strtok(NULL, " ");
			address = atol(token);
			free_block(arena, address);
			break;

		case 5:
			token = strtok(command, " ");
			token = strtok(NULL, " ");
			address = atol(token);
			token = strtok(NULL, " ");
			size = atol(token);
			read(arena, address, size);
			break;

		case 6:
			strcpy(copy, command);
			token = strtok(command, " ");
			token = strtok(NULL, " ");
			address = atol(token);
			token = strtok(NULL, " ");
			size = atol(token);
			data = get_data(copy, size);
			write(arena, address, size, data);
			break;

		case 7:
			pmap(arena);
			break;

		case 8:
			perm = calloc(1, sizeof(int8_t));
			get_perm(command, perm);
			token = strtok(command, " ");
			token = strtok(NULL, " ");
			address = atol(token);
			mprotect(arena, address, perm);
			free(perm);
			break;
		}
	}

	return 0;
}
