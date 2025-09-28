#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "engine.h"

// Global variable to set game_mode
game_mode_t game_mode;

void init_board(board_t *board, game_history *history) {
	
	//Initialise checkers board to the start configuration.
	board -> player1_pawns = P1_PAWNS; // P1_PAWNS is a macro which contains the bit-positions of player1_pawns.
	board -> player2_pawns = P2_PAWNS; // P2_PAWNS is a macro which contains the bit-positions of player1_pawns.
	board -> player1_kings = 0; // No kings at the start.
	board -> player2_kings = 0; // No kings at the start.
	
	//Initialize game history stacks.
	history -> undo_top = NULL;
	history -> redo_top = NULL;
	return;
}

// Function used to display the board (visual representation) 
void display_board(board_t *board) {
	short i;
	printf("  +---+---+---+---+---+---+---+---+\n");
    // Iterate over all square indices.
	for (short row = 8; row >= 1; row--) {
      // row labels from 1 to 8
    	printf("%d |", row);
      	for (short col = 1; col <= 8; col++) {
			i = interpreter(col, row);
			// If bit at position i is set high in player1_pawns:
        	if (board -> player1_pawns & (1 << i))
				printf(" X |");
        	// If bit at position i is set high in player2_pawns:
        	else if (board -> player2_pawns & (1 << i))
				printf(" O |");
        	// If bit at position i is set high in player1_kings:
        	else if (board -> player1_kings & (1 << i))
				printf("KX |");
        	// If bit at position i is set high in player2_kings: 
        	else if (board -> player2_kings & (1 << i))
				printf("KO |");
			// If bit is set low for all bitboards, mark as empty.
        	else
        	  	printf(" . |");
		}
		printf("\n  +---+---+---+---+---+---+---+---+\n");
    }
    // column labels from A to H
    printf("    A   B   C   D   E   F   G   H  \n");
    return;
}

// Initialize board_state for undo-redo stack.
void init_board_state(board_state *bs) {
	bs -> next = NULL;
	return;
}

// Check is undo stack is empty.
bool is_undo_empty(game_history *history) {
	if(history -> undo_top -> next == NULL)
		return 1;
	return 0;
}

// Check if redo stack is empty.
bool is_redo_empty(game_history *history) {
	if(history -> redo_top == NULL)
		return 1;
	return 0;
}

// Save board state for undo-redo purposes.
void save_progress(board_t *board, game_history *history, player_t current_player) {
    // Allocate memory for the new board state.
    board_state *current_board = (board_state *)malloc(sizeof(board_state));

    // Initialize and configure board state.
    init_board_state(current_board);
    current_board -> board = *board;
    current_board -> player_turn = current_player;

    // Purge redo stack.
    if (!is_redo_empty(history))
        free_redo(history);

    // Push latest board state onto undo stack.
    current_board -> next = history -> undo_top;
    history -> undo_top = current_board;

    return;
}

// Reverts to the previous board state.
bool undo_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player) {
	if (is_undo_empty(history)) 
		return FALSE;

	board_state *p = history -> undo_top;
	
	// Pop latest board state from undo stack and push onto redo stack.
	history -> undo_top = p -> next;
	p -> next = history -> redo_top;
	history -> redo_top = p;
	
	// Set current_player according to the board_state just pushed to redo stack.
	*current_player = history -> redo_top -> player_turn;
	
	// Set board corresponding to undo_top.
	p = history -> undo_top;
	*board = p -> board;

	// Update fj with possible forced jumps.
	are_forced_jumps(board, fj, *current_player);
	
	return TRUE;
}

// Reverts the action of undo.
bool redo_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player) {
	if(is_redo_empty(history))
		return FALSE;
	else {
		board_state *p = history -> redo_top;
		
		// Pop board from redo stack and push onto undo stack.
		history -> redo_top = p -> next;
		p -> next = history -> undo_top;
		history -> undo_top = p;
		
		// Set board corresponding to undo_top.
		*board = p -> board;
		
		// If current board state is the latest board state
		if(is_redo_empty(history)) {
			// Calculate if any forced jumps available for current player.
			are_forced_jumps(board, fj, *current_player);
			// If no forced jumps are available and two player game is being played, switch players.
			if(is_fj_empty(fj) && game_mode == TWO_PLAYER) {
				switch_player(current_player);
				// Find forced jumps for the next player.
				are_forced_jumps(board, fj, *current_player);
			}
			else
				// If forced jumps are available for current player:
				*current_player = p -> player_turn;
		}
		// If redo stack is not empty, set board state corresponding to redo_top.
		else {
			*current_player = history -> redo_top -> player_turn;
			are_forced_jumps(board, fj, *current_player);
		}
		return TRUE;
	}
}

// Purge redo stack.
void free_redo(game_history *history) {
	// Check if redo-stack is empty.
	if(is_redo_empty(history))
		return;

	// If not, iterate through and free board_state(s).
	board_state *p = history -> redo_top;
	while(p != NULL) {
		history -> redo_top = p -> next;
		free(p);
		p = history -> redo_top;
	}
	// Update stack top to NULL.
	history -> redo_top = NULL;
	return;
}

