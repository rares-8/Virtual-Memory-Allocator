#include "vma.h"

arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena = malloc(sizeof(arena_t));
	arena->arena_size = size;
	arena->alloc_list = malloc(sizeof(list_t));
	arena->alloc_list->size = 0;
	arena->alloc_list->head = NULL;
	arena->alloc_list->data_size = sizeof(block_t);
	return arena;
}

void add_last(list_t *list, node_t *new_node)
{
	node_t *curr = list->head;
	while (curr->next)
		curr = curr->next;

	curr->next = new_node;
	new_node->prev = curr;
	new_node->next = NULL;
}

node_t *create_node(size_t data_size)
{
	node_t *new_block = malloc(sizeof(node_t));
	new_block->next = NULL;
	new_block->prev = NULL;
	new_block->data = malloc(data_size);
	return new_block;
}

void initialize_block(node_t *node, uint64_t start, uint64_t size)
{
	(*(block_t *)node->data).size = size;
	(*(block_t *)node->data).start_address = start;
	(*(block_t *)(node->data)).miniblock_list = malloc(sizeof(list_t));
	(*(block_t *)(node->data)).miniblock_list->size = 1;
}

void initialize_miniblock(node_t *miniblock, uint64_t address, uint64_t size)
{
	(*(miniblock_t *)(miniblock->data)).start_address = address;
	(*(miniblock_t *)(miniblock->data)).size = size;
	(*(miniblock_t *)(miniblock->data)).rw_buffer = NULL;
	(*(miniblock_t *)(miniblock->data)).perm = 6;
}

void insert_block(list_t *bl_list, node_t *new_block)
{
	node_t *curr = bl_list->head;
	while (curr && (*(block_t *)(new_block->data)).start_address > BLOCK_START)
		curr = curr->next;

	if (bl_list->size == 0) {
		bl_list->head = new_block;
		bl_list->size++;
		return;
	}

	if (!curr) {
		add_last(bl_list, new_block);
	} else {
		if (!curr->prev) {
			new_block->next = bl_list->head;
			bl_list->head->prev = new_block;
			bl_list->head = new_block;
			new_block->prev = NULL;
		} else {
			new_block->next = curr;
			new_block->prev = curr->prev;
			curr->prev->next = new_block;
			new_block->next->prev = new_block;
		}
	}
	bl_list->size++;
}

void dealloc_arena(arena_t *arena)
{
	if (!arena)
		exit(0);

	list_t *bl_list = arena->alloc_list;
	node_t *curr_bl = bl_list->head;

	for (unsigned int i = 0; i < bl_list->size; i++) {
		node_t *aux_bl = curr_bl;
		curr_bl = curr_bl->next;

		list_t *mini_list = (*(block_t *)aux_bl->data).miniblock_list;
		node_t *curr_mini = mini_list->head;

		for (unsigned int j = 0; j < mini_list->size; j++) {
			node_t *aux_mini = curr_mini;
			curr_mini = curr_mini->next;
			if ((*(miniblock_t *)aux_mini->data).rw_buffer)
				free((*(miniblock_t *)aux_mini->data).rw_buffer);
			free(aux_mini->data);
			free(aux_mini);
		}

		free(mini_list);
		free(aux_bl->data);
		free(aux_bl);
	}

	free(bl_list);
	free(arena);
	exit(0);
}

