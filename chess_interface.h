#include "chess_state.h"
class ChessInterface: public ChessState { // handles algebraic notation, can play from move list, handle human input
    public:
        ChessState state;
        void move(string str);
        void play_moves(vector<string> moves, bool verbose=true);
        bool one_play_input(int8_t verbose=2); // make the next move according to human input, return false if human quit
        void play_input(int8_t verbose=2); // keep moving according to input until "q"
        vector<string> all_notes(); // notations for all possible moves
    private:
        static int get_sq(string str);
        uint8_t attack_sq(uint8_t sq2, uint8_t piece, char type,int drow=SZ,int dcol = SZ);
        minfo not2minfo(string str);
        string minfo2not(minfo minfo);
};