#include "chess_interface.h"
#include <sstream>
int main() {
    ChessInterface cgame;
    // string gmstr = "e4 e6 d4 Qf6 e5 Qg6 Nf3 Nc6 Bd3 Qxg2 Rg1 Qh3 Rg3 Qh5 Nc3 Nb4 a3 Nxd3+ Qxd3 b6 Rg5 Qh3 Bf4 Bb7 O-O-O Bxf3 Rg3 Qh5 Rxf3 f6 Rh3 Qf7 exf6 Nxf6 Be5 h5 Bxc7 Ng4 Rf3 Qe7 h3 Nf6 Be5 Nd5 Nxd5 exd5 Re1 O-O-O";
    // stringstream ss(gmstr);
    // string mvstr;
    // vector<string> moves;
    // while(getline(ss,mvstr,' '))
    //     moves.push_back(mvstr);
    // cgame.play_moves(moves);

    cgame.play_input();
    return 0;
}