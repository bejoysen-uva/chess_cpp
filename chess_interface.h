#include "chess_state.h"
class ChessInterface: public ChessState { // handles algebraic notation, can play from move list, handle human input
    public:
        ChessInterface();
        void move(minfo mv);
        void play_moves(vector<string> moves, bool verbose=true);
        bool one_play_input(int8_t verbose=2); // make the next move according to human input, return false if human quit
        void play_input(int8_t verbose=2); // keep moving according to input until "q"
    private:
        ChessState copy1;
        ChessState copy2;
        map<string,minfo> not2move; // maps notations to legal moves
        void generate_notes();
};