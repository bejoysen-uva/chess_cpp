#include "chess_interface.h"
#include <sstream>
int main() {
    ChessInterface cgame;
    string gmstr = "d4 d5 Bf4 e6 e3 Bd6 Bg3 Bxg3 c3 Nc6 hxg3 Bd7 Bd3 Qf6 Nd2 g6 Ngf3 O-O-O a4 e5 dxe5 Nxe5 Bf1 Nxf3+";
    stringstream ss(gmstr);
    string mvstr;
    vector<string> moves;
    while(getline(ss,mvstr,' '))
        moves.push_back(mvstr);
    cgame.play_moves(moves);

    // cgame.play_input();
    return 0;
}