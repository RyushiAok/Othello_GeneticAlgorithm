/*

・コンピューターの次の手の探索にはαβ法を用い、遺伝的アルゴリズムで重みづけした評価関数を利用した。
・評価関数では、四隅～辺の確定石の数と、その盤面における石配置の自由度で評価値を計算している。

*/

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<time.h>
#include<string.h>
#include<math.h>

#define VOID 0
#define BLACK 1
#define WHITE -1
#define NEXT 2
#define LEN 8

//探索の深さ
#define DEPTH_FIRST 6  //8まで
#define DEPTH_MIDDLE 6 //7まで
#define DEPTH_LAST 12  //13まで

#define CAPTURED_POINT 100

#define GA_N 40                   //GA時の個体数
#define GA_RUNNING  false          //遺伝的アルゴリズム実行→false 人vsCPU対戦→true 
#define GA_DISPLAY_BOARD true    //GA時に盤面を表示するならtrue

#define GAME_CLEAR_CONSOLE false  //対戦時 棋譜表示切り替え

typedef struct {
	int turn;
	bool (*put_Piece)(int turn, int board[LEN][LEN]);
} Player;

typedef struct
{
	int Dir[LEN][LEN]; // 下位bitから、 ↑ ↗ → ↘ ↓ ↙ ← ↖ の順に、候補点からみて同色の石があれば1,無ければ0をセット
	void (*update)(int turn, int board[LEN][LEN]);
	void (*clear)(int board[LEN][LEN]);
} Next_Info;


void Game();
bool HumanTurn(int turn, int board[LEN][LEN]); //プレイヤー入力
bool ComputerTurn(int turn, int board[LEN][LEN]); //コンピューター入力


//反転
void TurnOver(int turn, int x, int y, int board[LEN][LEN]); //反転処理
void UpdateNext(int turn, int board[LEN][LEN]); //反転処理に利用
void ClearNext(int board[LEN][LEN]); // 反転処理に利用
Next_Info NextInfo; //反転処理の補助

void Pass(int turn, int board[LEN][LEN]); //パス

//探索
int SearchNextMove(int* x, int* y, int turn, int board[LEN][LEN], int (*eval_board)(int turn, int board[LEN][LEN])); //探索の深さ・評価関数を設定
int Search(int depth, int turn, int board[LEN][LEN], int* x, int* y, int (*eval_board)(int turn, int board[LEN][LEN]));
int Min_Max(int depth, int turn, int board[LEN][LEN], bool myTurn, int* x, int* y, int (*eval_board)(int turn, int board[LEN][LEN]));
int AlphaBata(int depth, int turn, int board[LEN][LEN], int alpha, int bata, bool OppornentTurn, int* x, int* y, int (*eval_board)(int turn, int board[LEN][LEN]));
int EvalBoard(int turn, int board[LEN][LEN]);    //序中盤の評価関数
int GA_EvalBoard(int turn, int board[LEN][LEN]); //GA時の評価関数（グローバル変数を参照する点を除き上と同じ）
int CalcScore(int turn, int board[LEN][LEN]);//終盤の評価関数。turnの石差

int SlatePoints(int nx[LEN * LEN], int ny[LEN * LEN], int turn, int board[LEN][LEN]); // turnの候補点(次置ける場所)を探す
void BoardCaptured(int board[LEN][LEN], int* black, int* white); //四隅から連続する確定石の個数
int EdgeCaptured(int edge[LEN], int turn); //確定石の数え上げに利用
int CountCaptured(int edge[LEN], int pos, int turn, int prev, int prevKak, bool leftKaktei);//確定石の数え上げに利用
int CountEmpty(int edge[LEN]);//確定石の数え上げに利用

void BoardInitialize(Player player[], int board[LEN][LEN]); // 盤面の初期化
void CopyBoard(int from[LEN][LEN], int to[LEN][LEN]); // 盤面のコピー
int CountPieces(int board[LEN][LEN]); // 盤面上の石の個数
void DispBoard(int board[LEN][LEN]); //コンソール表示
void DispResult(int board[LEN][LEN]);//結果表示

//遺伝的アルゴリズム
void GA(); // depth 4,4,10
void GA_Initialize(int pb[LEN * LEN], int pw[LEN * LEN]); //GA時の評価関数の設定
void GA_Selection(int nxt[]);//選択淘汰
void GA_CrossingAndMutation(int nxt_pri[][LEN * LEN], int nxt_gene[], int pri[][LEN * LEN]);//交叉と突然変異
int GA_RandomGene();//突然変異に利用
void GA_Evaluate(int pri[][LEN * LEN], int rank[]); //総当たり戦による順位付け
int GA_Compare(Player com1, Player com2); //別個体の比較 
void GA_DispOP(int pri[LEN * LEN]); //個体の表示 
int GA_OpennedPoint_B[LEN * LEN]; int GA_OpennedPoint_W[LEN * LEN];



int main(void) {
	srand(time(NULL));
	if (GA_RUNNING) {
		GA();
	}
	else {
		Game();
	}
	return 0;
}

void Game() {
	// 初期化
	int board[LEN][LEN];
	Player HUM = { BLACK, HumanTurn };//{ BLACK, ComputerTurn };
	Player COM = { WHITE, ComputerTurn };
	Player player[2] = { HUM,COM }; //{ 先手,後手 }
	BoardInitialize(player, board);

	int passCnt = 0;
	printf("\nGAME START\n");
	for (int i = 0; !(passCnt == 2); i = (i + 1) % 2) {
		DispBoard(board);
		if (player[i].put_Piece(player[i].turn, board)) {
			passCnt += 1;
		}
		else {
			passCnt = 0;
		}
	}
	DispResult(board);
}

void BoardInitialize(Player player[], int board[LEN][LEN]) {
	for (int i = 0; i < LEN; i++) {
		for (int j = 0; j < LEN; j++) {
			board[i][j] = 0;
			board[i][j] = 0;
			NextInfo.Dir[i][j] = 0;
		}
	}
	board[3][3] = BLACK; board[3][4] = WHITE;
	board[4][3] = WHITE; board[4][4] = BLACK;
	NextInfo.update = UpdateNext;
	NextInfo.clear = ClearNext;

	NextInfo.update(player[0].turn, board);
}

