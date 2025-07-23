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
    current_board -> bitboard = *board;
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
	*board = p -> bitboard;

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
		*board = p -> bitboard;
		
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