void merge_blocks(node_t *right, node_t *left, node_t *miniblock,
				  node_t *new_block, list_t *bl_list)
{
	if (right && !left) {
		list_t *mini_list = (*(block_t *)right->data).miniblock_list;
		block_t *bl_data = (block_t *)right->data;
		miniblock_t *miniblock_data = (miniblock_t *)miniblock->data;

		bl_data->start_address = miniblock_data->start_address;
		bl_data->size += miniblock_data->size;

		miniblock->next = mini_list->head;
		mini_list->head->prev = miniblock;
		mini_list->head = miniblock;
		miniblock->prev = NULL;
		mini_list->size++;

		free((*(block_t *)new_block->data).miniblock_list);
		free(new_block->data);
		free(new_block);

	} else if (!right && left) {
		list_t *mini_list = (*(block_t *)left->data).miniblock_list;
		node_t *last = mini_list->head;
		block_t *block_data = (block_t *)left->data;

		block_data->size += (*(miniblock_t *)miniblock->data).size;

		while (last->next)
			last = last->next;

		last->next = miniblock;
		miniblock->prev = last;
		miniblock->next = NULL;
		mini_list->size++;

		free((*(block_t *)new_block->data).miniblock_list);
		free(new_block->data);
		free(new_block);

	} else if (right && left) {
		list_t *left_minilist = (*(block_t *)left->data).miniblock_list;
		list_t *right_minilist = (*(block_t *)right->data).miniblock_list;
		block_t *bl_data = (block_t *)right->data;
		block_t *left_data = (block_t *)left->data;

		left_data->size += (*(miniblock_t *)miniblock->data).size;
		left_data->size += bl_data->size;

		node_t *last = left_minilist->head;
		while (last->next)
			last = last->next;

		last->next = miniblock;
		miniblock->prev = last;
		miniblock->next = NULL;
		left_minilist->size++;

		miniblock->next = right_minilist->head;
		right_minilist->head->prev = miniblock;
		left_minilist->size += right_minilist->size;

		left->next = right->next;
		if (right->next)
			right->next->prev = left;

		bl_list->size--;
		free(right_minilist);
		free((*(block_t *)new_block->data).miniblock_list);
		free(new_block->data);
		free(new_block);
		free(right->data);
		free(right);
	}
}

int check_alloc(arena_t *arena, list_t *bl_list,
				const uint64_t address, const uint64_t size)
{
	if (!arena) {
		printf("Alloc arena first!\n");
		return 0;
	}
	if (!bl_list)
		return 0;

	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return 0;
	}

	if (address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return 0;
	}
	return 1;
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	list_t *bl_list = arena->alloc_list;
	if (!check_alloc(arena, bl_list, address, size))
		return;

	int ok = 1;
	node_t *curr = bl_list->head;
	// verificare daca a fost deja alocata o zona din arena
	while (curr) {
		uint64_t block_end = BLOCK_SIZE + BLOCK_START;

		if (address + size > BLOCK_START && address + size < block_end)
			ok = 0;

		if (address > BLOCK_START && address + size < block_end)
			ok = 0;

		if (address > BLOCK_START && address < block_end &&
		    address + size >= block_end)
			ok = 0;

		if (address < BLOCK_START && address + size > block_end)
			ok = 0;

		if (address == BLOCK_START && address + size == block_end)
			ok = 0;

		curr = curr->next;
	}

	if (ok == 0) {
		printf("This zone was already allocated.\n");
		return;
	}

	// verificare daca blockul are vecini
	curr = bl_list->head;
	while (curr) {
		uint64_t block_end = BLOCK_SIZE + BLOCK_START;
		if (block_end == address)
			ok = 0;

		if (address + size == BLOCK_START)
			ok = 0;

		curr = curr->next;
	}

	// daca blockul nu are vecini, se introduce direct in lista
	node_t *new_block = create_node(sizeof(block_t));
	initialize_block(new_block, address, size);

	node_t *miniblock = create_node(sizeof(miniblock_t));
	(*(block_t *)(new_block->data)).miniblock_list->head = miniblock;
	initialize_miniblock(miniblock, address, size);

	if (ok == 1) {
		insert_block(bl_list, new_block);
		return;
	}

	curr = bl_list->head;
	// se cauta vecini in dreapta blocului
	while (curr) {
		if (address + size == BLOCK_START)
			break;
		curr = curr->next;
	}

	node_t *aux_curr = bl_list->head;
	// se cauta vecini in stanga nodului
	while (aux_curr) {
		block_t *block_data = (block_t *)aux_curr->data;
		if (block_data->start_address + block_data->size == address)
			break;
		aux_curr = aux_curr->next;
	}

	merge_blocks(curr, aux_curr, miniblock, new_block, bl_list);
}