// Purge undo stack.
void free_undo(game_history *history) {
	board_state *p = history -> undo_top;
	while(p != NULL) {
		history -> undo_top = p -> next;
		free(p);
		p = history -> undo_top;
	}
	history -> undo_top = NULL;
	return;
}

// To calculate 0-7 row of a square from 32 based indexing.
unsigned short get_row(unsigned short index) {
	return index / 4;
}

// To calculate 0-7 column of a square from 32 based indexing.
unsigned short get_col(unsigned short index) {
	return (index / 4) % 2 ? (index % 4) * 2 + 1 : (index % 4) * 2;
}

unsigned short interpreter(unsigned short col, unsigned short row) {
	if(row % 2 == col % 2)
		return row % 2 ? (((row - 1) * 4) + (col / 2)) : (((row - 1) * 4) + (col / 2) - 1);
	else
		return INVALID;
}

//in this function, no pieces are moved. only checks validity of the movements
bool is_valid_move(board_t *b, player_t player, unsigned short from_index, unsigned short to_index, fj_array *fj) {

	// Check if indices are valid or not.
	if(from_index > LAST_INDEX || to_index > LAST_INDEX) {
		return FALSE;
	}

	// Check if any forced jumps are avaiable.
    if(!is_fj_empty(fj)) {
    	for(int i = 0; i < fj -> size; i++) {
    		if(from_index == fj -> jumps[i].from_index && to_index == fj -> jumps[i].to_index) {
    			fj -> index = i;
    			return TRUE;
    		}
    	}
    	return FALSE;
    }
	
	uint32_t player_pieces = player == PLAYER_1 ? b -> player1_pawns | b -> player1_kings : b -> player2_pawns | b -> player2_kings;
	uint32_t opp_pieces = player == PLAYER_2 ? b -> player1_pawns | b -> player1_kings : b -> player2_pawns | b -> player2_kings;

	// Checking if player has chosen their own pieces.
	if(!(player_pieces & (1 << from_index))) {
		return FALSE;
	}

	// checking if the destination square is empty
	if ((player_pieces | opp_pieces) & (1 << to_index)) {
        return FALSE;
    }

	if(player == PLAYER_1 || b -> player2_kings & (1 << from_index)) {
		if(to_index == from_index + 4)
			return TRUE;
		else {
			// Check if pawn is on even rows.
			if((from_index / 4) % 2 == 0 && (from_index % 8 != 0 && (to_index == from_index + 3)))
				return TRUE;
			// Check if pawn on odd rows.
			else if((from_index / 4) % 2 == 1 && (from_index % 8 != 7 && (to_index == from_index + 5)))
				return TRUE;
		}
	}
	if(player == PLAYER_2 || b -> player1_kings & (1 << from_index)) {
		if(to_index == from_index - 4)
			return TRUE;
		else {
			// Check if pawn is on even rows.
			if((from_index / 4) % 2 == 0 && ((from_index % 8 != 0) && (to_index == from_index - 5)))
				return TRUE;
			// Check if pawn on odd rows.
			else if((from_index / 4) % 2 == 1 && ((from_index % 8 != 7) && (to_index == from_index - 3)))
				return TRUE;
		}
	}
    return FALSE;
}

// Generate bitboard after movement
void move_piece(board_t *b, unsigned short from_index, unsigned short to_index, fj_array *fj, player_t player) {
	*b = generate_bitboard(*b, from_index, to_index, player);
	if(fj -> index != -1) {
		uint32_t *capture_pieces;
		int capture_index = fj -> jumps[fj -> index].capture_index;
		if(player == PLAYER_1) {
			if(b -> player2_pawns & 1U << capture_index)
				capture_pieces = &(b -> player2_pawns);
			else
				capture_pieces = &(b -> player2_kings);
		}
		else {
			if(b -> player1_pawns & 1U << capture_index)
				capture_pieces = &(b -> player1_pawns);
			else
				capture_pieces = &(b -> player1_kings);
		}
		*capture_pieces &= ~(1U << capture_index);
	}
}

// For pieces able move to from 1st row towards the 8th row.
bool forward_moving_pieces(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares) {
	short i;
	for(i = 27; i >= 0; i--) {
		if(player_pieces & (1 << i)) {
			// Check if 4's diagonal is empty.
			if(empty_squares & (1 << (i + 4)))
				return 1;
			else {
				// Check if pawn is on even rows.
				if((i / 4) % 2 == 0 && (i % 8 != 0 && (empty_squares & 1 << (i + 3))))
					return 1;
				// Check if pawn on odd rows.
				else if((i / 4) % 2 == 1 && (i % 8 != 7 && (empty_squares & 1 << (i + 5))))
					return 1;
			}
		}
	}
	return 0;
}

