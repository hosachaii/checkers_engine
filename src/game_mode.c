#include "game_mode.h"

// Global variable to set game_mode
game_mode_t game_mode;

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