void free_single_block(list_t *bl_list, node_t *curr,
					   node_t *mini_curr, list_t *mini_list)
{
	// daca blockul care trebuie eliminat e headul listei
	if (bl_list->size != 1) {
		if (bl_list->head == curr) {
			bl_list->head = curr->next;
			bl_list->head->prev = NULL;
		} else if (!curr->next) {
			curr->prev->next = NULL;
		} else {
			curr->prev->next = curr->next;
			curr->next->prev = curr->prev;
		}
	}

	if ((*(miniblock_t *)mini_curr->data).rw_buffer)
		free((*(miniblock_t *)mini_curr->data).rw_buffer);
	free(mini_curr->data);
	mini_curr->data = NULL;
	free(mini_curr);
	mini_curr = NULL;
	free(mini_list);
	mini_list = NULL;
	free(curr->data);
	curr->data = NULL;
	free(curr);
	curr = NULL;
	bl_list->size--;
	if (bl_list->size == 0)
		bl_list->head = NULL;
}

void split_block(node_t *new_left, node_t *new_right, node_t *curr_left,
				 node_t *curr_right)
{
	while (curr_right) {
		block_t *right_data = (block_t *)new_right->data;
		node_t *copy = malloc(sizeof(node_t));
		copy->data = curr_right->data;
		miniblock_t *mini_data = (miniblock_t *)copy->data;
		copy->next = NULL;
		copy->prev = NULL;

		if (!(*(block_t *)new_right->data).miniblock_list->head) {
			right_data->miniblock_list->head = copy;
			right_data->miniblock_list->size = 1;
			right_data->start_address = mini_data->start_address;
			right_data->size = mini_data->size;
		} else {
			add_last(right_data->miniblock_list, copy);
			right_data->miniblock_list->size++;
			right_data->size += mini_data->size;
		}
		curr_right = curr_right->next;
	}

	block_t *left_data = (block_t *)new_left->data;
	while (curr_left) {
		node_t *copy = malloc(sizeof(node_t));
		copy->data = curr_left->data;
		miniblock_t *copy_data = (miniblock_t *)copy->data;
		copy->next = NULL;
		copy->prev = NULL;

		if (!left_data->miniblock_list->head) {
			left_data->miniblock_list->head = copy;
			left_data->miniblock_list->size = 1;
			left_data->start_address = copy_data->start_address;
			left_data->size = copy_data->size;
		} else {
			copy->next = left_data->miniblock_list->head;
			left_data->miniblock_list->head->prev = copy;
			left_data->miniblock_list->head = copy;
			left_data->start_address = copy_data->start_address;
			left_data->miniblock_list->size++;
			left_data->size += copy_data->size;
		}
		curr_left = curr_left->prev;
	}
}

void link_blocks(list_t *bl_list, node_t *curr, node_t *new_left,
				 node_t *new_right)
{
	if (curr == bl_list->head) {
		new_left->prev = NULL;
		new_right->prev = new_left;
		new_left->next = new_right;

		if (bl_list->size == 2)
			new_right->next = NULL;

		if (curr->next) {
			new_right->next = curr->next;
			curr->next->prev = new_right;
		}
		bl_list->head = new_left;
	} else if (!curr->next) {
		// daca blocul este ultimul
		new_left->next = new_right;
		new_right->next = NULL;
		new_right->prev = new_left;
		if (curr->prev) {
			new_left->prev = curr->prev;
			curr->prev->next = new_left;
		}
	} else {
		new_left->prev = curr->prev;
		new_left->next = new_right;
		new_right->next = curr->next;
		new_right->prev = new_left;
		curr->prev->next = new_left;
		curr->next->prev = new_right;
	}
}

/* elibereaza datele care nu sunt necesare
dintr-un bloc care se imparte in doua */
void free_big_block(node_t *curr, node_t *mini_curr)
{
	if ((*(miniblock_t *)mini_curr->data).rw_buffer)
		free((*(miniblock_t *)mini_curr->data).rw_buffer);

	free(mini_curr->data);
	node_t *curr_aux = (*(block_t *)curr->data).miniblock_list->head;
	uint64_t n = (*(block_t *)curr->data).miniblock_list->size;
	for (unsigned int i = 0; i < n; i++) {
		node_t *node_free = curr_aux;
		curr_aux = curr_aux->next;
		free(node_free);
	}

	free((*(block_t *)curr->data).miniblock_list);
	free(curr->data);
	free(curr);
}

