#include "chess_state.h"
int main() {
    ChessState cstate;
    vector<string> moves = {"e4","d5","exd5","c5","dxc6","a5","cxb7","a4","b8=Q+","Nf6","g4","Nxg4","Bc4"};
    cstate.play_moves(moves);
    return 0;
}