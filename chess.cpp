#include <iostream>
#define SZ 8
#define EMP 0

#define WP 1
#define WN 2
#define WB 3
#define WR 4
#define WQ 5
#define WK 6

#define BP 7
#define BN 8
#define BB 9
#define BR 10
#define BQ 11
#define BK 12

using namespace std;
char pchars[13] = {[EMP]=' ',
                  [WP]='P',
                  [WN]='N',
                  [WB]='B',
                  [WR]='R',
                  [WQ]='Q',
                  [WK]='K',
                  [BP]='p',
                  [BN]='n',
                  [BR]='r',
                  [BB]='b',
                  [BQ]='q',
                  [BK]='k'}; // piece characters

void print(uint8_t (&board)[SZ][SZ]) {
    for(uint8_t j=0;j<2*SZ+1;j++) {
        cout << '-';
    }
    cout << endl;
    for(uint8_t i=0;i<SZ;i++) {
        cout << '|';
        for(uint8_t j=0;j<SZ;j++) {
            cout << pchars[board[i][j]] << '|';
        }
        cout << endl;
        for(uint8_t j=0;j<2*SZ+1;j++) {
            cout << '-';
        }
        cout << endl;
    }
}
int main() {
    uint8_t board[SZ][SZ] = {
                            {BR,BN,BB,BQ,BK,BB,BN,BR},
                            {BP,BP,BP,BP,BP,BP,BP,BP},
                            {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                            {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                            {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                            {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                            {WP,WP,WP,WP,WP,WP,WP,WP},
                            {WR,WN,WB,WQ,WK,WB,WN,WR}};
    print(board);
    return 1;
};