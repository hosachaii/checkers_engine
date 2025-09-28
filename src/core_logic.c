#include "core_logic.h"

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
	if(!player_pieces & (1 << from_index)) {
		return FALSE;
	}

	// checking if the destination square is empty
	if (player_pieces | opp_pieces & (1 << to_index)) {
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
	if(fj -> index != 1) {
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