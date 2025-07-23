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