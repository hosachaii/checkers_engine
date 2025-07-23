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