// For pieces moving from 8th row towards the 1st row.
bool reverse_moving_pieces(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares) {
	short i;
	for(i = 4; i < BOARD_SIZE; i++) {
		if(player_pieces & (1 << i)) {
			if(empty_squares & (1 << (i - 4)))
				return 1;
			else {
				// Check if pawn is on even rows.
				if((i / 4) % 2 == 0 && ((i % 8 != 0) && (empty_squares & 1 << (i - 5))))
					return 1;
				else if((i / 4) % 2 == 1 && ((i % 8 != 7) && (empty_squares & 1 << (i - 3))))
					return 1;
			}
		}
	}
	return 0;
}

// Function to check if game is over.
unsigned short is_game_over(board_t *b, player_t player) {
	if(!(b -> player1_pawns || b -> player1_kings))
		return 2;
	else if (!(b -> player2_pawns || b -> player2_kings))
		return 1;
	else {
        bool moves_left;
		uint32_t empty_squares = ~(b -> player1_pawns | b -> player2_pawns | b -> player1_kings | b -> player2_kings);
		uint32_t player1_pieces = b -> player1_pawns | b -> player1_kings;
		uint32_t player2_pieces = b -> player2_pawns | b -> player2_kings;

        if(player == PLAYER_1) {
            moves_left = forward_moving_pieces(player1_pieces, player2_pieces, empty_squares);
            if(!moves_left && b -> player1_kings)
                moves_left = reverse_moving_pieces(b -> player1_kings, player2_pieces, empty_squares);
            return moves_left ? 0 : 2;
        }
        else {
            moves_left = reverse_moving_pieces(player2_pieces, player1_pieces, empty_squares);
            if(!moves_left && b -> player2_kings)
                moves_left = forward_moving_pieces(b -> player2_kings, player1_pieces, empty_squares);
            return moves_left ? 0 : 1;
        }
	}
}

// Switches player pointed to by player *.
void switch_player(player_t *player) {
	if(*player == PLAYER_1)
		*player = PLAYER_2;
	else if(*player == PLAYER_2)
		*player = PLAYER_1;
	return;
}

// Main function to execute moves provided by the player and to update the board.
unsigned short move_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player) {
		unsigned short from_index, to_index, game_status;
		
		// prompts the player to enter a valid move.
		get_move(board, &from_index, &to_index, fj, *current_player);
		
		// Special action requests are returned to the caller to be handled.
		if(from_index > INVALID || to_index > INVALID) {
			if(from_index < INVALID)
				return to_index;
			return from_index;
		}
		// move the desired piece.
		*board = generate_bitboard(*board, from_index, to_index, *current_player);
		
		// Update the game_history stacks.
		save_progress(board, history, *current_player);
		
		// Check if game is over:
		game_status = is_game_over(board, *current_player);
		if(game_status != 0) {
			funeral(board, history, fj, game_status);
			return FALSE;
		}
		
		// Check if current move was a capture:
		if(!is_fj_empty(fj)) {
			// If yes, Check if additional forced jumps are available for the current player.
			are_forced_jumps(board, fj, *current_player);
			prune_fj(fj, to_index);
			// If there are additional forced jumps, don't switch the current player.
			if(is_fj_empty(fj)) {
				// If no additional forced jumps are available, switch player
				// And find forced jumps for the next player.
				switch_player(current_player);
				are_forced_jumps(board, fj, *current_player);
			}
		}
		// If current move was not a capture:
		else {
			// Switch player and find forced jumps for the next player.
			switch_player(current_player);
			are_forced_jumps(board, fj, *current_player);
		}
		return TRUE;
}

// Function called at the end of the game to free all allocated memory
// And to print the final board configuration and game result.
void funeral(board_t *board, game_history *history, fj_array *fj, unsigned short game_state) {
	// Display final baord configuration.
	clear_screen();
	display_board(board);
	printf("\n============ GAME OVER ============\n\n");
	// Free allocated memory.
	free(board);
	free_undo(history);
	free_redo(history);
	free(fj -> jumps);
	// Print game result.
	if(game_state == QUIT)
		return;
	else if(game_state == 1)
		printf("Player 1 won the game.\n");
	else if(game_state == 2)
		printf("Player 2 won the game.\n");
	return;
}

// Check if the forced jump array is empty.
bool is_fj_empty(fj_array *fj) {
	if(fj -> size == 0)
		return TRUE;
	return FALSE;
}

// Initialize the forced jumps array.
void init_fj_array(fj_array *fj) {
	fj -> jumps = (forced_jump *)malloc(sizeof(forced_jump) * MAX_FORCED_JUMPS);
	fj -> size = 0;
	fj -> index = -1;
	return;
}

