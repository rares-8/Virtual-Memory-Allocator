#pragma once
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_START ((*(block_t *)curr->data).start_address)
#define BLOCK_SIZE ((*(block_t *)curr->data).size)

/* TODO : add your implementation for doubly-linked list */
typedef struct node_t node_t;
struct node_t {
	void *data;
	node_t *prev;
	node_t *next;
};

typedef struct {
	node_t *head;
	unsigned int data_size;
	unsigned int size;
} list_t;

typedef struct {
	uint64_t start_address;
	size_t size;
	uint8_t perm;
	void *rw_buffer;
} miniblock_t;

typedef struct {
	uint64_t start_address;
	size_t size;
	list_t *miniblock_list;
} block_t;

typedef struct {
	uint64_t arena_size;
	list_t *alloc_list;
} arena_t;

arena_t *alloc_arena(const uint64_t size);
void dealloc_arena(arena_t *arena);

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);
void free_block(arena_t *arena, const uint64_t address);

void read(arena_t *arena, uint64_t address, uint64_t size);
void write(arena_t *arena, const uint64_t address,
		   const uint64_t size, char *data);
void pmap(const arena_t *arena);
void mprotect(arena_t *arena, uint64_t address, int8_t *permission);

int get_command(char command[200]);

void add_last(list_t *list, node_t *new_node);
char *get_data(char command[1000], uint64_t size);
void get_perm(char command[1000], int8_t *perm);

node_t *create_node(size_t data_size);
void initialize_block(node_t *node, uint64_t start, size_t size);
void initialize_miniblock(node_t *miniblock, uint64_t address, uint64_t size);

void insert_block(list_t *bl_list, node_t *new_block);
void merge_blocks(node_t *curr, node_t *aux_curr, node_t *miniblock,
				  node_t *new_block, list_t *bl_list);
int check_alloc(arena_t *arena, list_t *bl_list,
				const uint64_t address, const uint64_t size);
void free_single_block(list_t *bl_list, node_t *curr,
					   node_t *mini_curr, list_t *mini_list);
void split_block(node_t *new_left, node_t *new_right, node_t *curr_left,
				 node_t *curr_right);
void link_blocks(list_t *bl_list, node_t *curr, node_t *new_left,
				 node_t *new_right);
void free_big_block(node_t *curr, node_t *mini_curr);
void free_first_miniblock(list_t *mini_list, node_t *curr, node_t *mini_curr);
void free_last_miniblock(list_t *mini_list, node_t *curr, node_t *mini_curr);
void print_buffer(node_t *curr_miniblk, uint64_t r_address, uint64_t my_size);
void write_buffer(node_t *curr_miniblk, uint64_t my_size,
				  char *data, uint64_t w_address, block_t *blk_data);
