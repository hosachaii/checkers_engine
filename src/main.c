#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "board.h"
#include "undo_redo.h"
#include "forced_jumps.h"
#include "core_logic.h"
#include "game_io.h"
#include "move_generation.h"
#include "minimax.h"
#include "game_modes.h"

// Facilitate Gameplay
void checkers(void) {
	int choice;
	clear_screen();
	printf("\n\n\nChoose Game Mode : \n");
	printf("\n1. Two_player\n");
	printf("\n2. Computer_player\n");
	printf("\nEnter : ");
	scanf("%d", &choice);
	if(choice == 1) {
		game_mode = TWO_PLAYER;
		two_player();
	}
	else if(choice == 2) {
		game_mode = AI_PLAYER;
		player_t starting_player;
		printf("\n\nHow would you like to start?\n");
		printf("\n1. I would like to start first.\n");
		printf("\n2. I would like the computer to start first.\n");
		printf("\nEnter : ");
		scanf("%d", &choice);
		starting_player = (choice == 1) ? PLAYER_1 : PLAYER_2;
		AI_player(starting_player);
	}
}

int main(void) {
	checkers();
	return 0;
}