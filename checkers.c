#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "board.h"
#include "game_logic.h"
#include "game_loop.h"
#include "minimax.h"

// Global variable to set game_mode
game_mode_t game_mode;

void init_board(board_t *board, game_history *history) {
	unsigned short i = 0;
	
	//Initialise checkers board to the start configuration.
	board -> squares = (square_profile *)malloc(sizeof(square_profile) * BOARD_SIZE);
	while(i < BOARD_SIZE) {
		if(i < 12)
			board -> squares[i].player = PLAYER_1;
		else if(i > 19)
			board -> squares[i].player = PLAYER_2;
		else
			board -> squares[i].player = EMPTY;
		board -> squares[i].king = FALSE;
		i++;
	}
	
	//Initialize game history stacks.
	history -> undo_top = NULL;
	history -> redo_top = NULL;
	return;
}

// Initialize bitboard to stating configuration.
void init_bitboard(bitboard_t *b) {
	b -> player1_pawns = P1_PAWNS; // P1_PAWNS is a macro which contains the bit-positions of player1_pawns.
	b -> player2_pawns = P2_PAWNS; // P2_PAWNS is a macro which contains the bit-positions of player1_pawns.
	b -> player1_kings = 0; // No kings at the start.
	b -> player2_kings = 0; // No kings at the start.
	return;
}

/**/
unsigned short interpreter(unsigned short col, unsigned short row) {
	if(row % 2 == col % 2)
		return row % 2 ? (((row - 1) * 4) + (col / 2)) : (((row - 1) * 4) + (col / 2) - 1);
	else
		return INVALID;
}

// Function used to display the board (visual representation) 
void display_board(board_t *board) {
	short row, col;
	square_profile square;
    printf("  +---+---+---+---+---+---+---+---+\n");
    for (row = 8; row >= 1; row--) {
        // row labels from 1 to 8
        printf("%d |", row);
        for (col = 1; col <= 8; col++) {
            square = board -> squares[interpreter(col, row)]; //storing the location of piece at square
            if (square.player == PLAYER_1) {
                if (square.king)
                    printf(" KX|");  // king for PLAYER_1
                else
                    printf(" X |");  // normal pawn for PLAYER_1
            }
            else if (square.player == PLAYER_2) {
                if (square.king)
                    printf(" KO|");  // king for PLAYER_2
                else
                    printf(" O |");  // normal pawn for PLAYER_2
            } 
            else
                printf(" . |");  // empty square
        }
        printf("\n  +---+---+---+---+---+---+---+---+\n");
    }
    // column labels from A to H
    printf("    A   B   C   D   E   F   G   H  \n");
    return;
}

// Converts 1D array based representation into a bitboard.
bitboard_t arr_to_bitboard(board_t *board) {
	short i = 0;
	bitboard_t bitboard = {0};
	// Iterate over all squares.
	while(i < BOARD_SIZE) {
		if(board -> squares[i].player == PLAYER_1) {
			if(!board -> squares[i].king)
				bitboard.player1_pawns = bitboard.player1_pawns | 1 << i; // If square contains a pawn, set mapped bit to 1.
			else
				bitboard.player1_kings = bitboard.player1_kings | 1 << i; // If square contains a king, set mapped bit to 1.
		}
		else if(board -> squares[i].player == PLAYER_2) {
			if(!board -> squares[i].king)
				bitboard.player2_pawns = bitboard.player2_pawns | 1 << i; // If square contains a pawn, set mapped bit to 1.
			else
				bitboard.player2_kings = bitboard.player2_kings | 1 << i; // If square contains a king, set mapped bit to 1.
		}
		i++;
	}
	return bitboard;
}