void free_first_miniblock(list_t *mini_list, node_t *curr, node_t *mini_curr)
{
	miniblock_t *next_data = (miniblock_t *)mini_list->head->next->data;
	BLOCK_START = next_data->start_address;
	BLOCK_SIZE -= (*(miniblock_t *)mini_curr->data).size;
	mini_list->head = mini_curr->next;
	mini_list->head->prev = NULL;
	if ((*(miniblock_t *)mini_curr->data).rw_buffer)
		free((*(miniblock_t *)mini_curr->data).rw_buffer);
	free(mini_curr->data);
	free(mini_curr);
	mini_list->size--;
}

void free_last_miniblock(list_t *mini_list, node_t *curr, node_t *mini_curr)
{
	BLOCK_SIZE -= (*(miniblock_t *)mini_curr->data).size;
	mini_curr->prev->next = NULL;
	if ((*(miniblock_t *)mini_curr->data).rw_buffer)
		free((*(miniblock_t *)mini_curr->data).rw_buffer);
	free(mini_curr->data);
	free(mini_curr);
	mini_list->size--;
}

void free_block(arena_t *arena, const uint64_t address)
{
	if (!arena) {
		printf("Alloc arena first!\n");
		return;
	}

	if (arena->alloc_list->size == 0) {
		printf("Invalid address for free.\n");
		return;
	}

	list_t *bl_list = arena->alloc_list;
	list_t *mini_list = NULL;
	node_t *curr = bl_list->head;
	node_t *mini_curr = NULL;
	unsigned int n = 0;
	int ok = 0;

	while (curr) {
		mini_list = (*(block_t *)curr->data).miniblock_list;
		mini_curr = mini_list->head;
		while (mini_curr) {
			if ((*(miniblock_t *)mini_curr->data).start_address == address) {
				ok = 1;
				break;
			}
			mini_curr = mini_curr->next;
		}
		if (ok == 1)
			break;

		curr = curr->next;
	}

	if (ok == 0) {
		printf("Invalid address for free.\n");
		return;
	}

	n = mini_list->size;
	// daca exista un singur miniblock
	if (n == 1) {
		free_single_block(bl_list, curr, mini_curr, mini_list);
		return;
	}

	// daca este primul miniblock din lista
	if (!mini_curr->prev) {
		free_first_miniblock(mini_list, curr, mini_curr);
		return;
	}

	// daca este ultimul miniblock din lista
	if (!mini_curr->next) {
		free_last_miniblock(mini_list, curr, mini_curr);
		return;
	}

	node_t *curr_right = mini_curr->next;
	node_t *new_right = create_node(sizeof(block_t));
	initialize_block(new_right, 0, 0);
	(*(block_t *)new_right->data).miniblock_list->head = NULL;

	node_t *curr_left = mini_curr->prev;
	node_t *new_left = create_node(sizeof(block_t));
	initialize_block(new_left, 0, 0);
	(*(block_t *)new_left->data).miniblock_list->head = NULL;

	split_block(new_left, new_right, curr_left, curr_right);

	bl_list->size++;

	link_blocks(bl_list, curr, new_left, new_right);
	free_big_block(curr, mini_curr);
}