// Append a valid forced jump to the array.
void fj_append(fj_array *fj, unsigned short from_index, unsigned short capture_index, unsigned short to_index) {
	if(fj -> size < MAX_FORCED_JUMPS) {
		fj -> jumps[fj -> size].from_index = from_index;
		fj -> jumps[fj -> size].capture_index = capture_index;
		fj -> jumps[fj -> size].to_index = to_index;
		fj -> size++;
	}
	else
		printf("Error : Max forced jump limit exceeded | fn : fj_append");
	return;
}

// Find available forced jumps for pieces able to move from 1st row to 8th row.
void find_forward_fj(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares, fj_array *fj) {
	short i;
	// Iterate through right most square of the 6th row to the 0th square.
	for(i = 24; i >= 0; i--) {
		if(player_pieces & (1 << i)) {
			// Check if piece is on odd rows.
			if((i / 4) % 2) {
				// Check if valid capture is available on non-4's diagonal.
				if(i % 8 != 7 && (opp_pieces & (1 << (i + 5))) && (empty_squares & (1 << (i + 9))))
					fj_append(fj, i, i + 5, i + 9);
				// Check if valid capture is available on 4's diagonal.
				if(i % 8 != 4 && (opp_pieces & (1 << (i + 4))) && (empty_squares & (1 << (i + 7))))
					fj_append(fj, i, i + 4, i + 7);
			}
			// If piece is on even rows.
			else {
				// Check if valid capture is available on non-4's diagonal.
				if(i < 20 && (i % 8 != 0 && (opp_pieces & (1 << (i + 3))) && (empty_squares & (1 << (i + 7)))))
					fj_append(fj, i, i + 3, i + 7);
				// Check if valid capture is available on 4's diagonal.
				if(i < 20 && (i % 8 != 3 && (opp_pieces & (1 << (i + 4))) && (empty_squares & (1 << (i + 9)))))
					fj_append(fj, i, i + 4, i + 9);
			}
		}
	}
	return;
}

// Find available forced jumps for pieces able to move from 8th row to 1st row.
void find_reverse_fj(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares, fj_array *fj) {
	short i;
	// Iterate through the left most square of the 3rd low to the 31st square.
	for(i = 7; i < BOARD_SIZE; i++) {
		if(player_pieces & (1 << i)) {
			// Check if piece is on even row.
			if((i / 4) % 2 == 0) {
				// Check if valid capture is available on non-4's diagonal.
				if(i % 8 != 0 && (opp_pieces & (1 << (i - 5))) && (empty_squares & (1 << (i - 9))))
					fj_append(fj, i, i - 5, i - 9);
				// Check if valid capture is available on 4's diagonal.
				if(i % 8 != 3 && (opp_pieces & (1 << (i - 4))) && (empty_squares & (1 << (i - 7))))
					fj_append(fj, i, i - 4, i - 7);
			}
			// If piece is on odd row.
			else {
				// Check if valid capture is available on non-4's diagonal.
				if(i > 11 && (i % 8 != 7 && (opp_pieces & (1 << (i - 3))) && (empty_squares & (1 << (i - 7)))))
					fj_append(fj, i, i - 3, i - 7);
				// Check if valid capture is available on 4's diagonal.
				if(i > 11 && (i % 8 != 4 && (opp_pieces & (1 << (i - 4))) && (empty_squares & (1 << (i - 9)))))
					fj_append(fj, i, i - 4, i - 9);
			}
		}
	}
	return;
}

// Check if forced jumps are available for a player in a particular board configuration.
void are_forced_jumps(board_t *b, fj_array *fj, player_t player) {
	uint32_t opp_pieces, player_pieces, empty_squares;
	
	empty_squares = ~(b -> player1_pawns | b -> player2_pawns | b -> player1_kings | b -> player2_kings);
	
	// With every function call reset size and index of fj_array.
	fj -> size = 0;
	fj -> index = -1;
	
	if(player == PLAYER_1) {
		// Create bitboard for opponent's pieces (here Player 2).
		opp_pieces = b -> player2_pawns | b -> player2_kings;
		player_pieces = b -> player1_pawns | b -> player1_kings;
		
		// Find if any forced jumps are available for Player 1.
		find_forward_fj(player_pieces, opp_pieces, empty_squares, fj);
		
		// If Player 1 has kings, check forced jumps for them as well.
		if(b -> player1_kings)
			find_reverse_fj(b -> player1_kings, opp_pieces, empty_squares, fj);
	}
	else if(player == PLAYER_2) {
		// Create bitboard for opponent's pieces (here Player 1).
		opp_pieces = b -> player1_pawns | b -> player1_kings;
		player_pieces = b -> player2_pawns | b -> player2_kings;
		
		// Find if any forced jumps are available for Player 2.
		find_reverse_fj(player_pieces, opp_pieces, empty_squares, fj);
		
		// If Player 2 has kings, check forced jumps for them as well.
		if(b -> player2_kings)
			find_forward_fj(b -> player2_kings, opp_pieces, empty_squares, fj);
	}
	return;
}