//次の手を選択（Player.put_Pieceにより呼び出される。パスした場合、Trueを返す）
bool HumanTurn(int turn, int board[LEN][LEN]) {
	printf("\nプレイヤーの番です.\n");
	if (SlatePoints(NULL, NULL, turn, board) == 0) {
		Pass(turn, board);
		return true;
	}
	int x, y;
	do {
		printf("行列(ex.34) : ");
		scanf("%1d%1d", &y, &x);
	} while (!(0 <= x && x < LEN) ||
		!(0 <= y && y < LEN) ||
		board[y][x] != NEXT);

	TurnOver(turn, x, y, board);
	if (GAME_CLEAR_CONSOLE) system("cls"); //画面消去
	return false;
}

bool ComputerTurn(int turn, int board[LEN][LEN]) {
	if (!GA_RUNNING) printf("\nCPUの番です\n");
	if (SlatePoints(NULL, NULL, turn, board) == 0) {
		Pass(turn, board);
		return true;
	}
	int x, y, eval;
	eval = SearchNextMove(&x, &y, turn, board, (GA_RUNNING) ? GA_EvalBoard : EvalBoard);
	if (!GA_RUNNING) printf("行列(ex.34) : %d%d\n", y, x);
	TurnOver(turn, x, y, board);
	return false;
}

//パス
void Pass(int turn, int board[LEN][LEN]) {
	if (!GA_RUNNING) printf("パスします。\n");
	NextInfo.update(-turn, board);
}


//探索
int SearchNextMove(int* x, int* y, int turn, int board[LEN][LEN], int (*eval_board)(int turn, int board[LEN][LEN])) {
	int depth = 0;
	if (CountPieces(board) <= 15)
		return  Search(DEPTH_FIRST, turn, board, x, y, eval_board);
	else if (CountPieces(board) <= LEN * LEN - DEPTH_LAST)
		return  Search(DEPTH_MIDDLE, turn, board, x, y, eval_board);
	else
		return  Search(DEPTH_LAST, turn, board, x, y, CalcScore);


	return  -1;

}

int Search(int depth, int turn, int board[LEN][LEN], int* x, int* y, int(*eval_board)(int turn, int board[LEN][LEN])) {
	return AlphaBata(depth, turn, board, -(1 << 20), 1 << 20, true, x, y, eval_board);
	// Min_Max(depth, turn, board, true, x, y, eval_board);
}

int Min_Max(int depth, int turn, int board[LEN][LEN], bool myTurn, int* x, int* y, int(*eval_board)(int turn, int board[LEN][LEN])) {
	int nx[LEN * LEN], ny[LEN * LEN], nxtPoints = SlatePoints(nx, ny, turn, board), nxt_board[LEN][LEN];
	if (depth == 0 || nxtPoints == 0) return eval_board((!myTurn) ? -turn : turn, board); //先手の評価値を返す

	int v = 0, best = (myTurn) ? -(1 << 20) : 1 << 20;
	int j = 0;
	for (int i = 0; i < nxtPoints; i++) {
		CopyBoard(board, nxt_board);
		TurnOver(turn, nx[i], ny[i], nxt_board);
		if (myTurn) {
			v = Min_Max(depth - 1, -turn, nxt_board, !myTurn, x, y, eval_board);
			if (best <= v) {
				best = v;
				j = i;
			}
		}
		else {
			best = min(best, Min_Max(depth - 1, -turn, nxt_board, !myTurn, x, y, eval_board));
		}
	}
	*x = nx[j];
	*y = ny[j];
	return best;
}

int AlphaBata(int depth, int turn, int board[LEN][LEN], int alpha, int bata, bool myTurn, int* x, int* y, int(*eval_board)(int turn, int board[LEN][LEN])) {
	int nx[LEN * LEN], ny[LEN * LEN], nxtPoints = SlatePoints(nx, ny, turn, board), nxt_board[LEN][LEN];

	if (depth == 0 || nxtPoints == 0) return  eval_board((!myTurn) ? -turn : turn, board); //先手の評価値を返す
	int select = 0;
	for (int i = 0; i < nxtPoints && alpha < bata; i++) {
		CopyBoard(board, nxt_board);
		TurnOver(turn, nx[i], ny[i], nxt_board);
		if (myTurn) {
			int a = AlphaBata(depth - 1, -turn, nxt_board, alpha, bata, !myTurn, x, y, eval_board);
			if (alpha < a) {
				alpha = a;	select = i;
			}
			if (alpha >= bata) break;

		}
		else {

			bata = min(bata, AlphaBata(depth - 1, -turn, nxt_board, alpha, bata, !myTurn, x, y, eval_board));
			if (alpha >= bata) break;
		}
	}
	*x = nx[select];
	*y = ny[select];
	return (myTurn) ? alpha : bata;
}


//局面評価関数 評価値差分を計算したい（2020-02-07）
int EvalBoard(int turn, int board[LEN][LEN]) {

	static int oppenedPoint[LEN * LEN] =
		//{ 99,  6, 73, 24, 79,  8, 58, 85, 67, 14, 82, 74, 34, 40, 72, 62, 61,  0, 94, 24, 28, 77, 82, 77, 79, 86,105,  0, 17, 89, 49, 22, 72,  1, 70, 37, 50,  6, 21, 81, 74,  5, 35, 34,  4, 27, 96, 61, 78, 90, 30, 26, 15, 17, 54, 28, 23, 34, 11, 94, 35, 65,109, 56, };
		//{108,103,  2, 19, 26, 12, 27, 71, 77,  0, 15, 76, 16, 49, 11, 57, 84, 95, 85,  0, 63, 92, 81, 61, 79, 18, 39, 56, 54, 43, 75,  4,105, 75, 51, 83, 37,  0, 92, 21, 20,  0, 14, 61, 55, 10, 89, 25, 13, 30,105, 10, 88, 31, 96,101,  7,103, 39, 68, 62, 88,  0, 36,};
		 
		{ 28, 20, 62,104, 26,102, 27,102, 77, 93, 15, 76, 70, 11,103,101, 16, 92, 42,  0,  6, 98,  5, 76,101, 66, 33, 95, 46, 57, 71, 65, 65, 57, 51, 43, 99, 60,  0,103,104,  8, 79, 19, 76, 39, 23, 24, 80, 18,  1, 81, 89, 15,105, 70, 40, 43,108, 95,  0,105, 11, 36, };
	
	int result = 0;
	int mine = 0, oppo = 0;//確定石の個数	

	if (turn == BLACK)
		BoardCaptured(board, &mine, &oppo);
	else if (turn == WHITE)
		BoardCaptured(board, &oppo, &mine);
	result += (mine - oppo) * CAPTURED_POINT; //確定石
	result += oppenedPoint[CountPieces(board)] * SlatePoints(NULL, NULL, turn, board); //石配置の自由度

	return result;
}