void print_buffer(node_t *curr_miniblk, uint64_t r_address, uint64_t my_size)
{
	char last;
	uint64_t cnt = my_size, data_read = 0;
	unsigned int i, ok = 0;
	miniblock_t *miniblk_data = NULL;

	while (data_read != cnt) {
		miniblk_data = (miniblock_t *)curr_miniblk->data;
		if (r_address == miniblk_data->start_address) {
			for (i = 0; i < miniblk_data->size && data_read != cnt; i++) {
				printf("%c", ((char *)miniblk_data->rw_buffer + i)[0]);
				last = ((char *)miniblk_data->rw_buffer + i)[0];
				ok = 1;
				data_read++;
			}
		} else {
			unsigned int offset = r_address - miniblk_data->start_address;
			for (i = offset; i < miniblk_data->size && data_read != cnt; i++) {
				printf("%c", ((char *)miniblk_data->rw_buffer + i)[0]);
				last = ((char *)miniblk_data->rw_buffer + i)[0];
				ok = 1;
				data_read++;
			}
		}
		if (data_read == cnt)
			break;
		curr_miniblk = curr_miniblk->next;
		r_address = (*(miniblock_t *)curr_miniblk->data).start_address;
	}
	if (ok == 1) {
		if (last != '\n')
			printf("\n");
	}
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	if (!arena)
		return;

	if (!arena->alloc_list) {
		printf("Invalid address for read.\n");
		return;
	}
	list_t *bl_list = arena->alloc_list;

	if (!bl_list->head) {
		printf("Invalid address for read.\n");
		return;
	}

	node_t *read_block = bl_list->head;
	block_t *blk_data = (block_t *)read_block->data;
	int ok = -1;
	while (read_block && ok == -1) {
		blk_data = (block_t *)read_block->data;

		if (blk_data->start_address <= address &&
			blk_data->start_address + blk_data->size >= address) {
			if (blk_data->start_address + blk_data->size >= address + size)
				ok = 1;
			else
				ok = 0;
		}
		read_block = read_block->next;
	}

	if (ok == -1) {
		printf("Invalid address for read.\n");
		return;
	}

	uint64_t my_size = size, r_address = address;
	if (ok == 0) {
		uint64_t end = blk_data->start_address + blk_data->size;
		printf("Warning: size was bigger than the block size. ");
		printf("Reading %ld characters.\n", end - r_address);
		my_size = end - r_address;
	}

	node_t *curr_miniblk = blk_data->miniblock_list->head;
	miniblock_t *miniblk_data = NULL;

	node_t *check_perms = NULL;

	while (curr_miniblk) {
		miniblk_data = (miniblock_t *)curr_miniblk->data;
		if (miniblk_data->start_address <= address &&
		    miniblk_data->start_address + miniblk_data->size > address)
			break;
		curr_miniblk = curr_miniblk->next;
	}

	if (!curr_miniblk) {
		printf("Invalid address for read.\n");
		return;
	}

	check_perms = curr_miniblk;
	while (check_perms) {
		miniblk_data = (miniblock_t *)check_perms->data;

		if (miniblk_data->perm < 4 || miniblk_data->perm == 7) {
			printf("Invalid permissions for read.\n");
			return;
		}
		check_perms = check_perms->next;
	}

	print_buffer(curr_miniblk, r_address, my_size);
}

void write_buffer(node_t *curr_miniblk, uint64_t my_size,
				  char *data, uint64_t w_address, block_t *blk_data)
{
	uint64_t cnt = my_size, indx = 0;
	miniblock_t *miniblk_data = NULL;
	while (cnt > 0) {
		miniblk_data = (miniblock_t *)curr_miniblk->data;
		if (!miniblk_data->rw_buffer)
			miniblk_data->rw_buffer = calloc(miniblk_data->size, sizeof(char));

		void *buffer = miniblk_data->rw_buffer;

		if (w_address == miniblk_data->start_address) {
			if (strlen(data + indx) <= miniblk_data->size) {
				size_t no_bytes = strlen(data + indx);
				memcpy(buffer, data + indx, no_bytes);
				cnt = 0;
			} else {
				memcpy(buffer, data + indx, miniblk_data->size);
				cnt -= miniblk_data->size;
				indx += miniblk_data->size;
				miniblock_t *next_dt = (miniblock_t *)curr_miniblk->next->data;
				if (curr_miniblk->next)
					w_address = next_dt->start_address;
			}
		} else {
			uint64_t offset = w_address - blk_data->start_address;
			if (strlen(data + indx) <= miniblk_data->size) {
				memcpy(buffer + offset, data + indx, strlen(data + indx));
				cnt = 0;
			} else {
				uint64_t size = miniblk_data->size;
				memcpy(buffer + offset, data + indx, size - w_address);
				cnt -= miniblk_data->size - w_address;
				indx += miniblk_data->size - w_address;
				miniblock_t *next_dt = (miniblock_t *)curr_miniblk->next->data;
				if (curr_miniblk->next)
					w_address = next_dt->start_address;
			}
		}
		curr_miniblk = curr_miniblk->next;
	}
}