// Used in case of chain / multiple forced jumps
// To prune forced jumps which have different origin than the end index of last jump.
void prune_fj(fj_array *fj, unsigned short from_index) {
	short j = 0;
	for(short i = 0; i < fj -> size; i++) {
		if(fj -> jumps[i].from_index == from_index) {
			fj -> jumps[j] = fj -> jumps[i];
			j++;
		}
	}
	fj -> size = j;
}

// Scrolls to a fresh terminal window.
void clear_screen() {
    printf("\033[2J");
    printf("\033[H");
    return;
}

// Prints the player to play.
void print_player(player_t player) {
    if(player == PLAYER_1)
        printf("\nIts Player X's turn\n\n\n");
    else
        printf("\nIts Player O's turn\n\n\n");
    return;
}

// Prompts the player to enter a valid move.
void get_move(board_t *board, unsigned short *from_index, unsigned short *to_index, fj_array *fj, player_t player) {
	bool move_status;
    do {
        do {
            printf("From: ");
            *from_index = get_index();
            if (*from_index == INVALID)
                printf("Invalid move. Please enter again.\n");
            if(*from_index > INVALID)
            	return;
        } while (*from_index == INVALID); // Repeat until a valid "from" index is entered
        
        do {
            printf("To: ");
            *to_index = get_index();
            if (*to_index == INVALID)
                printf("Invalid move. Please enter again.\n");
            if(*to_index > INVALID)
            	return;
        } while (*to_index == INVALID);
        
        move_status = is_valid_move(board, player, *from_index, *to_index, fj);
        if(!move_status)
        	printf("Illegal Move, please enter again.\n");
    } while (!move_status);
    return;
}

// Returns a 32-indexed index from a given 0-7 indexed row and column.
unsigned short get_index(void) {
	char *input = (char *)calloc(4, sizeof(char));
	scanf("%s", input);
	
	// Special cases, return corresponding macros for further action.
	if(strcmp(input, "undo") == 0)
		return UNDO;
	else if(strcmp(input, "redo") == 0)
		return REDO;
	else if(strcmp(input, "quit") == 0)
		return QUIT;
		
	char col = tolower(input[0]);
	char row = input[1];
	short colin = (int)col - 96;
	short rolin = (int)row - '0';
	free(input);
	return interpreter(colin, rolin);
}

// Informs player about avaiable forced jumps.
void inform_fj(fj_array *fj) {
	if(!is_fj_empty(fj)) {
		printf("\n");
		printf("Forced jump available : ");
		for(int i = 0; i < fj -> size; i++) {
			if(i > 0)
				printf(", ");
			printf("%c", (char)(get_col(fj -> jumps[i].from_index) + 97));
			printf("%d", (get_row(fj -> jumps[i].from_index) + 1));
			printf(" -> ");
			printf("%c", (char)(get_col(fj -> jumps[i].to_index) + 97));
			printf("%d", (get_row(fj -> jumps[i].to_index) + 1));
		}
		printf(".\n");
	}
	return;
}

// Get description of move chosen by minimax algorithm
void inform_minimax_move(board_t *prev, board_t *next, player_t player) {
	uint32_t prev_player_pieces = player == PLAYER_1 ? prev -> player1_pawns | prev -> player1_kings : prev -> player2_pawns | prev -> player2_kings;
	uint32_t prev_opp_pieces = player == PLAYER_2 ? prev -> player1_pawns | prev -> player1_kings : prev -> player2_pawns | prev -> player2_kings;
	uint32_t next_player_pieces = player == PLAYER_1 ? next -> player1_pawns | next -> player1_kings : next -> player2_pawns | next -> player2_kings;
	uint32_t next_opp_pieces = player == PLAYER_2 ? next -> player1_pawns | next -> player1_kings : next -> player2_pawns | next -> player2_kings;
	short index, from_index, to_index, str_index = 0;
	
	uint32_t captured_pieces = prev_opp_pieces ^ next_opp_pieces;
	uint32_t moved_piece = prev_player_pieces ^ next_player_pieces;
	
	while(moved_piece) {
		index = __builtin_ctz(moved_piece);
		if(next_player_pieces & 1U << index)
			to_index = index;
		else
			from_index = index;
		moved_piece &= ~(1 << index);
	}
	printf("%c", (char)(get_col(from_index) + 97));
	printf("%d", (get_row(from_index) + 1));
	printf(" -> ");
	printf("%c", (char)(get_col(to_index) + 97));
	printf("%d\n", (get_row(to_index) + 1));
	
	if(captured_pieces)
		printf("Capture Indices : ");
	
	while(captured_pieces) {
		index = __builtin_ctz(captured_pieces);
		captured_pieces &= ~(1 << index);
		printf("%c", (char)(get_col(index) + 97));
		printf("%d", (get_row(index) + 1));
		if(captured_pieces)
			printf(", ");
		else
			printf("\n\n");
	}
	return;
}

void inform_ai_move(board_t *board, board_t *ai_move) {
	printf("\n\nAI move : ");
	inform_minimax_move(board, ai_move, PLAYER_2);
	return;
}

