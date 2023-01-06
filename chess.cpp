#include "chess_interface.h"
#include <sstream>
int main() {
    ChessInterface cgame;
    // string gmstr = "e4 c5 d4 Nc6 Nf3 cxd4 Nxd4 Nxd4 Qxd4 d6 Qe3 e5 Bd3 Nf6 O-O Be7 Nc3 Bd7 Nd5 Bc6 Nxe7 Qxe7 c4 Rd8 b4 b6 b5 Bb7 a4 Ng4 Qg3 Nf6 Qf3 h6 a5 Qc7 a6 Ba8 Ba3 O-O Rfd1 Rd7 Bc2 Rfd8 Rd2 Qxc4 Rad1 Qe6 Qb3 Qg4 f3 Qg5 Qb2 d5 exd5 Nxd5 Bb3 Nf4 Rxd7 Rxd7 Rxd7 Bxf3 Rd2 Kh7 Rf2 g6 Rxf3 Ne2+ Qxe2 Qc1+ Rf1 Qxa3 Re1 Qc5+ Qe3 f6 Qxc5 Kg7 Rc1 bxc5 Rxc5 g5 Rc7+ Kg6 Rxa7 h5 Rb7 f5 a7 g4 a8=Q Kg5 Qc8 e4 Rd7 e3 Qe8 e2 Qxe2 f4 Qf2 f3 Rf7 fxg2 Rf3 gxf3 Qxf3 h4 Qxg2+";
    // stringstream ss(gmstr);
    // string mvstr;
    // vector<string> moves;
    // while(getline(ss,mvstr,' '))
    //     moves.push_back(mvstr);
    // cgame.play_moves(moves);

    cgame.play_input();
    return 0;
}