int GA_EvalBoard(int turn, int board[LEN][LEN]) {

	int result = 0, pri_sum = 0;
	int mine = 0, oppo = 0;//確定石の個数	

	if (turn == BLACK)
		BoardCaptured(board, &mine, &oppo);
	else if (turn == WHITE)
		BoardCaptured(board, &oppo, &mine);
	result += (mine - oppo) * CAPTURED_POINT;
	result += ((turn == BLACK) ? GA_OpennedPoint_B[CountPieces(board)] : GA_OpennedPoint_W[CountPieces(board)]) * SlatePoints(NULL, NULL, turn, board);

	return result;
}

int CalcScore(int turn, int board[LEN][LEN]) {
	int b = 0, w = 0;
	for (int i = 0; i < LEN; i++) {
		for (int j = 0; j < LEN; j++) {
			if (board[i][j] == BLACK) b++;
			if (board[i][j] == WHITE) w++;
		}
	}
	return (turn == BLACK) ? b - w : w - b;
}

int CountPieces(int board[LEN][LEN]) {
	int cnt = 0;
	for (int i = 0; i < LEN; i++) {
		for (int j = 0; j < LEN; j++) {
			cnt += (board[i][j] == BLACK || board[i][j] == WHITE) ? 1 : 0;
		}
	}
	return cnt;
}


//確定石(四隅から連続する、以降反転しない石)の数え上げ
void BoardCaptured(int board[LEN][LEN], int* black, int* white) {
	int edges[8][LEN];
	for (int i = 0; i < LEN; i++) {
		edges[0][i] = board[0][i];
		edges[1][i] = board[0][LEN - i - 1];
		edges[2][i] = board[LEN - 1][i];
		edges[3][i] = board[LEN - 1][LEN - i - 1];
		edges[4][i] = board[i][0];
		edges[5][i] = board[LEN - i - 1][0];
		edges[6][i] = board[i][LEN - 1];
		edges[7][i] = board[LEN - i - 1][LEN - 1];
	}
	for (int i = 0; i < 8; i += 2) {
		int a = EdgeCaptured(edges[i], BLACK);
		int b = EdgeCaptured(edges[i + 1], BLACK);
		if (a == b) *black += a;
		else *black += a + b;

		a = EdgeCaptured(edges[i], WHITE);
		b = EdgeCaptured(edges[i + 1], WHITE);
		if (a == b) *white += a;
		else *white += a + b;
	}
	if (board[0][0] == BLACK) (*black)--;
	if (board[0][LEN - 1] == BLACK) (*black)--;
	if (board[LEN - 1][0] == BLACK) (*black)--;
	if (board[LEN - 1][LEN - 1] == BLACK) (*black)--;
	if (board[0][0] == WHITE) (*white)--;
	if (board[0][LEN - 1] == WHITE)  (*white)--;
	if (board[LEN - 1][0] == WHITE)  (*white)--;
	if (board[LEN - 1][LEN - 1] == WHITE)  (*white)--;
}

int EdgeCaptured(int edge[LEN], int turn) { //（確定石）→（不明）→（確定石）と並んでいれば、不明は確定石と定まる
	if (!(edge[0] == BLACK || edge[0] == WHITE)) return 0; // 開始点が不明
	if (edge[0] == turn)
		return 1 + max(0, CountCaptured(edge, 1, turn, edge[0], edge[0], true));
	else
		return max(0, CountCaptured(edge, 1, turn, edge[0], edge[0], true));
}


int CountCaptured(int edge[LEN], int pos, int turn, int prev, int lastCaptured, bool prevPieceIsCaptured) {
	// 確定条件
	//（確定石）→（同色石） このとき同色石は確定する
	//（確定石）→（不明）→（確定石）と並んでいれば、不明は確定石と定まる
	//（確定石）→（不明X）→ (不明色A)→（不明色B: B!=A）→(不明:空白1つ)  空白にA,Bどちらの色が置かれても、不明Xは確定石同士で挟まれる 
	// 
	// 返り値について
	// 以降(pos+1～)不明なら、負の値を返される。確定石ならば、edge[pos～LEN-1]上のturn色の確定石の数を返す.

	if (pos == LEN - 1) {
		if (!(edge[pos] == BLACK || edge[pos] == WHITE))
			return -1;
		else
			return (edge[pos] == turn) ? 1 : 0;
	}

	//空白
	if (!(edge[pos] == BLACK || edge[pos] == WHITE)) return -1;

	//（直前）→(現在)→（次）
	if (prevPieceIsCaptured) {
		int cnt = CountCaptured(edge, pos + 1, turn, edge[pos], (lastCaptured == edge[pos]) ? lastCaptured : edge[pos], lastCaptured == edge[pos]);
		if (cnt >= 0) //（確定）→(確定)→（確定）
			return (turn == edge[pos]) ? cnt + 1 : cnt;
		else //（確定）→(確定)→（確定:確定石の同色連続である場合,edge[pos]は確定  or 不確定）
			return (prev == edge[pos]) ? (turn == edge[pos]) ? 1 : 0
			: -1;
	}
	else {
		int cnt = CountCaptured(edge, pos + 1, turn, edge[pos], lastCaptured, false);
		if (cnt >= 0)//（不明）→（不明）→（確定）
			return (turn == edge[pos]) ? cnt + 1 : cnt;
		else//（不明）→（確定:残り空白１&&現在直前の石と別色  or  不確定）  反転可能性があるのはedge[pos]だけ => 直前の石にとってedge[pos]は確定石
			return (CountEmpty(edge) == 1 && prev != edge[pos]) ? 0 : -1;
	}
}