// Main function to facilitate game play between two human players.
void two_player(void) {
	board_t board;
	game_history history;
	fj_array fj;
	player_t current_player = PLAYER_1;
	unsigned short action = TRUE;
	
	// Initialize all required structures.
	init_board(&board, &history);
	init_fj_array(&fj);
	save_progress(&board, &history, current_player);
	
	// loop till move_action returns a FALSE.
	while(action) {
		// Clear the terminal window and display the current player and board.
		clear_screen();
		print_player(current_player);
		display_board(&board);
		
		// Inform player about any forced jumps if available.
		inform_fj(&fj);
		printf("\n\n");
		
		// Execute move_action in a loop to execute moves and update board.
		action = move_action(&board, &history, &fj, &current_player);
		
		// Special action requests:
		if(action == UNDO)
			undo_action(&board, &history, &fj, &current_player); // Undo, reverts back to previous board state.
		else if(action == REDO)
			redo_action(&board, &history, &fj, &current_player); // Redo, reverts actions of undo.
		else if(action == QUIT) {
			funeral(&board, &history, &fj, QUIT); // Quits the game.
			return;
		}
	}
	return;
}

// Implement human-human play.
unsigned short human_move(board_t *board, game_history *history, fj_array *fj, player_t *current_player) {
		unsigned short from_index, to_index, game_status;

		get_move(board, &from_index, &to_index, fj, *current_player);
		if(from_index > INVALID || to_index > INVALID) {
			if(from_index < INVALID)
				return to_index;
			return from_index;
		}

		*board = generate_bitboard(*board, from_index, to_index, *current_player);

		game_status = is_game_over(board, *current_player);
		if(game_status != 0) {
			funeral(board, history, fj, game_status);
			return FALSE;
		}

		if(!is_fj_empty(fj)) {
			are_forced_jumps(board, fj, *current_player);
			prune_fj(fj, to_index);
			if(is_fj_empty(fj))
				switch_player(current_player);
			else
				save_progress(board, history, PLAYER_1);
		}
		else
			switch_player(current_player);
		return TRUE;
}

// Implement human-computer play.
// Here PLAYER_2 is assumed as the AI player.
void AI_player(player_t starting_player) {
	board_t board, ai_move;
	game_history history;
	fj_array fj;
	player_t current_player = starting_player;
	unsigned short action = 1, game_status = 0;
	
	bool ai_moved = FALSE;

	init_board(&board, &history);
	init_fj_array(&fj);

	if(starting_player == PLAYER_1)
		save_progress(&board, &history, PLAYER_1);

	while(action) {
		clear_screen();
		display_board(&board);
		if(ai_moved) {
			inform_ai_move(&board, &ai_move);
			ai_moved = FALSE;
		}
		inform_fj(&fj);
		printf("\n\n");
		if(current_player == PLAYER_1) {
			action = human_move(&board, &history, &fj, &current_player);
		
			if(action == UNDO)
				undo_action(&board, &history, &fj, &current_player);
			else if(action == REDO)
				redo_action(&board, &history, &fj, &current_player);
			if(action == QUIT) {
				funeral(&board, &history, &fj, QUIT);
				break;
			}
		}

		if(current_player == PLAYER_2) {
			minimax(&board, DEPTH, SHRT_MIN, SHRT_MAX, PLAYER_2, &ai_move);
			game_status = is_game_over(&ai_move, current_player);
			if(game_status) {
				funeral(&board, &history, &fj, game_status);
				break;
			}
			switch_player(&current_player);
			save_progress(&ai_move, &history, PLAYER_1);
			are_forced_jumps(&ai_move, &fj, PLAYER_1);
			ai_moved = TRUE;
			board = ai_move;
		}
	}
	return;
}

// Static evaluation function
short evaluate_board(board_t *b, player_t player, unsigned short game_result) {
	// If game over, return following scores:
	if(game_result) {
		// If PLAYER_1 won the game:
		if(game_result == 1)
			return SHRT_MIN;
		// If PLAYER_2 won the game:
		else if(game_result == 2)
			return SHRT_MAX;
		// If match draw:
		else if(game_result == 3)
			return 0;
	}
	
	short score = 0;
	
	// Assign scores based on number of pieces
	score -= (__builtin_popcount(b -> player1_pawns) * PAWN_VALUE) + (__builtin_popcount(b -> player1_kings) * KING_VALUE);
	score += (__builtin_popcount(b -> player2_pawns) * PAWN_VALUE) + (__builtin_popcount(b -> player2_kings) * KING_VALUE);
	
	// Assign scores based on position
	score -= __builtin_popcount((b -> player1_pawns | b -> player1_kings) & CENTER_SQ) * CENTER_BONUS;
	score += __builtin_popcount((b -> player2_pawns | b -> player2_kings) & CENTER_SQ) * CENTER_BONUS;
	
	return score;
}

