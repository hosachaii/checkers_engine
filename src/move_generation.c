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