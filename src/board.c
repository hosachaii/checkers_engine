void init_board(board_t *board, game_history *history) {
	
	//Initialise checkers board to the start configuration.
	board -> player1_pawns = P1_PAWNS; // P1_PAWNS is a macro which contains the bit-positions of player1_pawns.
	board -> player2_pawns = P2_PAWNS; // P2_PAWNS is a macro which contains the bit-positions of player1_pawns.
	board -> player1_kings = 0; // No kings at the start.
	board -> player2_kings = 0; // No kings at the start.
	return;
	
	//Initialize game history stacks.
	history -> undo_top = NULL;
	history -> redo_top = NULL;
	return;
}

// Function used to display the board (visual representation) 
void display_board(board_t *board) {
	short interpreter;
	printf("  +---+---+---+---+---+---+---+---+\n");
    // Iterate over all square indices.
	for (short row = 8; row >= 1; row--) {
      // row labels from 1 to 8
    	printf("%d |", row);
      	for (short col = 1; col <= 8; col++) {
			index = interpreter(row, col)
			// If bit at position i is set high in player1_pawns:
        	if (board -> player1_pawns & (1 << i))
				printf("X |");
        	// If bit at position i is set high in player2_pawns:
        	else if (board -> player2_pawns & (1 << i))
				printf("O |");
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