int CountEmpty(int edge[LEN]) {
	int r = 0;
	for (int i = 0; i < LEN; i++) {
		if (!(edge[i] == BLACK || edge[i] == WHITE)) r++;
	}
	return r;
}


//候補点
int SlatePoints(int nx[LEN * LEN], int ny[LEN * LEN], int turn, int board[LEN][LEN]) {
	NextInfo.update(turn, board);
	int cnt = 0;
	for (int y = 0; y < LEN; y++) {
		for (int x = 0; x < LEN; x++) {
			if (board[y][x] == NEXT) {
				cnt++;
				if (nx != NULL) {
					nx[cnt - 1] = x;
					ny[cnt - 1] = y;
				}
				else {
					continue;
				}
			}
		}
	}
	return cnt;
}

void CopyBoard(int from[LEN][LEN], int to[LEN][LEN]) {
	for (int i = 0; i < LEN; i++) {
		for (int j = 0; j < LEN; j++) {
			to[i][j] = from[i][j];
		}
	}
}


//反転
void TurnOver(int turn, int x, int y, int board[LEN][LEN]) {
	NextInfo.update(turn, board);
	int dx[] = { 0,1,1,1,0,-1,-1,-1, };
	int dy[] = { -1,-1,0,1,1,1,0,-1 };
	int dir = NextInfo.Dir[y][x];
	int cx, cy;
	int opponent = WHITE; if (turn == WHITE) opponent = BLACK;
	board[y][x] = turn;
	for (int i = 0; i < 8; i++) {
		if ((dir & (1 << i)) == 0) continue;
		cx = x + dx[i];
		cy = y + dy[i];
		while ((0 <= cx && cx < LEN && 0 <= cy && cy < LEN) && (board[cy][cx] == opponent)) {
			board[cy][cx] = turn;
			cx += dx[i];
			cy += dy[i];
		}
	}
	NextInfo.update(-turn, board);
}

void UpdateNext(int turn, int board[LEN][LEN]) {
	NextInfo.clear(board); //反転情報クリア
	int dx[] = { 0,1,1,1,0,-1,-1,-1, };
	int dy[] = { -1,-1,0,1,1,1,0,-1 };
	int cx, cy;
	int opponent = WHITE; if (turn == WHITE) opponent = BLACK;

	for (int i = 0; i < LEN; i++) {
		for (int j = 0; j < LEN; j++) { 
			if (board[i][j] == turn) {
				for (int dir = 0; dir < 8; dir++) {
					// 隣接する石が別の色
					cx = j + dx[dir];
					cy = i + dy[dir];
					if (!(0 <= cx && cx < LEN) || !(0 <= cy && cy < LEN)) continue; // 盤外					
					if (board[cy][cx] != opponent) continue;

					do {
						cx += dx[dir];
						cy += dy[dir];
						if (!(0 <= cx && cx < LEN) || !(0 <= cy && cy < LEN)) continue; // 盤外
						if (board[cy][cx] == turn) continue; //自分の石
					} while (board[cy][cx] == opponent);

					if (!(0 <= cx && cx < LEN) || !(0 <= cy && cy < LEN)) continue;
					if (board[cy][cx] == VOID || board[cy][cx] == NEXT) {
						board[cy][cx] = NEXT;
						NextInfo.Dir[cy][cx] += 1 << ((dir + 4) % 8); //反転情報更新
					}

				}
			}
		}
	}
}

void ClearNext(int board[LEN][LEN]) {
	for (int i = 0; i < LEN; i++) {
		for (int j = 0; j < LEN; j++) {
			NextInfo.Dir[i][j] = 0;
			if (board[i][j] == NEXT) board[i][j] = VOID;
		}
	}
}


//表示
void DispBoard(int board[LEN][LEN]) {
	printf("\n---------------------\n");
	printf("    0 1 2 3 4 5 6 7 \n");
	for (int i = 0; i < LEN; i++) {
		printf("%2d ", i);
		for (int j = 0; j < LEN; j++)
			switch (board[i][j])
			{
			case BLACK:
				printf("〇");
				break;
			case WHITE:

				printf("●");
				break;
			case NEXT:
				printf("※");
				break;
			case VOID:
				printf("・");
				break;
			}
		printf("\n");
	}
	printf("\n");
	return;
}

void DispResult(int board[LEN][LEN]) {
	int b = 0, w = 0;
	for (int i = 0; i < LEN; i++) {
		for (int j = 0; j < LEN; j++) {
			if (board[i][j] == BLACK) b++;
			if (board[i][j] == WHITE) w++;
		}
	}
	printf("黒石: %d, 白石: %d\n", b, w);
}


