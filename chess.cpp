#include "chess_interface.h"
int main() {
    ChessInterface cgame;
    vector<string> moves = {"e4","e5",
                            "Nf3","Nc6",
                            "Bb5","Nf6",
                            "O-O","Ke7",
                            "Re1"};
    // vector<string> moves = {"f4","e6",
    //                         "g4","Qh4"};
    // cgame.play_moves(moves);
    cgame.play_input();
    return 0;
}