void write(arena_t *arena, const uint64_t address,
		   const uint64_t size, char *data)
{
	if (!arena) {
		free(data);
		return;
	}

	if (!arena->alloc_list) {
		free(data);
		printf("Invalid address for write.\n");
		return;
	}
	list_t *bl_list = arena->alloc_list;
	if (!bl_list->head) {
		free(data);
		printf("Invalid address for write.\n");
		return;
	}

	node_t *write_block = bl_list->head;
	block_t *blk_data = (block_t *)write_block->data;
	int ok = -1;
	while (write_block && ok == -1) {
		blk_data = (block_t *)write_block->data;
		if (blk_data->start_address <= address &&
			blk_data->start_address + blk_data->size >= address) {
			if (blk_data->start_address + blk_data->size >= address + size)
				ok = 1;
			else
				ok = 0;
		}
		write_block = write_block->next;
	}

	if (ok == -1) {
		free(data);
		printf("Invalid address for write.\n");
		return;
	}

	uint64_t my_size = size, w_address = address;
	if (ok == 0) {
		uint64_t end = blk_data->start_address + blk_data->size;
		printf("Warning: size was bigger than the block size. ");
		printf("Writing %ld characters.\n", end - w_address);
		my_size = end - w_address;
		data[my_size] = '\0';
	}

	// caut miniblockul de la care incepe scrierea
	node_t *curr_miniblk = blk_data->miniblock_list->head;
	miniblock_t *miniblk_data = NULL;
	while (curr_miniblk) {
		miniblk_data = (miniblock_t *)curr_miniblk->data;
		if (miniblk_data->start_address <= address &&
			miniblk_data->start_address + miniblk_data->size > address)
			break;
		curr_miniblk = curr_miniblk->next;
	}

	if (!curr_miniblk) {
		free(data);
		printf("Invalid address for write.\n");
		return;
	}

	node_t *check_perms = curr_miniblk;
	while (check_perms) {
		miniblk_data = (miniblock_t *)check_perms->data;
		if (miniblk_data->perm <= 1 || miniblk_data->perm == 4 ||
			miniblk_data->perm == 5) {
			printf("Invalid permissions for write.\n");
			return;
		}
		check_perms = check_perms->next;
	}

	write_buffer(curr_miniblk, my_size, data, w_address, blk_data);
	free(data);
}

void pmap(const arena_t *arena)
{
	if (!arena) {
		printf("Alloc arena first!\n");
		return;
	}
	int total_memory = 0, miniblocks_count = 0;
	list_t *bl_list = arena->alloc_list, *mini_list = NULL;
	node_t *curr = bl_list->head;
	node_t *curr_mini = NULL;

	for (unsigned int i = 0; i < bl_list->size; i++) {
		total_memory += BLOCK_SIZE;

		mini_list = (*(block_t *)curr->data).miniblock_list;
		curr_mini = mini_list->head;

		while (curr_mini) {
			miniblocks_count++;
			curr_mini = curr_mini->next;
		}

		curr = curr->next;
	}

	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	printf("Free memory: 0x%lX bytes\n", arena->arena_size - total_memory);
	printf("Number of allocated blocks: %d\n", bl_list->size);
	printf("Number of allocated miniblocks: %d\n", miniblocks_count);

	curr = bl_list->head;
	for (unsigned int i = 0; i < bl_list->size; i++) {
		printf("\nBlock %d begin\n", i + 1);
		printf("Zone: 0x%lX - 0x%lX\n", BLOCK_START, BLOCK_START + BLOCK_SIZE);

		mini_list = (*(block_t *)curr->data).miniblock_list;
		curr_mini = mini_list->head;
		int cnt = 0;
		while (curr_mini) {
			uint64_t start = (*(miniblock_t *)curr_mini->data).start_address;
			uint64_t size = (uint64_t)((*(miniblock_t *)curr_mini->data).size);
			miniblock_t *mini_data = ((miniblock_t *)curr_mini->data);
			printf("Miniblock %d:\t\t0x%lX\t\t", ++cnt, start);
			printf("-\t\t0x%lX\t\t| ", start + size);
			switch (mini_data->perm) {
			case 7:
				printf("RWX\n");
				break;
			case 6:
				printf("RW-\n");
				break;

			case 5:
				printf("R-X\n");
				break;

			case 4:
				printf("R--\n");
				break;

			case 3:
				printf("-WX\n");
				break;

			case 2:
				printf("-W-\n");
				break;

			case 1:
				printf("--X\n");
				break;

			default:
				printf("---\n");
			}
			curr_mini = curr_mini->next;
		}
		printf("Block %d end\n", i + 1);
		curr = curr->next;
	}
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	if (!arena)
		return;

	if (!arena->alloc_list) {
		printf("Invalid address for mprotect.\n");
		return;
	}
	list_t *bl_list = arena->alloc_list;

	if (!bl_list->head) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	node_t *curr_block = bl_list->head;
	block_t *blk_data = (block_t *)curr_block->data;
	int ok = -1;
	while (curr_block) {
		blk_data = (block_t *)curr_block->data;

		if (blk_data->start_address <= address &&
			blk_data->start_address + blk_data->size >= address)
			ok = 1;
		curr_block = curr_block->next;
	}

	if (ok == -1) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	node_t *curr_miniblk = blk_data->miniblock_list->head;
	miniblock_t *miniblk_data = NULL;

	while (curr_miniblk) {
		miniblk_data = (miniblock_t *)curr_miniblk->data;
		if (miniblk_data->start_address == address)
			break;
		curr_miniblk = curr_miniblk->next;
	}

	if (!curr_miniblk) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	miniblk_data->perm = permission[0];
}