//遺伝的アルゴリズムによる最適化
void GA() {
	printf("\n------------- GA START --------------\n");
	//初期個体群の生成
	int op[GA_N][LEN * LEN];
	for (int i = 0; i < GA_N; i++) {
		for (int j = 0; j < LEN * LEN; j++) {
			op[i][j] = GA_RandomGene();
		}
	}

	int seednum = 40;

	int seeds[][LEN * LEN] =
	{	{  0, 94, 85,  9,  0, 12, 81,  0, 36, 27,  0,  0,  0,  0, 37, 38, 84, 67, 31,  0, 78,  0,  0, 10,  0, 21,  0, 43,  0, 72,  0,  9,104, 27,102, 12,  0,107, 15, 21,  0,  0, 14, 41,  0, 16, 85, 54, 91, 78,  0, 52, 88,  0,  0, 71,  0, 58, 43,  0, 51,105,106, 52,},
		{  0, 74,  0,  9, 93, 12, 30,  0, 36, 27, 63,  0, 98,  0, 31, 23, 84, 50, 31,  0, 78,  3, 86, 10,102, 12, 66, 43,  0, 72,108,  9,104, 72, 23, 94, 76,107,  5, 21,  6, 27, 14, 41,  0, 16, 85,  7, 54, 78,  0, 52, 53, 97, 62, 71,  0,  0,108, 41,  0, 65, 31,107,},
		{ 48, 94, 85, 16,  0, 54, 72, 54, 77,  4, 12, 37,  0, 11, 37, 86, 16, 67, 31, 69, 49,  0, 36, 76,  5, 52,  0, 73,  3, 72,  0,  0, 52, 27,102, 12,  0, 10, 15, 21,  0,  0, 57, 16, 99, 49, 30, 54, 91,  6,  0, 89, 88,  0,  0, 54,  1, 58, 43,  0, 51,105,106, 52,},
		{ 48, 47,  0, 16, 93,  0,109, 71, 77, 82, 89, 76, 98, 11,103,107, 16,104, 89, 69, 49,  3, 19, 76, 95, 27, 45, 73, 96, 72,108,  0, 77, 57, 23, 24, 76, 23,  5, 21,  6, 27,108, 89, 76, 49, 30,  7, 54,  6, 15, 19, 55, 97, 62, 54,  1,  0,104, 41, 39, 65, 31,  0,},
		{ 48, 74, 85,104, 88, 54, 99, 54, 77,  4, 64, 37, 70, 11, 31, 38,  3,107, 31,  0, 34, 42, 11, 10, 35,  4, 66, 43,  3, 57,  8,  3,  7, 72, 51, 94, 76, 10,  5, 92,104,  8, 10, 16, 99, 66, 30,  7, 48, 42,  0, 89, 53, 81, 62, 58, 40, 43,108, 20,  0, 65, 31,107,},
		{ 23,107, 97, 44, 88, 54,109, 54,  5, 82,  0, 77, 81, 11,107,107, 67, 50, 89,  0, 34, 42,  5, 10,101, 66,108, 43,  3, 57,  8, 37,  0, 57, 97, 43, 99, 23,  0, 54, 20,  8, 71, 89, 59, 66, 30,  0, 48, 42, 15, 46, 55, 81, 62,104, 69, 46,104, 20, 39, 65, 31,  0,},
		{ 48, 47, 85,104, 26,  0, 49, 71, 77, 52, 18, 76, 92, 11,103,101,  3, 76, 26,  0,  1,  0, 50, 10, 95, 27, 65, 73, 96, 58,108,  3, 99, 53, 71, 24, 76, 10,  5, 26,104, 83, 27,  0, 40,  0, 89,  7, 78,  6, 15, 19, 19, 54,  8, 58, 40, 43, 39,102, 51, 31, 72,  0,},
		{ 23, 38, 83, 44, 11, 54, 49, 54,  5, 52, 18, 57, 70, 11,103,101, 40, 41, 26,  0,  1, 25, 50, 61,101, 60, 56, 25, 34, 27,108,  2, 53, 53, 96, 17, 37, 28, 46, 49, 63, 57, 58,  0, 99,  0, 89, 24, 47,  6, 67, 46, 68, 54, 86, 29, 69, 28, 39,102, 51, 31, 27, 64,},
		{ 48,107, 97,  0, 26, 83, 36, 78,  5, 82, 18, 39, 23, 76, 95, 97, 67, 30, 27, 96, 42, 61, 50, 10,  5, 66,105, 73,  3, 58, 17, 37,  0, 28,  0, 43, 99, 49,  0, 56, 20, 83, 71, 38, 62, 66, 76,  0, 78,102, 15, 19, 48, 31,  8, 58, 57, 46,108, 57, 59, 65, 72,  0,},
		{ 48, 38, 97, 72, 93, 83, 26, 78,  5,109, 18,  0, 70, 76, 95, 97, 40, 50, 27, 96, 42, 63,104, 61,  5, 55, 56, 43,  3, 57, 28,  3, 53, 28, 81, 17, 89,106, 23, 49, 20, 57, 72, 38, 99, 22, 76, 24, 47,102,101, 74, 53, 56, 86, 70, 89,  0, 39, 57, 44,101, 31, 79,},
		{ 23, 35, 83, 25, 11, 12, 36, 54,  5, 82, 14,  0, 23, 16,103,  0, 40, 49, 89,  0,  1, 36,105, 10, 95, 60, 45, 25, 34, 27,  5,  2, 52, 57,  0, 46, 13, 10, 46,  3, 63,  8, 58, 34, 62, 66, 30, 20, 54,102, 67, 19, 55, 27,105, 37, 57, 28,108,  0, 59, 65, 27, 64,},
		{ 23, 95, 97, 26, 93, 12, 26, 59,  5,109, 14,  0, 70, 16, 92, 24, 60, 50, 51,  0,  6, 83, 93, 61,102, 31, 78, 43,  3, 93, 75,  3, 52, 57, 75, 22, 16, 18, 23, 49, 44, 80, 71, 34, 76,  0, 30,  0, 54,  0,101, 88, 70, 54,105,101, 89, 70,  0, 24,  0,101, 31,  0,},
		{ 23, 35, 97, 61, 93, 28, 59, 54, 77, 57, 14,  0, 54, 96, 16,  0, 33, 32, 89,  0,  1, 63, 74, 10, 95, 93, 45, 25, 82, 57,108,  4, 45, 93, 81, 46, 76, 10,  5,  3, 20,  8, 72,104, 62, 22, 89, 20, 54,102,105, 80, 85, 21, 59, 70, 77,  0, 39,  0, 44, 65,  5, 79,},
		{ 23, 95, 97, 61, 93, 12, 59, 33,  5, 57, 14, 41, 70, 16, 22,  0, 84, 32,  0,  0,  6, 83, 81, 61,102, 60, 78, 31,  0, 93, 75,  4, 45, 80, 36, 99, 26, 18, 15, 49, 20, 34, 75,104,  0,  0,102,  0, 54,  0,105, 89, 20, 21, 59, 64, 77, 40, 81, 24,  0, 65, 41,  0,},
		{ 36, 13, 97, 26, 39, 28, 13, 59, 77, 57, 14,  0, 54, 96, 95, 24, 40, 50,102,  0, 41, 59, 60, 61,  0, 66, 71, 25, 82, 18, 46, 65, 24, 93, 75, 22, 16, 60,  5,  3, 44, 80, 74, 34, 76, 49, 89,  7, 28,  6, 67, 80, 53, 54, 85, 12, 89, 70,108,  0, 44, 40, 98, 79,},
		{ 36, 35, 97, 44, 56, 12, 37, 33,  5, 61, 14, 76, 70, 58, 95, 39, 40, 50, 49, 43, 41, 59, 14, 61,102, 66, 71, 31, 96, 18, 46,  0,  6, 80, 17, 99, 86, 60, 15, 41, 20, 83, 27, 34, 76, 49, 89,  7, 78,  6, 67, 89, 53, 81, 62, 70, 89, 40,  6,  0, 72, 40, 64, 79,},
		{ 48,107, 85, 61, 39, 54, 17, 33,  5, 76, 14, 41, 70,  9,103, 77, 16, 50, 26, 35, 41, 98, 81, 61,  0, 66,105, 73,  0, 75,  8, 65, 24, 80, 36,  6, 79, 60, 75,  3,104, 34, 32, 34,  0, 49,102,  7, 28,  6, 25, 89, 19, 54, 85, 53, 69, 40,104,102, 44, 40, 72, 32,},
		{ 89,107, 83, 53, 26, 54, 23, 33,  5, 82, 18, 76, 70, 11,103, 98, 16, 63, 27, 34, 41, 98, 14, 61, 74, 66,  0, 73, 96, 58,  8,  0,  6, 80, 60,  6, 99, 60, 63, 41,104, 15, 32, 38, 98, 66, 89,  7, 78,  0, 90, 30, 53, 81, 76, 58, 19, 47, 39,102, 72, 40, 64, 32,},
		{ 48, 74, 85, 44, 56, 83,  1,105,  5, 43, 14,  0, 23, 80, 95, 65, 67, 50, 26,  0, 29,  0,  0, 41, 26, 34,105, 28, 37, 75, 17,  3, 27, 97, 27, 41,  0,  0, 27, 49,104, 83, 25, 34, 94, 49, 61, 35, 47,  6, 68, 89, 19,  0, 62, 53, 69, 40,104,  0, 40,  0, 72, 79,},
		{ 41, 84, 78, 62, 26,  0, 21, 25, 49, 40, 18, 99, 23, 80, 87,  2, 67, 49, 27,  0,  0,  0, 14, 76, 41, 32,  0, 10, 54, 57, 49,  3,  8, 32, 59, 74, 21,  0, 15, 21,104, 10,105, 68, 99, 66, 96, 35, 38,  6, 15, 20, 88,  0,  8,105, 26,  0, 39,  0, 40,  0, 64, 28,},
		{ 13, 74, 83, 53, 68,105, 23,105,  5, 82, 18,  0, 54, 60, 48, 98, 62, 63, 22, 34, 29, 98,  0, 41, 55, 34, 21, 28, 37, 58, 17, 24, 27, 97, 10, 41, 99, 10, 24, 49, 90, 17, 25, 38, 99, 90, 61, 42, 47,  0,102, 26, 53, 26, 65, 74, 19, 47, 39, 49,  0, 85, 96, 79,},
		{ 58, 74,  4, 16, 87, 98, 21, 25, 49, 41, 18,  0, 54, 11, 19, 77,108, 49, 22,  0,  0, 98, 14, 10, 35, 32,105, 10, 54,102, 49, 24,104, 20, 59, 74, 50,  0, 59, 17,  6, 53,105, 34, 73, 66, 96, 42, 62, 42, 15, 19, 88, 31, 65, 54, 26,  0, 17,  5, 99, 72, 96,  0,},
		{ 80, 84, 78, 62, 68, 83, 36,  0, 77, 15, 18, 99, 54, 60,  0, 18, 62, 90, 75,103,  0,  3, 60, 76, 41, 27, 21, 10, 75, 57, 46, 65,  8, 32, 59, 12, 45, 10, 24, 21, 90, 83, 59, 68, 74, 90,102,104, 38,  6, 95, 10, 51, 26,  0, 58, 40, 43, 58, 49, 27, 85,  0, 28,},
		{ 80, 74,  4,  0, 87, 83, 36, 34, 46, 40, 18,  0, 54, 59,  0, 87,108, 90, 75,103,  0,  3, 60, 61, 55, 27,105, 25,103,102, 46, 65,104, 20, 81, 12,  0,107,109, 17,  6, 53, 57, 34,  0,  0, 76, 86, 62, 42, 95, 19, 13, 26, 32, 54, 40,  7,108, 26, 31, 72,  0,107,},
		{ 80, 74,  0, 16, 68, 83, 33,  0, 77, 59, 15, 76, 70, 79,  0, 95, 35, 20, 51, 69, 78,  3, 50, 10, 35, 34,105, 10, 75, 72,  0, 96,104, 20, 77, 22,  0,  0,  6,  0,104,  0, 59, 34,  0, 66,102,104, 62, 10, 37, 19, 51, 31, 10, 54,  0, 43,  6,105,108,  8,  0,  0,},
		{ 80,  5, 78,  0, 93, 46, 32, 54, 77,  4, 14, 76, 70, 11,  0, 95, 35, 50, 74, 69, 35, 64, 50, 61, 55, 66,105, 25,103, 75,  0, 96, 68, 20,  0, 22,  0,107,  5,  0,104,  0, 58, 34,  0, 18, 29, 86, 62, 10, 37, 33, 13, 26, 62, 54,  0,  7,108, 76, 51, 62,  0,107,},
		{ 23, 74,  0,  0, 68, 83, 33, 34, 46, 59, 15,  0, 21, 11, 31, 38, 65, 20, 95,103, 42, 25, 71, 61, 55, 34, 45, 25, 96, 72,108,  0,104, 20, 19, 22,  0,107,  5, 21,104, 83, 57, 90, 99, 77,  1, 57, 13,  6,  0,  6, 53, 31, 62, 54, 40,  0,108, 57, 78,  8, 74,107,},
		{ 78, 11, 78, 41,108, 46, 72,102,  5, 86, 15,  0, 70, 78, 95, 38, 84, 49, 70,103, 49, 27, 71, 61, 55, 66,105, 25, 96, 75, 77,  0, 53, 20, 19, 22, 76,107,  4, 21,104, 80, 58, 34, 67,  0, 89, 20,  8,  6,  0, 89, 89, 54, 86, 37, 40,  0,108, 57, 51, 37, 74,  0,},
		{ 47,  5,  2,  0, 93,102, 32, 54, 77, 19, 14, 78, 21, 11, 31, 58, 65, 50, 18, 43, 78,  0, 81, 61,  0, 60, 45, 43, 87, 16,108, 65, 68, 28, 53,  6,  0, 60, 10, 21, 34, 83, 42, 90, 51,  0, 30, 57, 13, 41,  4, 80, 53, 31, 62, 54, 57, 42, 76, 92, 78, 62, 72,107,},
		{ 11, 74,  2, 41, 33,102, 48, 11,  5, 40, 15,  0, 38, 59, 95, 98, 76, 62,  0,107, 78, 36, 19, 41, 55, 60, 14, 43, 87, 16, 77, 65, 53, 97,  0, 45, 99, 10, 59,103, 34,102, 42, 34, 62,  0, 30, 20,  8,  0,  4, 46, 89, 54,  8, 33, 57, 42, 76, 92,  9, 94, 94, 32,},
		{ 48, 11,  2, 38, 27, 83, 72,102,  0, 52, 15, 78, 68, 94, 95, 58, 67, 49, 58, 43, 78, 36, 20, 61,  0, 34,105, 43, 83, 93, 77, 65, 27, 28, 53,  6, 76, 60, 15, 21,104, 80, 42, 34, 76, 35, 30, 20,  8, 41,  4, 80, 41, 54, 86, 37, 19, 42, 76, 92, 46, 37, 72,  0,},
		{ 23, 74, 97, 38, 26, 12, 21, 27,  0, 57, 14,  0, 70, 88, 19, 98, 84, 32, 58,107, 78, 98, 50, 72,101, 60, 94, 43, 83, 57, 77, 65, 45, 97,  0, 45, 16, 10, 15, 34, 20, 83, 42,  0, 76,  0, 89, 92,  8, 25,  4, 46, 41, 26, 59,104, 26, 42, 76, 92,  0, 94,  5, 32,},
		{ 11, 95,  2, 16,108, 83, 48, 11, 49, 40, 15,105, 22, 52, 95,101, 84, 62, 22, 96,  0, 36, 81,  5, 55, 34, 14, 25,  9, 93,  0,  2, 27, 53, 59, 74, 99, 28,  0,103,104,102, 71, 34, 76, 35, 30, 48, 96, 75, 15, 46, 88, 54,  8, 33, 19,  0, 39,102, 42, 85, 94, 49,},
		{ 23, 95, 78, 48, 26, 82, 21,105, 49, 57,  0, 33, 70,  0, 19,101, 84, 67, 31, 96,  0, 98, 50,  0, 55, 48, 27, 43,  9, 57,  0,  2,  0, 53, 93, 12, 16,  0,  0, 36, 68, 83, 71, 54,106, 66, 89, 35, 91,102, 50, 46, 88, 26, 62,104, 26,106, 39,  0, 40, 85,  5,107,},
		{ 80,  0, 97, 22, 26,100, 27, 27,  5, 42, 14,105, 22, 92, 36, 99, 84, 32, 22,  0, 29,  0,  0, 24,101, 64, 94, 25, 20, 72,  0, 65, 45, 27, 59, 74, 76, 28,  0, 15, 20, 60, 57,  0, 76,  0, 61,  0, 96,  6, 35, 52, 51,108, 59, 47, 38,  0, 39,102,  0,105, 64, 49,},
		{ 80, 75,  0, 73,101,  0, 21,105,  5, 42,  0, 37, 12,  0, 36, 99, 84, 67, 31,  0,  0, 36,  0,  0, 30, 62, 27, 43,109, 72, 25, 34, 42, 27, 93, 12, 76,  0,  0, 92,  0, 68, 57,  0,106, 66, 61, 35, 60,102, 67, 52, 51,108, 62, 11, 72,106, 39, 21, 40,  0,  5,107,},
		{ 80,  0, 78,  9, 26,102, 27,102, 24, 52,  0, 33, 70, 11, 30, 32,  0, 31,  0,  0, 29,  0, 93, 61, 92, 57, 27, 73, 55, 95, 68, 65,  0,  0, 93,  0, 97, 10, 24,100,104, 60, 57, 54,  0, 17, 69, 71,104, 78, 67, 13, 51,108, 62, 47,102,106,109,  0,104,105, 64,107,},
		{  0, 75,  0,  0,  0, 96, 21,  0,  0,  0, 59,  0, 12,  0, 30, 32,  0, 31,  0,  0,  0,  0, 35,  0, 55,  0,  0,  0, 54,  0,  0,  4,  0,  0, 93,104,  0, 10,  0, 21,  0, 68,  0,  0, 95, 17,  0, 71, 13,  0, 67,  0,  0,  0, 62, 11, 86,  0,109,  0,104,  0,  5,107,},
		{ 80,  0,  0, 73,101, 40,  0,102, 24, 52, 35, 37,  0, 11,  0,  0,  0,  0,  0,  0,  0, 36, 17, 61,  0, 57, 27, 73,  0, 95,  0, 87, 42,  0,  0, 57, 97,  0, 24,  0,  0,  0, 57,  0, 84,  0, 69,  0,  0, 78,  0, 13, 51,108,  0,  0,  0,106,  0, 21,  0,  0,  0,  0,},
		{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,} };

	for (int i = 0; i < seednum && i < GA_N; i++) {
		for (int j = 0; j < LEN * LEN; j++) {
			op[i][j] = seeds[i][j];
		}
	}

	int genemax = 10000;
	for (int i = 1; i <= genemax; i++) {
		//成績評価
		int nxtIdx[GA_N] = { 0 };
		GA_Evaluate(op, nxtIdx);
		//選択淘汰
		GA_Selection(nxtIdx);
		//交叉・突然変異
		int nxt_pri[GA_N][LEN * LEN];// 次世代
		GA_CrossingAndMutation(nxt_pri, nxtIdx, op);
		if (i % 5 == 0) {
			printf("%4d(/%4d)世代目\n", i, genemax);
			for (int j = 0; j < GA_N; j++) {
				GA_DispOP(op[j]);
			}
		}
		else {
			printf("%4d(/%4d)世代目\n", i, genemax);
			GA_DispOP(op[0]);
			printf("\n");

		}
	}
	printf("\n------------- GA RESULT --------------\n");
	GA_DispOP(op[0]);
	return;
}