short minimax(board_t *b, unsigned short depth, short alpha, short beta, player_t player, board_t *best_move) {
	unsigned short game_status = is_game_over(b, player);
	if(depth == 0 || game_status)
		return evaluate_board(b, player, game_status);

	short score, best_score;
	
	// Initialize move_table.
	move_table mt;
	init_mt(&mt);
	
	// Initialize fj_array.
	fj_array fj;
	init_fj_array(&fj);
	
	// Generate all legal moves for minimax.
	generate_moves(b, &mt, &fj, player);
	
	// Switch player for minimax to be called next.
	player_t next_player = player;
	switch_player(&next_player);

	// Player 2 is assumed as maximizing player.
	if(player == PLAYER_2) {
		// Assign absolute minimum score at start.
		best_score = SHRT_MIN;
		for (int i = 0; i < mt.size; i++) {
			// recursively call minimax for next player.
			score = minimax(&(mt.children[i]), depth - 1, alpha, beta, next_player, NULL);
			if(score > best_score) {
				best_score = score;
				// for root call, return bitboard with best move.
				if(best_move)
					*best_move = mt.children[i];
			}
			alpha = (best_score > alpha) ? best_score : alpha;
			// if beta <= alpha, stop evaluating further.
			if (beta <= alpha) {
       			break;
			}
		}
	}

	// Player 1 is assumed as the minimizing player.
	else {
		// Assign absolute maximum score at start.
		best_score = SHRT_MAX;
		for (int i = 0; i < mt.size; i++) {
			// recursively call minimax for next player.
			score = minimax(&(mt.children[i]), depth - 1, alpha, beta, next_player, NULL);
			if(score < best_score) {
				best_score = score;
				// for root call, return bitboard with best move.
				if(best_move)
					*best_move = mt.children[i];
			}
			beta = (best_score < beta) ? best_score : beta;
			// if beta <= alpha, stop evaluating further.
			if (beta <= alpha) {
				break;
			}
		}
	}

	free(mt.children);
	free(fj.jumps);
    return best_score;
}

// Initialize the move_table.
void init_mt(move_table *mt) {
	mt -> children = (board_t *)malloc(sizeof(board_t) * MT_MAX_SIZE);
	mt -> size = 0;
	mt -> capacity = MT_MAX_SIZE;
	return;
}

// Reallocate memory for move_table if full.
void expand_mt_if_full(move_table *mt) {
	if(mt -> size == mt -> capacity) {
		mt -> capacity *= 2;
		mt -> children = (board_t *)realloc(mt -> children, sizeof(board_t) * mt -> capacity);
	}
	return;
}

board_t generate_bitboard(board_t b, unsigned short from_index, unsigned short to_index, player_t player) {
	uint32_t *from_pieces, *to_pieces;

	if(player == PLAYER_1) {
		if(b.player1_pawns & 1U << from_index) {
			from_pieces = &(b.player1_pawns);
			// Check for king promotions
			to_pieces = to_index > 27 ? &(b.player1_kings) : from_pieces;
		}
		else {
			from_pieces = &(b.player1_kings);
			to_pieces = from_pieces;
		}
	}
	else {
		if(b.player2_pawns & 1U << from_index) {
			from_pieces = &(b.player2_pawns);
			// Check for king promotions
			to_pieces = to_index < 4 ? &(b.player2_kings) : from_pieces;
		}
		else {
			from_pieces = &(b.player2_kings);
			to_pieces = from_pieces;
		}
	}

	*to_pieces |= 1U << to_index;
	*from_pieces &= ~(1U << from_index);
	return b;
}

// Generate all legal moves for a given board configuration.
void generate_moves(board_t *b, move_table *mt, fj_array *fj, player_t player) {
	// Check if any forced jumps are available for the given board.
	are_forced_jumps(b, fj, player);
	init_mt(mt);
	
	// If no forced jumps are available, call functions to append single moves to the move_table.
	if(is_fj_empty(fj)) {
		uint32_t empty_squares = ~(b -> player1_pawns | b -> player1_kings | b -> player2_pawns | b -> player2_kings);
		
		if(player == PLAYER_1) {
			uint32_t player_pieces = b -> player1_pawns | b -> player1_kings;
			if(b -> player1_kings)
				// Append all single legal moves for king moving in direction from 8th to 1st row
				append_reverse_moves(b, mt, b -> player1_kings, empty_squares, player);
			// Append all single legal moves for pieces moving from 1st to 8th row
			append_forward_moves(b, mt, player_pieces, empty_squares, player);
		}
		else if(player == PLAYER_2) {
			uint32_t player_pieces = b -> player2_pawns | b -> player2_kings;
			if(b -> player2_kings)
				// Append all single legal moves for king moving in direction from 1st to 8th row
				append_forward_moves(b, mt, b -> player2_kings, empty_squares, player);
			// Append all single legal moves for pieces moving from 8th to 1st row
			append_reverse_moves(b, mt, player_pieces, empty_squares, player);
		}
	}
	// If forced jumps are avaiable for the given board
	else {
		// Exclusively append all available forced jumps.
		generate_fj_moves(b, fj, mt, player);
	}
	return;
}

