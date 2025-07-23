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