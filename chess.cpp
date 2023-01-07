#include "chess_interface.h"
#include <sstream>
int main() {
    ChessInterface cgame;

    // string gmstr = "e4 d5 d3 dxe4 dxe4 Qxd1+ Kxd1 Nc6 Bd3 Bg4+ f3 Bh5 Be3 Bg6 Ke2 O-O-O Nc3 Nd4+ Kd2 e5 Bxd4 exd4 Nd5 Ne7 Nxe7+ Bxe7 Nh3 Bb4+ c3 dxc3+ bxc3 Ba5 a4 Rd7 Kc2 Rhd8 c4 Rxd3 Nf4 Rd2+ Kb3 f5 exf5 Bxf5 Rac1 g5 Nd5 c6 Nc3 Bxc3 Rxc3 Rxg2 Re1 Rgd2 Re5 Bg6 Rxg5 R2d3 Rxd3 Rxd3+ Kb4 Rxf3 h4 Rh3 Rg4 Bh5 Rg8+ Kd7 Rg7+ Ke6 Rxb7 Rxh4 Rxa7 Bg6 Ra6 Kd7 Ra7+ Kc8 Ra8+ Kb7 Rf8 Bd3";
    // stringstream ss(gmstr);
    // string mvstr;
    // vector<string> moves;
    // while(getline(ss,mvstr,' '))
    //     moves.push_back(mvstr);
    // cgame.play_moves(moves,true);

    cgame.play_input();
    return 0;
}