// Converts bitboard based representation back to 1D array.
void bitboard_to_arr(bitboard_t *bitboard, board_t *board) {
    short i = 0;
    // Iterate over all square indices.
    while (i < BOARD_SIZE) {
    	// If bit at position i is set high in player1_pawns:
        if (bitboard -> player1_pawns & (1 << i)) {
            board -> squares[i].player = PLAYER_1;
            board -> squares[i].king = FALSE;
        } 
        // If bit at position i is set high in player2_pawns:
        else if (bitboard -> player2_pawns & (1 << i)) {
            board -> squares[i].player = PLAYER_2;
            board -> squares[i].king = FALSE;
        }
        // If bit at position i is set high in player1_kings:
        else if (bitboard -> player1_kings & (1 << i)) {
            board -> squares[i].player = PLAYER_1;
            board -> squares[i].king = TRUE;
        }
        // If bit at position i is set high in player2_kings: 
        else if (bitboard -> player2_kings & (1 << i)) {
            board -> squares[i].player = PLAYER_2;
            board -> squares[i].king = TRUE;
        }
        // If bit is set low for all bitboards, mark as empty.
        else {
            board -> squares[i].player = EMPTY;
            board -> squares[i].king = FALSE;
        }
        i++;
    }
    return;
}

// Initialize board_state for undo-redo stack.
void init_board_state(board_state *bs) {
	bs -> next = NULL;
	return;
}

// Purge redo stack.
void free_redo(game_history *history) {
	if(is_redo_empty(history))
		return;
	board_state *p = history -> redo_top;
	while(p != NULL) {
		history -> redo_top = p -> next;
		free(p);
		p = history -> redo_top;
	}
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

// Save board state for undo-redo purposes.
void save_progress(bitboard_t *bitboard, game_history *history, player_t current_player) {
    // Allocate memory for the new board state.
    board_state *current_board = (board_state *)malloc(sizeof(board_state));

    // Initialize and configure board state.
    init_board_state(current_board);
    current_board -> bitboard = *bitboard;
    current_board -> player_turn = current_player;

    // Purge redo stack.
    if (!is_redo_empty(history))
        free_redo(history);

    // Push latest board state onto undo stack.
    current_board -> next = history -> undo_top;
    history -> undo_top = current_board;

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
	
	p = history -> undo_top;
	// Set board corresponding to undo_top.
	bitboard_to_arr(&(p -> bitboard), board);
	// Find if any forced jumps are available for the current_player.
	are_forced_jumps(&(p -> bitboard), fj, *current_player);
	
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
		bitboard_to_arr(&(p -> bitboard), board);
		
		// If current board state is the latest board state
		if(is_redo_empty(history)) {
			// Calculate if any forced jumps available for current player.
			are_forced_jumps(&(p -> bitboard), fj, p -> player_turn);
			// If no forced jumps are available and two player game is being played, switch players.
			if(is_fj_empty(fj) && game_mode == TWO_PLAYER) {
				switch_player(current_player);
				// Find forced jumps for the next player.
				are_forced_jumps(&(p -> bitboard), fj, p -> player_turn);
			}
			else
				// If forced jumps are available for current player:
				*current_player = p -> player_turn;
		}
		// If redo stack is not empty, set board state corresponding to redo_top.
		else {
			*current_player = history -> redo_top -> player_turn;
			are_forced_jumps(&(p -> bitboard), fj, *current_player);
		}
		return TRUE;
	}
}

