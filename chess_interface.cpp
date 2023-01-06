#include "chess_interface.h"
#include <sstream>

ChessInterface::ChessInterface() {
    not2move = {};
    copy1 = *this;
    copy2 = *this;
    generate_notes();
}
void ChessInterface::move(minfo mv) {
    execute_move(mv);
    copy1.execute_move(mv);
    copy2.execute_move(mv);
    generate_notes();
}
void ChessInterface::play_moves(vector<string> moves, bool verbose) {
    for(string str: moves) {
        if ((active==WT)&&verbose)
            cout << fmove << endl;
        if(not2move.count(str)==0)
            throw invalid_argument(str+" not in the move list");
        cout << "playing " << str << endl;
        move(not2move[str]);
        if(verbose) {
            print_board();
            cout << endl;
        }
    }
}

bool ChessInterface::one_play_input(int8_t verbose) {
    // verbose=0: no feedback
    // verbose=1: display board after each move
    // verbose=2: display possible moves for player
    if ((active==WT)&&verbose)
        cout << fmove << endl;
    if(verbose) {
        print_board();
        cout << endl;
    }

    if(verbose==2) {
        cout << "All moves: [";
        for(auto it = not2move.begin(); it!=not2move.end(); it++) {
            cout << (it==not2move.begin() ? "":",") << it->first;
        }
        cout << "]" << endl;
    }
    string anot;
    cout << "Move: ";
    cin >> anot;
    cout << endl;
    
    if(anot=="q")
        return false;
    if(not2move.count(anot)) {
        move(not2move[anot]);
        cout << "playing " << anot << endl;
    }
    else
        throw invalid_argument(anot+" is not in the move dict.");
    return true;
}
void ChessInterface::play_input(int8_t verbose) {
    // verbose=0: no feedback
    // verbose=1: display board after each move
    // verbose=2: display possible moves for player
    while(one_play_input(verbose)) {}
}

void ChessInterface::generate_notes() {
    not2move.clear();
    vector<minfo> mlist;
    all_legal_moves(mlist,&copy1);
    // p2minfo for disambiguation: if multiple white knights are going to sq2, then map[WN<<8+sq2] contains both their starting squares.
    map<uint16_t,vector<uint8_t>> p2minfo;
    for(minfo minf: mlist) { // for disambiguation
        uint8_t psq1 = board[minf.sq1/SZ][minf.sq1%SZ];
        p2minfo[(psq1<<8) + minf.sq2].push_back(minf.sq1);
    }
    stringstream note;
    for(minfo minf: mlist) {
        note.str("");

        uint8_t r1 = minf.sq1/SZ;
        uint8_t c1 = minf.sq1%SZ;
        uint8_t r2 = minf.sq2/SZ;
        uint8_t c2 = minf.sq2%SZ;
        uint8_t psq1 = board[r1][c1];
        uint8_t psq2 = board[r2][c2];

        // castling
        if((minf.castle)==KCAST)
            note << "O-O";
        else if((minf.castle)==QCAST)
            note << "O-O-O";
        // pawn moves
        else if((psq1==WP)||(psq1==BP)) {
            if(c1==c2) {
                // pawn moves forward, does not promote (d6,e4)
                if(psq1==minf.newp)
                    note << cols[c2] << int(SZ-r2);
                // pawn moves forward, promotes (e8=Q)
                else
                    note << cols[c2] << int(SZ-r2) << "=" << map_type(minf.newp);
            } else {
                // pawn captures (exd5), en passant included
                if(psq1==minf.newp)
                    note << cols[c1] << "x" << cols[c2] << int(SZ-r2);
                // pawn moves forward, promotes (e8=Q)
                else {
                    note << cols[c1] << "x" << cols[c2] << int(SZ-r2) << "=" << map_type(minf.newp);
                }
            }
        } 
        // any other piece (NBRQK)
        else {
            // piece name
            note << map_type(psq1);
            const auto& other_sqs = p2minfo[(psq1<<8)+minf.sq2];
            // disambiguation
            if(other_sqs.size()>1) {
                bool cols_differ = true;
                bool rows_differ = true;
                for(uint8_t sq3: other_sqs) {
                    if(sq3==minf.sq1)
                        continue;
                    else if(sq3%SZ==c1)
                        cols_differ = false;
                    else if(sq3/SZ==r1)
                        rows_differ = false;
                }
                if(cols_differ)
                    note << cols[c1];
                else if(rows_differ)
                    note << int(SZ-r1);
                else
                    note << cols[c1] << int(SZ-r1);
            }
            // capture
            if(psq2!=EMP)
                note << "x";
            // square: e.g. f3
            note << cols[c2] << int(SZ-r2);
        }
        // markers: +(check) or #(checkmate)
        // e.g. white moves, did they check/checkmate black?
        copy1.execute_move(minf); // white -> black
        uint8_t ksq = *copy1.psquares[(active==WT)?BK:WK].begin();
        // check: could white capture black king if they moved again?
        copy1.active=active; // black -> white
        if(copy1.is_checking(ksq)) { 
            // can black move to block check?
            copy1.active = NEXT(active); // white -> black

            vector<minfo> mlist2 = {};

            copy2.execute_move(minf);
            copy1.all_legal_moves(mlist2,&copy2);
            copy2.undo_move(*this,minf);

            // checkmate occurs if 
            if(mlist.size()==0)
                note << "#";
            else
                note << "+";
        }
        copy1.undo_move(*this,minf);

        not2move[note.str()] = minf;
    }
}