void GA_Selection(int nxt[]) {
	for (int i = GA_N - 1; i >= 0; i--) {
		nxt[i] = nxt[i / 2];

	}
}

void GA_Evaluate(int op[][LEN * LEN], int rank[]) {

	//成績評価	
	int eval[GA_N] = { 0 }, cnt = GA_N - 1;
	int tornament[GA_N] = { 0 };
	for (int i = 0; i < GA_N; i++) {
		tornament[i] = i;
	}
	//ランダム入れ替え
	for (int i = 0; i < GA_N * 4; i += 2) {
		int a = rand() % GA_N;
		int b = rand() % GA_N;
		int tmp = tornament[a]; tornament[a] = tornament[b]; tornament[b] = tmp;
	}
	//トーナメント
	for (int i = 1; i < GA_N; i *= 2) {
		for (int j = 0; j + i < GA_N; j += 2 * i) {
			int a = tornament[j];
			int b = tornament[j + i];
			Player com_black = { BLACK, ComputerTurn };
			Player com_white = { WHITE, ComputerTurn };

			int blackScore = 0;
			GA_Initialize(op[a], op[b]);
			blackScore = GA_Compare(com_black, com_white);
			eval[a] += (blackScore > 0) ? 1 : 0;
			eval[b] += (blackScore < 0) ? 1 : 0;

			if (blackScore < 0) {//b win
				int tmp = tornament[j]; tornament[j] = tornament[j + i]; tornament[j + i] = tmp;
			}
			printf("\b\b\b\b\b[%3d]", --cnt);

		}
	}
	printf("\b\b\b\b\b");

	printf("\n\n");

	// 成績順によるソート、nxt_geneの設定
	int dif[GA_N] = { 0 };
	for (int i = 0; i < GA_N; i++) {
		int max = eval[i];
		int pos = i;
		for (int j = i; j < GA_N; j++) {
			if (max < eval[j]) {
				pos = j;
				max = eval[j];
			}
		}
		int tmp = dif[i]; dif[i] = dif[pos]; dif[pos] = tmp;
		dif[i] += pos - i;
		dif[pos] -= pos - i;
		eval[pos] = eval[i]; eval[i] = max;
	}
	for (int i = 0; i < GA_N; i++)  rank[i] = i + dif[i];
}