// Function called at the end of the game to free all allocated memory
// And to print the final board configuration and game result.
void funeral(board_t *board, game_history *history, fj_array *fj, unsigned short game_state) {
	// Display final baord configuration.
	clear_screen();
	display_board(board);
	printf("\n============ GAME OVER ============\n\n");
	// Free allocated memory.
	free(board -> squares);
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
	else if(game_state == 3)
		printf("Draw.\n");
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

//in this function, no pieces are moved. only checks validity of the movements
bool is_valid_move(board_t *board, player_t player, unsigned short from_index, unsigned short to_index, fj_array *fj) {

	// Check if indices are valid or not.
	if(from_index > LAST_INDEX || to_index > LAST_INDEX) {
		return FALSE;
	}

	// Checking if player has chosen their own pieces.
	if(board -> squares[from_index].player != player) {
		return FALSE;
	}

	// checking if the destination square is empty
	if (board -> squares[to_index].player != EMPTY) {
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

     // calculating 2D coordinates of concerned squares.
    unsigned short from_x = get_col(from_index);
    unsigned short from_y = get_row(from_index);
    unsigned short to_x = get_col(to_index);
    unsigned short to_y = get_row(to_index);

    short dx = to_x - from_x;
    short dy = to_y - from_y;

    square_profile from_square = board -> squares[from_index];

    // for a regular piece(not a king)
    if (!from_square.king) {
        // PLAYER_1 can only move downwards and PLAYER_2 can only move upwards (granted there is no king)
        if ((from_square.player == PLAYER_1 && dy != 1) || (from_square.player == PLAYER_2 && dy != -1)) {
            return FALSE;  // invalid movement
        }
    }

    // simple singular square movement
    if (abs(dx) == 1 && abs(dy) == 1) {
        return TRUE;
    }
    
    return FALSE;
}

//used exclusively for moving pieces. it doesn't check any specific cases.
void move_piece(board_t *board, unsigned short from_index, unsigned short to_index, fj_array *fj) {

    // moving piece
    board -> squares[to_index] = board -> squares[from_index];

    //emptying out piece's old position.
    board -> squares[from_index].player = EMPTY;
    board -> squares[from_index].king = FALSE;
    
    if(fj -> index != -1)
     {
    	//emptying out captured piece's position.
    	unsigned short capture_index = fj -> jumps[fj -> index].capture_index;
        board -> squares[capture_index].player = EMPTY;
        board -> squares[capture_index].king = FALSE;
    }
    return;
}

// To check and promote a pawn to king.
void kinging(board_t *board, unsigned short index) {
    square_profile *square = &(board -> squares[index]);

    //if PLAYER_1 reached the last row, it gets promoted to king
    if (square -> player == PLAYER_1 && index > 27) {
        square -> king = TRUE;  
    }
    //if PLAYER_2 reached the first row, it gets promoted to king
    else if (square -> player == PLAYER_2 && index < 4) {
        square -> king = TRUE;  
    }
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
void are_forced_jumps(bitboard_t *b, fj_array *fj, player_t player) {
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
		if(b -> player1_kings)
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

// Prints the player to play.
void print_player(player_t player) {
    if(player == PLAYER_1)
        printf("\nIts Player X's turn\n\n\n");
    else
        printf("\nIts Player O's turn\n\n\n");
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
				if((i / 4) % 2 == 0) {
				// Check if non 4's diagonal is empty.
					if(i % 8 != 0 && (empty_squares & 1 << (i + 3)))
						return 1;
					// Check if valid capture possible on non 4's diagonal.
					else if(i < 20 && (i % 8 != 0 && (opp_pieces & (1 << (i + 3))) && (empty_squares & (1 << (i + 7)))))
						return 1;
					// Check if valid capture possible on 4's diagonal.
					if(i < 20 && (i % 8 != 3 && (opp_pieces & (1 << (i + 4))) && (empty_squares & (1 << (i + 9)))))
						return 1;
				}
				// Check if pawn on odd rows.
				else if((i / 4) % 2 == 1) {
					// Check if non 4's diagonal is empty.
					if(i % 8 != 7 && (empty_squares & 1 << (i + 5)))
						return 1;
					// Check if valid capture possible on non 4's diagonal.
					else if(i < 24 && (i % 8 != 7 && (opp_pieces & (1 << (i + 5))) && (empty_squares & (1 << (i + 9)))))
						return 1;
					// Check if valid capture possible on 4's diagonal.
					if(i < 24 && (i % 8 != 4 && (opp_pieces & (1 << (i + 4))) && (empty_squares & (1 << (i + 7)))))
						return 1;
				}
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
				if((i / 4) % 2 == 0) {
					// Check if non 4's diagonal is empty.
					if((i % 8 != 0) && (empty_squares & 1 << (i - 5)))
						return 1;
					// Check if valid capture possible on non 4's diagonal.
					else if(i > 7 && (i % 8 != 0 && (opp_pieces & (1 << (i - 5))) && (empty_squares & (1 << (i - 9)))))
						return 1;
					// Check if valid capture possible on 4's diagonal.
					if(i > 7 && (i % 8 != 3 && (opp_pieces & (1 << (i - 4))) && (empty_squares & (1 << (i - 7)))))
						return 1;
				}
				else if((i / 4) % 2 == 1) {
					// Check if non 4's diagonal is empty.
					if((i % 8 != 7) && (empty_squares & 1 << (i - 3)))
						return 1;
					// Check if valid capture possible on non 4's diagonal.
					else if(i > 11 && (i % 8 != 7 && (opp_pieces & (1 << (i - 3))) && (empty_squares & (1 << (i - 7)))))
						return 1;
					// Check if valid capture possible on 4's diagonal.
					if(i > 11 && (i % 8 != 4 && (opp_pieces & (1 << (i - 4))) && (empty_squares & (1 << (i - 9)))))
						return 1;
				}
			}
		}
	}
	return 0;
}

// Function to check if game is over.
unsigned short is_game_over(bitboard_t *bitboard) {
	if(bitboard -> player1_pawns == 0 && bitboard -> player1_kings == 0)
		return 2;
	else if (bitboard -> player2_pawns == 0 && bitboard -> player2_kings == 0)
		return 1;
	else {
		bool player1_moves_left = FALSE;
		bool player2_moves_left = FALSE;
		uint32_t empty_squares = ~(bitboard -> player1_pawns | bitboard -> player2_pawns | bitboard -> player1_kings | bitboard -> player2_kings);
		uint32_t player1_pieces = bitboard -> player1_pawns | bitboard -> player1_kings;
		uint32_t player2_pieces = bitboard -> player2_pawns | bitboard -> player2_kings;

		// For player 1 moves.
		if(bitboard -> player1_pawns)
			player1_moves_left = forward_moving_pieces(bitboard -> player1_pawns, player2_pieces, empty_squares);

		// Check if kings have moves left if there are no playable pawns.
		if(bitboard -> player1_kings && player1_moves_left == FALSE)
			player1_moves_left = reverse_moving_pieces(bitboard -> player1_kings, player2_pieces, empty_squares)
				|| forward_moving_pieces(bitboard -> player1_kings, player2_pieces, empty_squares);

		// For player 2 moves.
		if(bitboard -> player2_pawns)
			player2_moves_left = reverse_moving_pieces(bitboard -> player2_pawns, player1_pieces, empty_squares);

		// Check if kings have moves left if there are no playable pawns.
		if(bitboard -> player2_kings && player2_moves_left == FALSE)
			player2_moves_left = forward_moving_pieces(bitboard -> player2_kings, player1_pieces, empty_squares)
				|| reverse_moving_pieces(bitboard -> player2_kings, player1_pieces, empty_squares);

		if(player1_moves_left && player2_moves_left)
			return 0; // game not over yet.
		else if(player1_moves_left && !player2_moves_left)
			return 1; // player 1 won the game.
		else if(!player1_moves_left && player2_moves_left)
			return 2; // player 2 won the game.
		else
			return 3; // possible draw.
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

// Scrolls to a fresh terminal window.
void clear_screen() {
    printf("\033[2J");
    printf("\033[H");
    return;
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

// Main function to execute moves provided by the player and to update the board.
unsigned short move_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player) {
		unsigned short from_index, to_index, game_status;
		bitboard_t bitboard;
		
		// prompts the player to enter a valid move.
		get_move(board, &from_index, &to_index, fj, *current_player);
		
		// Special action requests are returned to the caller to be handled.
		if(from_index > INVALID || to_index > INVALID) {
			if(from_index < INVALID)
				return to_index;
			return from_index;
		}
		// move the desired piece.
		move_piece(board, from_index, to_index, fj);
		// check if the move results in a promotion.
		kinging(board, to_index);
		
		// Get the bitboard based representation.
		bitboard = arr_to_bitboard(board);
		// Update the game_history stacks.
		save_progress(&bitboard, history, *current_player);
		
		// Check if game is over:
		game_status = is_game_over(&bitboard);
		if(game_status != 0) {
			funeral(board, history, fj, game_status);
			return FALSE;
		}
		
		// Check if current move was a capture:
		if(!is_fj_empty(fj)) {
			// If yes, Check if additional forced jumps are available for the current player.
			are_forced_jumps(&bitboard, fj, *current_player);
			prune_fj(fj, to_index);
			// If there are additional forced jumps, don't switch the current player.
			if(is_fj_empty(fj)) {
				// If no additional forced jumps are available, switch player
				// And find forced jumps for the next player.
				switch_player(current_player);
				are_forced_jumps(&bitboard, fj, *current_player);
			}
		}
		// If current move was not a capture:
		else {
			// Switch player and find forced jumps for the next player.
			switch_player(current_player);
			are_forced_jumps(&bitboard, fj, *current_player);
		}
		return TRUE;
}

// Main function to facilitate game play between two human players.
void two_player(void) {
	board_t board;
	bitboard_t bitboard;
	game_history history;
	fj_array fj;
	player_t current_player = PLAYER_1;
	unsigned short action = TRUE;
	
	// Initialize all required structures.
	init_board(&board, &history);
	init_fj_array(&fj);
	init_bitboard(&bitboard);
	save_progress(&bitboard, &history, current_player);
	
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

// Initialize the move_table.
void init_mt(move_table *mt) {
	mt -> children = (bitboard_t *)malloc(sizeof(bitboard_t) * MT_MAX_SIZE);
	mt -> size = 0;
	mt -> capacity = MT_MAX_SIZE;
	return;
}

// Reallocate memory for move_table if full.
void expand_mt_if_full(move_table *mt) {
	if(mt -> size == mt -> capacity) {
		mt -> capacity *= 2;
		mt -> children = (bitboard_t *)realloc(mt -> children, sizeof(bitboard_t) * mt -> capacity);
	}
	return;
}

// Generate all legal moves for a given board configuration.
void generate_moves(bitboard_t *b, move_table *mt, fj_array *fj, player_t player) {
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
void generate_fj_moves(bitboard_t *b, fj_array *fj, move_table *mt, player_t player) {
	short i;
	bitboard_t temp_bitboard;
	fj_array new_fj;
	init_fj_array(&new_fj);
	
	for(i = 0; i < fj -> size; i++) {
		temp_bitboard = move_fj(*b, &(fj -> jumps[i]), player);
		are_forced_jumps(&temp_bitboard, &new_fj, player);
		prune_fj(&new_fj, fj -> jumps[i].to_index);
		// If multiple forced jumps are available:
		if(new_fj.size > 0) {
			// Recursively call generate_fj_moves till no more forced jumps are available.	
			generate_fj_moves(&temp_bitboard, &new_fj, mt, player);
		}
		// If no more forced jumps are available:
		else {
			// Append bitboard to move_table.
			expand_mt_if_full(mt);
			mt -> children[mt -> size] = temp_bitboard;
			mt -> size++;
		}
	}
	
	free(new_fj.jumps);
	return;
}

// Generates all available board states for forced jumps.
bitboard_t move_fj(bitboard_t b, forced_jump *jump, player_t player) {
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
void append_forward_moves(bitboard_t *bitboard, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player) {
	short i;
	for(i = 27; i >= 0; i--) {
		if(player_pieces & (1 << i)) {
			expand_mt_if_full(mt);
			// Check if 4's diagonal is empty.
			if(empty_squares & (1 << (i + 4))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i + 4, player);
				mt -> size++;
			}
			// Check if piece is on even row and if non-4's diagonal is empty.
			if((i / 4) % 2 == 0 && (i % 8 != 0 && (empty_squares & 1 << (i + 3)))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i + 3, player);
				mt -> size++;
			}
			// Check if piece is on odd row and if non-4's diagonal is empty.
			if((i / 4) % 2 == 1 && (i % 8 != 7 && (empty_squares & 1 << (i + 5)))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i + 5, player);
				mt -> size++;
			}
		}
	}
	return;
}

// Find all legal single moves for pieces able to move from the 8th row to the 1st.
void append_reverse_moves(bitboard_t *bitboard, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player) {
	short i;
	for(i = 4; i < BOARD_SIZE; i++) {
		if(player_pieces & (1 << i)) {
			expand_mt_if_full(mt);
			// Check if 4's diagonal is empty.
			if(empty_squares & (1 << (i - 4))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i - 4, player);
				mt -> size++;
			}	
			// Check if piece is on even row and if non-4's diagonal is empty.
			if((i / 4) % 2 == 0 && (i % 8 != 0 && (empty_squares & 1 << (i - 5)))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i - 5, player);
				mt -> size++;
			}
			// Check if piece is on odd row and if non-4's diagonal is empty.
			if((i / 4) % 2 == 1 && (i % 8 != 7 && (empty_squares & 1 << (i - 3)))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i - 3, player);
				mt -> size++;
			}
		}
	}
	return;
}