void get_perm(char command[1000], int8_t *perm)
{
	char copy[1000];
	strcpy(copy, command);
	char *token = strtok(copy, " |");
	token = strtok(NULL, " |");
	token = strtok(NULL, " |");
	perm[0] = 0;
	int none = 0, write = 0, read = 0, exec = 0;
	while (token) {
		if (!strcmp(token, "PROT_NONE"))
			none = 1;

		if (!strcmp(token, "PROT_READ"))
			read = 1;

		if (!strcmp(token, "PROT_WRITE"))
			write = 1;

		if (!strcmp(token, "PROT_EXEC"))
			exec = 1;

		token = strtok(NULL, " |");
	}

	if (none == 1) {
		perm[0] = 0;
		return;
	}

	if (read == 1)
		perm[0] += 4;

	if (write == 1)
		perm[0] += 2;

	if (exec == 1)
		perm[0] += 1;
}

int get_command(char command[200])
{
	if (command[strlen(command) - 1] == '\n')
		command[strlen(command) - 1] = '\0';

	char copy[200], *token;
	strcpy(copy, command);
	int params = 0;
	token = strtok(copy, " ");
	while (token) {
		params++;
		token = strtok(NULL, " ");
	}

	int ok = -1;

	if (!strncmp(command, "ALLOC_ARENA ", 12)) {
		if (params == 2)
			ok = 1;
	}

	if (!strncmp(command, "DEALLOC_ARENA", 13)) {
		if (params == 1)
			ok = 2;
	}

	if (!strncmp(command, "ALLOC_BLOCK ", 12)) {
		if (params == 3)
			ok = 3;
	}

	if (!strncmp(command, "FREE_BLOCK ", 11)) {
		if (params == 2)
			ok = 4;
	}

	if (!strncmp(command, "READ ", 5)) {
		if (params == 3)
			ok = 5;
	}

	if (!strncmp(command, "WRITE ", 6)) {
		if (params >= 3)
			ok = 6;
	}

	if (!strncmp(command, "PMAP", 4)) {
		if (params == 1)
			ok = 7;
	}

	if (!strncmp(command, "MPROTECT ", 9)) {
		if (params >= 3)
			ok = 8;
	}

	if (ok == -1) {
		for (int i = 0; i < params; i++)
			printf("Invalid command. Please try again.\n");
	}

	return ok;
}

char *get_data(char command[1000], uint64_t size)
{
	strcat(command, "\n");
	unsigned int i = 0, cnt = 0;
	while (cnt != 3) {
		if (command[i++] == ' ')
			cnt++;
	}

	char *data = calloc(size * 2, sizeof(char));
	strncpy(data, command + i, size);
	char aux[1000] = "";
	while (strlen(data) < size) {
		fgets(aux, 1000, stdin);
		if (strlen(aux) + strlen(data) > size)
			strncat(data, aux, (unsigned int)size - strlen(data));
		else
			strcat(data, aux);
	}
	return data;
}