// To iterate through and append all avaiable forced jumps.
void generate_fj_moves(board_t *b, fj_array *fj, move_table *mt, player_t player) {
	short i;
	board_t temp_board;
	fj_array new_fj;
	init_fj_array(&new_fj);
	
	for(i = 0; i < fj -> size; i++) {
		temp_board = move_fj(*b, &(fj -> jumps[i]), player);
		are_forced_jumps(&temp_board, &new_fj, player);
		prune_fj(&new_fj, fj -> jumps[i].to_index);
		// If multiple forced jumps are available:
		if(new_fj.size > 0) {
			// Recursively call generate_fj_moves till no more forced jumps are available.	
			generate_fj_moves(&temp_board, &new_fj, mt, player);
		}
		// If no more forced jumps are available:
		else {
			// Append bitboard to move_table.
			expand_mt_if_full(mt);
			mt -> children[mt -> size] = temp_board;
			mt -> size++;
		}
	}
	
	free(new_fj.jumps);
	return;
}

// Generates all available board states for forced jumps.
board_t move_fj(board_t b, forced_jump *jump, player_t player) {
	uint32_t *from_pieces;
	uint32_t *to_pieces;
	uint32_t *opp_pieces;
	
	if(player == PLAYER_1) {
		// Check king promotions:
		if(b.player1_pawns & 1U << jump -> from_index) {
			from_pieces = &(b.player1_pawns);
			to_pieces = (jump -> to_index) > 27 ? &(b.player1_kings) : from_pieces;
		}
		else {
			from_pieces = &(b.player1_kings);
			to_pieces = from_pieces;
		}
		opp_pieces = b.player2_pawns & 1U << (jump -> capture_index) ? &(b.player2_pawns) : &(b.player2_kings);
	}
	else {
		// Check king promotions:
		if(b.player2_pawns & 1U << jump -> from_index) {
			from_pieces = &(b.player2_pawns);
			to_pieces = (jump -> to_index) < 4 ? &(b.player2_kings) : from_pieces;
		}
		else {
			from_pieces = &(b.player2_kings);
			to_pieces = from_pieces;
		}
		opp_pieces = b.player1_pawns & 1U << (jump -> capture_index) ? &(b.player1_pawns) : &(b.player1_kings);
	}
	
	// Flip concerned bits.
	*to_pieces |= 1U << jump -> to_index;
	*from_pieces &= ~(1U << jump -> from_index);
	*opp_pieces &= ~(1U << jump -> capture_index);
	return b;
}

// Find all legal single moves for pieces able to move from the 1st row to the 8th.
void append_forward_moves(board_t *board, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player) {
	short i;
	for(i = 27; i >= 0; i--) {
		if(player_pieces & (1 << i)) {
			expand_mt_if_full(mt);
			// Check if 4's diagonal is empty.
			if(empty_squares & (1 << (i + 4))) {
				mt -> children[mt -> size] = generate_bitboard(*board, i, i + 4, player);
				mt -> size++;
			}
			// Check if piece is on even row and if non-4's diagonal is empty.
			if((i / 4) % 2 == 0 && (i % 8 != 0 && (empty_squares & 1 << (i + 3)))) {
				mt -> children[mt -> size] = generate_bitboard(*board, i, i + 3, player);
				mt -> size++;
			}
			// Check if piece is on odd row and if non-4's diagonal is empty.
			if((i / 4) % 2 == 1 && (i % 8 != 7 && (empty_squares & 1 << (i + 5)))) {
				mt -> children[mt -> size] = generate_bitboard(*board, i, i + 5, player);
				mt -> size++;
			}
		}
	}
	return;
}

// Find all legal single moves for pieces able to move from the 8th row to the 1st.
void append_reverse_moves(board_t *board, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player) {
	short i;
	for(i = 4; i < BOARD_SIZE; i++) {
		if(player_pieces & (1 << i)) {
			expand_mt_if_full(mt);
			// Check if 4's diagonal is empty.
			if(empty_squares & (1 << (i - 4))) {
				mt -> children[mt -> size] = generate_bitboard(*board, i, i - 4, player);
				mt -> size++;
			}	
			// Check if piece is on even row and if non-4's diagonal is empty.
			if((i / 4) % 2 == 0 && (i % 8 != 0 && (empty_squares & 1 << (i - 5)))) {
				mt -> children[mt -> size] = generate_bitboard(*board, i, i - 5, player);
				mt -> size++;
			}
			// Check if piece is on odd row and if non-4's diagonal is empty.
			if((i / 4) % 2 == 1 && (i % 8 != 7 && (empty_squares & 1 << (i - 3)))) {
				mt -> children[mt -> size] = generate_bitboard(*board, i, i - 3, player);
				mt -> size++;
			}
		}
	}
	return;
}

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