// Append bitboard to move_table for single moves.
bitboard_t generate_bitboard(bitboard_t b, unsigned short from_index, unsigned short to_index, player_t player) {
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
			to_pieces = to_index < 4 ? &(b.player1_kings) : from_pieces;
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

// Static evaluation function
short evaluate_board(bitboard_t *b, player_t player, unsigned short game_result) {
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

short minimax(bitboard_t *b, unsigned short depth, short alpha, short beta, player_t player, bitboard_t *best_move) {
	unsigned short game_status = is_game_over(b);
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

// Implement human-human play.
unsigned short human_move(board_t *board, game_history *history, fj_array *fj, player_t *current_player, bitboard_t *bitboard) {
		unsigned short from_index, to_index, game_status;

		get_move(board, &from_index, &to_index, fj, *current_player);
		if(from_index > INVALID || to_index > INVALID) {
			if(from_index < INVALID)
				return to_index;
			return from_index;
		}
		move_piece(board, from_index, to_index, fj);
		kinging(board, to_index);

		*bitboard = arr_to_bitboard(board);

		game_status = is_game_over(bitboard);
		if(game_status != 0) {
			funeral(board, history, fj, game_status);
			return FALSE;
		}

		if(!is_fj_empty(fj)) {
			are_forced_jumps(bitboard, fj, *current_player);
			prune_fj(fj, to_index);
			if(is_fj_empty(fj))
				switch_player(current_player);
			else
				save_progress(bitboard, history, PLAYER_1);
		}
		else
			switch_player(current_player);
		return TRUE;
}

// Get description of move chosen by minimax algorithm
void inform_minimax_move(bitboard_t *prev, bitboard_t *next, player_t player) {
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

void inform_ai_move(bitboard_t *bitboard, bitboard_t *ai_move) {
	printf("\n\nAI move : ");
	inform_minimax_move(bitboard, ai_move, PLAYER_2);
	return;
}

// Implement human-computer play.
// Here PLAYER_2 is assumed as the AI player.
void AI_player(player_t starting_player) {
	board_t board;
	game_history history;
	fj_array fj;
	bitboard_t bitboard, ai_move;
	player_t current_player = starting_player;
	unsigned short action = 1, game_status = 0;
	
	bool ai_moved = FALSE;

	init_board(&board, &history);
	init_bitboard(&bitboard);
	init_fj_array(&fj);

	if(starting_player == PLAYER_1)
		save_progress(&bitboard, &history, PLAYER_1);

	while(action) {
		clear_screen();
		display_board(&board);
		if(ai_moved) {
			inform_ai_move(&bitboard, &ai_move);
			ai_moved = FALSE;
		}
		inform_fj(&fj);
		printf("\n\n");
		if(current_player == PLAYER_1) {
			action = human_move(&board, &history, &fj, &current_player, &bitboard);
		
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
			minimax(&bitboard, DEPTH, SHRT_MIN, SHRT_MAX, PLAYER_2, &ai_move);
			game_status = is_game_over(&ai_move);
			if(game_status) {
				funeral(&board, &history, &fj, game_status);
				break;
			}
			switch_player(&current_player);
			save_progress(&ai_move, &history, PLAYER_1);
			are_forced_jumps(&ai_move, &fj, PLAYER_1);
			ai_moved = TRUE;
			bitboard_to_arr(&ai_move, &board);
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