int GA_Compare(Player com1, Player com2) {
	int board[LEN][LEN];
	Player player[2] = { com1,com2 };
	BoardInitialize(player, board);

	// ゲーム
	int cnt = 0;
	for (int i = 0; !(cnt == 2); i = (i + 1) % 2) {
		if (GA_DISPLAY_BOARD) DispBoard(board);
		if (player[i].put_Piece(player[i].turn, board)) {
			cnt += 1;
		}
		else {
			cnt = 0;
		}
		if (GA_DISPLAY_BOARD) DispResult(board);
	}
	return CalcScore(com1.turn, board);
}

void GA_CrossingAndMutation(int children[][LEN * LEN], int rank[], int parents[][LEN * LEN]) {

	int mut = LEN * LEN / 10;
	for (int i = 0; i < 2; i++) { //エリートの生存
		for (int j = 0; j < LEN * LEN; j++) {
			children[i][j] = parents[rank[0]][j];
		}
	}
	for (int i = 1; i + 1 < GA_N; i += 2) {
		int p1 = rank[i], p2 = rank[i + 1];
		//交叉
		for (int j = 0; j < LEN * LEN; j++) {

			if ((rand() % 100) < 50) {
				children[i][j] = parents[p1][j];
				children[i + 1][j] = parents[p2][j];
			}
			else {
				children[i][j] = parents[p2][j];
				children[i + 1][j] = parents[p1][j];
			}
		}
		//突然変異
		for (int j = 0; j < mut; j++) {
			int a = rand() % (LEN * LEN);
			children[i][a] = GA_RandomGene();
			children[i + 1][a] = GA_RandomGene();
		}
	}
	//コピー
	for (int i = 0; i < GA_N - 1; i++) {
		for (int j = 0; j < LEN * LEN; j++) {
			parents[i][j] = children[i][j];
		}
	}
	//自由度に重みづけをしない、プレーンな状態の個体
	for (int i = 0; i < LEN * LEN; i++) {
		parents[GA_N - 1][i] = 0;
	}
}

int GA_RandomGene() {
	return  rand() % 110;
}

void GA_Initialize(int pb[LEN * LEN], int pw[LEN * LEN]) {
	for (int i = 0; i < LEN * LEN; i++) {
		GA_OpennedPoint_B[i] = pb[i];
		GA_OpennedPoint_W[i] = pw[i];
	}
}

void GA_DispOP(int op[LEN * LEN]) {
	printf("{");
	for (int i = 0; i < LEN * LEN; i++) {
		printf("\x1b[49m");
		if (i < 4 + DEPTH_FIRST) {
			printf("%3d,", op[i]);
		}
		else if (op[i] < 50) {
			printf("\x1b[44m%3d,", op[i]);
		}
		else if (op[i] < 100) {
			printf("\x1b[43m%3d,", op[i]);
		}
		else {
			printf("\x1b[41m%3d,", op[i]);
		}
	}
	printf("\x1b[49m");
	printf("},\n");
}
