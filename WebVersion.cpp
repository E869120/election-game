#include <Siv3D.hpp> // OpenSiv3D v0.6.4
#include <ctime>
using namespace std;

struct Player {
	int Speech;    // 演説力
	int Kanban;    // 影響力
	int Jiban;     // 協力者
	int Kaban;     // 資金
	int NumSpc; // 演説回数
};

// 状態・日程など
int Situation = 0;
int Turn = 1;
int Life = 0;
int Card = 0, CardID = -1;

// ゲームの状況
String Level;
bool Hajimete = true;
bool HajimeteEnzetsu = true;
bool Choice[5][9];
bool Wakamono[5][9];
int Keisei[5][9], NewKeisei[5][9];
int VoteRate1 = 20;
int VoteRate2 = 60;
int NextAI = 1;
Player Z1, Z2;

// マウスの状況
double WaitTime = 100.00; int IsWaiting = 0;
double ButtonA[100];
double ButtonB[100][100];
double GetLastClick = 0.0;

// 投票確定情報
int Kakutei = 0;
int P1[5][9], P2[5][9];
int FinalVote1 = 0, FinalVote2 = 0, FinalVote3 = 0, TotalVote = 0;
int DisVote1 = 0, DisVote2 = 0;

// 勝率などの履歴
int History[20];
int History2[20][5][9];

// AI の動作確率
double probAI[6][14] = {
	{ 0.40, 0.05, 0.05, 0.10, 0.15, 0.15, 0.15, 0.10, 0.10, 0.72, 0.82, 0.86, 0.86, 0.86 },  // 演説
	{ 0.01, 0.01, 0.23, 0.23, 0.23, 0.50, 0.15, 0.10, 0.10, 0.15, 0.05, 0.01, 0.01, 0.01 },  // 演説を練る
	{ 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.63, 0.73, 0.73, 0.10, 0.10, 0.10, 0.10, 0.10 },  // 資金集め
	{ 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01 },  // 設備購入
	{ 0.17, 0.12, 0.20, 0.50, 0.45, 0.32, 0.05, 0.05, 0.05, 0.01, 0.01, 0.01, 0.01, 0.01 },  // 協力者集め
	{ 0.40, 0.80, 0.50, 0.15, 0.15, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01 }   // ＳＮＳ投稿
};

double Randouble() {
	double s = 0, t = 1;
	for (int i = 0; i < 3; i++) {
		t /= 1024.0;
		s += 1.0 * (rand() % 1024) * t;
	}
	return s;
}

bool AllZero() {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (Keisei[i][j] != 0) return false;
		}
	}
	return true;
}

int AI_Choice() {
	int Days = (Turn + 2) / 3 - 1, NextChoice = 5;

	// 14 日目の場合
	if (Turn == 42) {
		if (Z2.Kaban < 20) NextChoice = 2;
		else NextChoice = 0;
	}
	else if (Turn >= 40 && Turn <= 42) {
		if (Z2.Kaban < Z2.NumSpc * 20) NextChoice = 2;
		else NextChoice = 0;
	}

	// 金額が 350 万円を超えた場合
	else if (Z2.Kaban >= 350) {
		int v1 = (Z2.NumSpc + 0) * (43 - Turn); // 設備を買わない場合
		int v2 = (Z2.NumSpc + 1) * (42 - Turn); // 設備を買う場合
		if (v1 < v2) NextChoice = 3;
		else NextChoice = 0;
	}

	// その他
	else {
		while (true) {
			double ret = Randouble();
			for (int i = 0; i < 6; i++) {
				if (ret < probAI[i][Days]) { NextChoice = i; break; }
				ret -= probAI[i][Days];
			}
			if (NextChoice == 0 && Z2.Kaban < Z2.NumSpc * 20) continue;
			if (NextChoice == 1 && Z2.Speech > 0.8 * (43 - Turn)) continue;
			if (NextChoice == 3 && Z2.Kaban < 350) continue;
			if (NextChoice == 4 && Z2.Jiban > 280) continue;
			if (NextChoice == 0 && Days >= 10 && Days <= 12 && Z2.NumSpc == 1) continue;
			break;
		}
	}
	return NextChoice;
}

void AI_Enzetsu() {
	for (int t = 0; t < Z2.NumSpc; t++) {
		if (Z2.Kaban < 20) break;
		double min_diff = -1.0; int px = -1, py = -1;
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 9; j++) {
				double prob1 = 1.0 / (1.0 + exp(0.1 * Keisei[i][j]));
				double prob2 = 1.0 / (1.0 + exp(0.1 * (Keisei[i][j] - Z2.Speech)));
				if (min_diff < prob2 - prob1) {
					min_diff = prob2 - prob1;
					px = i; py = j;
				}
			}
		}
		Keisei[px][py] -= Z2.Speech;
		Z2.Kaban -= 20;
	}
}

double Norm() {
	double cnt = 0;
	for (int i = 0; i < 10000; i++) {
		if (rand() % 2 == 0) cnt += 1.0;
	}
	return (cnt - 5000.0) / 50.0;
}

void Vote_Kakutei() {
	double OverAll_Gosa = 0.5 * Norm();
	double Vote_Gosa = 1.5 * Norm();
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			double Ken_Gosa = 1.5 * Norm();
			double VoteRate = 0.01 * (VoteRate1 + Vote_Gosa);
			if (Wakamono[i][j] == false) VoteRate = 0.01 * (VoteRate2 + Vote_Gosa);
			VoteRate += 0.05 * Norm(); VoteRate = min(0.9999, max(0.0001, VoteRate));
			P1[i][j] = 10000.0 * VoteRate * (1.0 / (1.0 + exp(-(Keisei[i][j] + OverAll_Gosa + Ken_Gosa) / 10.0)));
			P2[i][j] = 10000.0 * VoteRate * (1.0 / (1.0 + exp(+(Keisei[i][j] + OverAll_Gosa + Ken_Gosa) / 10.0)));
		}
	}

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (Wakamono[i][j] == true)  FinalVote1 += P1[i][j] + P2[i][j];
			if (Wakamono[i][j] == false) FinalVote2 += P1[i][j] + P2[i][j];
			FinalVote3 += P1[i][j] + P2[i][j];
		}
	}
	TotalVote = FinalVote3;
	FinalVote1 /= 15;
	FinalVote2 /= 30;
	FinalVote3 /= 45;
}

void Initialize() {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			Keisei[i][j] = 0; Wakamono[i][j] = false;
		}
	}
	for (int t = 0; t < 15; t++) {
		while (true) {
			int sx = rand() % 5;
			int sy = rand() % 9;
			if (Wakamono[sx][sy] == false) { Wakamono[sx][sy] = true; break; }
		}
	}
	Z1.Speech = 5; Z1.Kanban = 0; Z1.Jiban = 10; Z1.Kaban = 100; Z1.NumSpc = 1;
	Z2.Speech = 5; Z2.Kanban = 0; Z2.Jiban = 10; Z2.Kaban = 100; Z2.NumSpc = 1;

	for (int i = 0; i < 15; i++) {
		History[i] = 50;
		for (int j = 0; j < 5; j++) {
			for (int k = 0; k < 9; k++) History2[i][j][k] = 0;
		}
	}
}

void ResetChoice() {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) { ButtonB[i][j] = 0; Choice[i][j] = false; }
	}
	for (int t = 0; t < 7; t++) {
		while (true) {
			int sx = rand() % 5;
			int sy = rand() % 9;
			if (Choice[sx][sy] == false) { Choice[sx][sy] = true; break; }
		}
	}
}

void Apply_SNS() {
	int qx = -1, qy = -1;

	// 自分の影響力
	while (true) {
		int sx = rand() % 5;
		int sy = rand() % 9;
		if (Wakamono[sx][sy] == true) { Keisei[sx][sy] += Z1.Kanban; qx = sx; qy = sy; break; }
	}

	// 相手の影響力
	while (true) {
		int sx = rand() % 5;
		int sy = rand() % 9;
		if (Wakamono[sx][sy] == true && (sx != qx || sy != qy)) { Keisei[sx][sy] -= Z2.Kanban; break; }
	}

	VoteRate1 += (Z1.Kanban + Z2.Kanban + 5) / 15;
}

void Refresh() {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			int dx[4] = { 1, 0, -1, 0 };
			int dy[4] = { 0, 1, 0, -1 };

			double score = Keisei[i][j], maxs = Keisei[i][j], mins = Keisei[i][j];
			for (int k = 0; k < 4; k++) {
				int rx = i + dx[k], ry = j + dy[k];
				if (rx < 0 || ry < 0 || rx >= 5 || ry >= 9) continue;
				score -= 0.05 * Keisei[i][j];
				score += 0.10 * Keisei[rx][ry];
				mins = min(mins, 1.0 * Keisei[rx][ry]);
				maxs = max(maxs, 1.0 * Keisei[rx][ry]);
			}
			double newscore = min(maxs, max(mins, score));
			if (newscore >= 0) NewKeisei[i][j] = (int)(0.5 + newscore);
			else NewKeisei[i][j] = -(int)(0.5 - newscore);
		}
	}
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) Keisei[i][j] = NewKeisei[i][j];
	}
}

void Main() {
	// 背景を白にする
	srand((unsigned)time(NULL));

# if SIV3D_PLATFORM(WEB)
	// Web 版では通常の 2 倍の解像度でレンダリング
	Scene::SetResizeMode(ResizeMode::Keep);
	Scene::Resize(1600, 1200);
# endif

	Scene::SetBackground(ColorF(1.0, 1.0, 1.0));

	// MSDF フォントを用意
	const Font font{ FontMethod::MSDF, 48 };
	// 初期化時に文字をプリロードしておくと、ゲームでその文字が初登場する際の瞬間的な FPS 低下を抑制できる
	font.preload(U"選挙戦へようこそ！あなたは、とある議員選挙の立候補者です。今日から 2 週間かけて選挙活動を行います。選挙で大切な三バン（地盤・カバン・看板）を獲得し、SNS の力も活用して有利に選挙戦を進めましょう！なお、このゲームの設定は、実際の選挙とは一切関係のないことに注意してください。実際の選挙の方が難しいです。はじめる");
	font.preload(U"予想投票率18～39歳：40歳以上：▼：若者の多い地区AI+30あなた※評価値の付け方は GitHub 参照");
	font.preload(U"演説力影響力協力者資金演説回数予測得票");

	// 初期値を設定
	Initialize();

	while (System::Update()) {

# if SIV3D_PLATFORM(WEB)
		// 描画とマウス座標を 2 倍スケーリング
		const Transformer2D tr{ Mat3x2::Scale(2.0), TransformCursor::Yes };
# endif

		double MouseX = Cursor::PosF().x;
		double MouseY = Cursor::PosF().y;


		// -----------------------------------------------------------------------------------------------------------------------
		// ---------------------------------------------- 待ち受け画面 -----------------------------------------------------------
		// -----------------------------------------------------------------------------------------------------------------------
		if (Situation == 0) {
			Rect(0, 0, 800, 600).draw(ColorF(0.25, 0.25, 0.55));
			font(U"Let's Win in the Election").draw(60, 45, 45, ColorF(1.00, 1.00, 1.00));
			font(U"～選挙で勝とう ゲーム版～").draw(30, 205, 150, ColorF(1.00, 1.00, 1.00));

			Rect(75, 300, 140, 140).draw(ColorF(1.0, 1.0, 1.0, 0.5 + 0.5 * ButtonA[10]));
			Rect(245, 300, 140, 140).draw(ColorF(1.0, 1.0, 1.0, 0.5 + 0.5 * ButtonA[11]));
			Rect(415, 300, 140, 140).draw(ColorF(1.0, 1.0, 1.0, 0.5 + 0.5 * ButtonA[12]));
			Rect(585, 300, 140, 140).draw(ColorF(1.0, 1.0, 1.0, 0.5 + 0.5 * ButtonA[13]));
			font(U"簡単").draw(20, 125, 335, ColorF(0.0, 0.0, 0.0)); font(U"自分が現職").draw(20, 95, 370, ColorF(0.0, 0.0, 0.0));
			font(U"普通").draw(20, 295, 335, ColorF(0.0, 0.0, 0.0)); font(U"新人対決").draw(20, 275, 370, ColorF(0.0, 0.0, 0.0));
			font(U"難しい").draw(20, 455, 335, ColorF(0.0, 0.0, 0.0)); font(U"相手が現職").draw(20, 437, 370, ColorF(0.0, 0.0, 0.0));
			font(U"とても難しい").draw(20, 595, 335, ColorF(0.0, 0.0, 0.0)); font(U"相手が10選").draw(20, 603, 370, ColorF(0.0, 0.0, 0.0));
			font(U"クリックしてレベルを選択").draw(30, 220, 500, ColorF(1.0, 1.0, 1.0, Periodic::Sine0_1(1.5s)));
			const RectF urlArea = font(U"※詳しいルールは https://github.com/E869120/election-game 参照").draw(10, 390, 570, ColorF(1.0, 1.0, 1.0));
			if (urlArea.mouseOver()) {
				Cursor::RequestStyle(CursorStyle::Hand);
				if (MouseL.down()) System::LaunchBrowser(U"https://github.com/E869120/election-game");
			}

			// マウスの状態
			int MouseState = -1;
			if (MouseX >= 75.0 && MouseX <= 215.0 && MouseY >= 300.0 && MouseY <= 440.0) MouseState = 0;
			if (MouseX >= 245.0 && MouseX <= 385.0 && MouseY >= 300.0 && MouseY <= 440.0) MouseState = 1;
			if (MouseX >= 415.0 && MouseX <= 555.0 && MouseY >= 300.0 && MouseY <= 440.0) MouseState = 2;
			if (MouseX >= 585.0 && MouseX <= 725.0 && MouseY >= 300.0 && MouseY <= 440.0) MouseState = 3;
			for (int i = 10; i < 14; i++) {
				if (i != MouseState + 10) ButtonA[i] = max(0.0, ButtonA[i] - 5.0 * Scene::DeltaTime());
				if (i == MouseState + 10) ButtonA[i] = min(1.0, ButtonA[i] + 5.0 * Scene::DeltaTime());
			}

			// キーが押された場合
			if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 1.2) {
				GetLastClick = Scene::Time();
				if (MouseState == 0) { Level = U"簡単"; Z1.Jiban = 100; Z2.Jiban = 40; }
				if (MouseState == 1) { Level = U"普通"; Z1.Jiban = 40; Z2.Jiban = 40; }
				if (MouseState == 2) { Level = U"難しい"; Z1.Jiban = 40; Z2.Jiban = 100; }
				if (MouseState == 3) { Level = U"とても難しい"; Z1.Jiban = 40; Z2.Jiban = 100; Z2.Kanban = 8; }
				if (MouseState != -1) { WaitTime = 0.0; Situation = 1; }
			}
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// ----------------------------------------------- 選挙運動中 ------------------------------------------------------------
		// -----------------------------------------------------------------------------------------------------------------------
		if (Situation == 1 || Situation == 2 || Situation == 3 || Situation == 4 || Situation == 5 || Situation == 6 || Situation == 8) {
			Rect(0, 330, 520, 270).draw(ColorF(0.90, 0.95, 1.00));
			Rect(520, 330, 280, 270).draw(ColorF(0.80, 0.90, 1.00));

			// 表の描画
			font(U"形勢速報").draw(20, 540, 340, ColorF(0.20, 0.20, 0.20));
			Rect(560, 385, 200, 196).draw(ColorF(0.25, 0.25, 0.55));
			Rect(640, 413, 120, 168).draw(ColorF(1.00, 1.00, 1.00));
			for (int i = 0; i <= 7; i++) Line(560, 385 + 28 * i, 760, 385 + 28 * i).draw(2, ColorF(0.20, 0.20, 0.20));
			Line(560, 385, 560, 581).draw(2, ColorF(0.20, 0.20, 0.20));
			Line(640, 385, 640, 581).draw(2, ColorF(0.20, 0.20, 0.20));
			Line(700, 385, 700, 581).draw(2, ColorF(0.20, 0.20, 0.20));
			Line(760, 385, 760, 581).draw(2, ColorF(0.20, 0.20, 0.20));

			// 形勢評価（計算）
			double ExpVotes = 0, TotalVotes = 0;
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					double keisuu = VoteRate1; if (Wakamono[i][j] == false) keisuu = VoteRate2;
					ExpVotes += keisuu / (1.0 + exp(-0.1 * Keisei[i][j]));
					TotalVotes += keisuu;
				}
			}
			int Exp1 = (int)(100.0 * ExpVotes / TotalVotes + 0.5);
			int Exp2 = 100 - Exp1;

			// 形勢評価（1列目）
			font(U"演説力").draw(15, 577, 414, ColorF(1.00, 1.00, 1.00));
			font(U"影響力").draw(15, 577, 442, ColorF(1.00, 1.00, 1.00));
			font(U"協力者").draw(15, 577, 470, ColorF(1.00, 1.00, 1.00));
			font(U"資金").draw(15, 585, 498, ColorF(1.00, 1.00, 1.00));
			font(U"演説回数").draw(15, 570, 526, ColorF(1.00, 1.00, 1.00));
			font(U"予測得票").draw(15, 570, 554, ColorF(1.00, 1.00, 1.00));

			// 形勢評価（2列目）
			font(U"あなた").draw(15, 647, 386, ColorF(1.00, 1.00, 1.00));
			font(U"Lv.").draw(15, 697 - 9 * (3 + to_string(Z1.Speech).size()), 414, ColorF(0.60, 0.10, 0.30)); font(Z1.Speech).draw(15, 693 - 9 * to_string(Z1.Speech).size(), 414, ColorF(0.60, 0.10, 0.30));
			font(U"Lv.").draw(15, 697 - 9 * (3 + to_string(Z1.Kanban).size()), 442, ColorF(0.60, 0.10, 0.30)); font(Z1.Kanban).draw(15, 693 - 9 * to_string(Z1.Kanban).size(), 442, ColorF(0.60, 0.10, 0.30));
			font(Z1.Jiban).draw(15, 680 - 9 * to_string(Z1.Jiban).size(), 470, ColorF(0.60, 0.10, 0.30)); font(U"人").draw(15, 680, 470, ColorF(0.60, 0.10, 0.30));
			font(Z1.Kaban).draw(15, 680 - 9 * to_string(Z1.Kaban).size(), 498, ColorF(0.60, 0.10, 0.30)); font(U"万").draw(15, 680, 498, ColorF(0.60, 0.10, 0.30));
			font(Z1.NumSpc).draw(15, 680 - 9 * to_string(Z1.NumSpc).size(), 526, ColorF(0.60, 0.10, 0.30)); font(U"回").draw(15, 680, 526, ColorF(0.60, 0.10, 0.30));
			font(Exp1).draw(15, 681 - 9 * to_string(Exp1).size(), 554, ColorF(0.60, 0.10, 0.30)); font(U"%").draw(15, 681, 554, ColorF(0.60, 0.10, 0.30));

			// 形勢評価（3列目）
			font(U"AI").draw(15, 721, 386, ColorF(1.00, 1.00, 1.00));
			font(U"Lv.").draw(15, 757 - 9 * (3 + to_string(Z2.Speech).size()), 414, ColorF(0.25, 0.25, 0.55)); font(Z2.Speech).draw(15, 753 - 9 * to_string(Z2.Speech).size(), 414, ColorF(0.25, 0.25, 0.55));
			font(U"Lv.").draw(15, 757 - 9 * (3 + to_string(Z2.Kanban).size()), 442, ColorF(0.25, 0.25, 0.55)); font(Z2.Kanban).draw(15, 753 - 9 * to_string(Z2.Kanban).size(), 442, ColorF(0.25, 0.25, 0.55));
			font(Z2.Jiban).draw(15, 740 - 9 * to_string(Z2.Jiban).size(), 470, ColorF(0.25, 0.25, 0.55)); font(U"人").draw(15, 740, 470, ColorF(0.25, 0.25, 0.55));
			font(Z2.Kaban).draw(15, 740 - 9 * to_string(Z2.Kaban).size(), 498, ColorF(0.25, 0.25, 0.55)); font(U"万").draw(15, 740, 498, ColorF(0.25, 0.25, 0.55));
			font(Z2.NumSpc).draw(15, 740 - 9 * to_string(Z2.NumSpc).size(), 526, ColorF(0.25, 0.25, 0.55)); font(U"回").draw(15, 740, 526, ColorF(0.25, 0.25, 0.55));
			font(Exp2).draw(15, 741 - 9 * to_string(Exp2).size(), 554, ColorF(0.25, 0.25, 0.55)); font(U"%").draw(15, 741, 554, ColorF(0.25, 0.25, 0.55));

			// 盤面の表示
			if (Situation == 3) {
				double wait_keisuu = 1.0;
				if (WaitTime <= 0.2) wait_keisuu = min(1.00, -1.25 * WaitTime);
				for (int i = 0; i < 5; i++) {
					for (int j = 0; j < 9; j++) {
						if (Choice[i][j] == true) Rect(45 + j * 50, 40 + i * 50, 50, 50).draw(ColorF(1.0, 0.6, 0.2, (0.4 + 0.6 * ButtonB[i][j]) * wait_keisuu));
					}
				}
			}
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					double ra = 0, rb = 0, rc = 0;
					if (Keisei[i][j] >= 30) { ra = 1.00; rb = 0.10; rc = 0.55; }
					else if (Keisei[i][j] >= 15) {
						ra = 0.95 + 0.05 * (Keisei[i][j] - 15) / 15.0;
						rb = 0.35 - 0.25 * (Keisei[i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (Keisei[i][j] - 15) / 15.0;
					}
					else if (Keisei[i][j] >= 1) {
						ra = 0.86 + 0.09 * (Keisei[i][j] - 0) / 15.0;
						rb = 0.80 - 0.45 * (Keisei[i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (Keisei[i][j] - 0) / 15.0;
					}
					else if (Keisei[i][j] >= 0) {
						ra = 0.87;
						rb = 0.87;
						rc = 0.87;
					}
					else if (Keisei[i][j] >= -15) {
						ra = 0.81 - 0.36 * (-Keisei[i][j] - 0) / 15.0;
						rb = 0.81 - 0.36 * (-Keisei[i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (-Keisei[i][j] - 0) / 15.0;
					}
					else if (Keisei[i][j] >= -30) {
						ra = 0.45 - 0.20 * (-Keisei[i][j] - 15) / 15.0;
						rb = 0.45 - 0.20 * (-Keisei[i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (-Keisei[i][j] - 15) / 15.0;
					}
					else { ra = 0.25; rb = 0.25; rc = 0.55; }
					Rect(50 + j * 50, 45 + i * 50, 40, 40).draw(ColorF(ra, rb, rc));
					if (Wakamono[i][j] == true) font(U"▼").draw(15, 62 + j * 50, 45 + i * 50);
				}
			}

			// 予想投票率
			VoteRate1 = min(99, VoteRate1);
			VoteRate2 = min(99, VoteRate2);
			Rect(550, 165, 200, 120).drawFrame(1, 1, ColorF(0.20, 0.20, 0.20));
			Line(550, 73, 670, 73).draw(2, ColorF(0.00, 0.00, 0.00));
			font(U"予想投票率").draw(20, 550, 45, ColorF(0.20, 0.20, 0.20));
			font(U"18～39歳：").draw(20, 575, 80, ColorF(0.20, 0.20, 0.20)); font(VoteRate1).draw(20, 705, 80, ColorF(0.20, 0.20, 0.20)); font(U"%").draw(20, 730, 80, ColorF(0.20, 0.20, 0.20));
			font(U"40歳以上：").draw(20, 575, 105, ColorF(0.20, 0.20, 0.20)); font(VoteRate2).draw(20, 705, 105, ColorF(0.20, 0.20, 0.20)); font(U"%").draw(20, 730, 105, ColorF(0.20, 0.20, 0.20));

			// 凡例
			font(U"▼：若者の多い地区").draw(15, 570, 180, ColorF(0.20, 0.20, 0.20));
			Rect(570, 215, 20, 20).draw(ColorF(0.25, 0.25, 0.55));
			Rect(593, 215, 20, 20).draw(ColorF(0.41, 0.41, 0.63));
			Rect(616, 215, 20, 20).draw(ColorF(0.57, 0.57, 0.71));
			Rect(639, 215, 20, 20).draw(ColorF(0.85, 0.85, 0.85));
			Rect(662, 215, 20, 20).draw(ColorF(0.92, 0.50, 0.71));
			Rect(685, 215, 20, 20).draw(ColorF(0.96, 0.30, 0.63));
			Rect(708, 215, 20, 20).draw(ColorF(1.00, 0.10, 0.55));
			font(U"AI").draw(15, 570, 235, ColorF(0.25, 0.25, 0.55));
			font(U"+30").draw(15, 570, 251, ColorF(0.25, 0.25, 0.55));
			font(U"あなた").draw(15, 685, 235, ColorF(1.00, 0.10, 0.55));
			font(U"+30").draw(15, 699, 251, ColorF(1.00, 0.10, 0.55));
			font(U"評価値").draw(15, 628, 235, ColorF(0.50, 0.50, 0.50));
			/*font20(U"凡例").draw(560, 166, ColorF(1.00, 1.00, 1.00));
			font15(U"▼：若者の多い地区").draw(570, 205, ColorF(0.20, 0.20, 0.20));
			Rect(570, 235, 20, 20).draw(ColorF(0.25, 0.25, 0.55));
			Rect(593, 235, 20, 20).draw(ColorF(0.45, 0.45, 0.65));
			Rect(616, 235, 20, 20).draw(ColorF(0.65, 0.65, 0.75));
			Rect(639, 235, 20, 20).draw(ColorF(0.85, 0.85, 0.85));
			Rect(662, 235, 20, 20).draw(ColorF(0.90, 0.60, 0.75));
			Rect(685, 235, 20, 20).draw(ColorF(0.95, 0.35, 0.65));
			Rect(708, 235, 20, 20).draw(ColorF(1.00, 0.10, 0.55));
			font15(U"AI優勢").draw(570, 255, ColorF(0.25, 0.25, 0.55));
			font15(U"あなた優勢").draw(655, 255, ColorF(1.00, 0.10, 0.55));*/

			// 左側の表示
			font(U"選挙戦").draw(20, 15, 340, ColorF(0.20, 0.20, 0.20));
			font((Turn + 2) / 3).draw(20, 100 - 12 * to_string((Turn + 2) / 3).size(), 340, ColorF(0.20, 0.20, 0.20));
			font(U"日目 [全14日]").draw(20, 100, 340, ColorF(0.20, 0.20, 0.20));
			if (Turn % 3 == 1) font(U"朝").draw(20, 250, 340, ColorF(0.20, 0.20, 0.20));
			if (Turn % 3 == 2) font(U"昼").draw(20, 250, 340, ColorF(0.20, 0.20, 0.20));
			if (Turn % 3 == 0) font(U"夜").draw(20, 250, 340, ColorF(0.20, 0.20, 0.20));
			if (Situation == 2) {
				Rect(352, 338, 160, 30).draw(ColorF(0.25, 0.25, 0.55, min(1.00, 2.0 * abs(WaitTime))));
				font(U"AIのターン").draw(20, 380, 338, ColorF(1.00, 1.00, 1.00, min(1.00, 2.0 * abs(WaitTime))));
			}
			else {
				Rect(352, 338, 160, 30).draw(ColorF(0.60, 0.10, 0.30, min(1.00, 2.0 * abs(WaitTime))));
				font(U"あなたのターン").draw(20, 362, 338, ColorF(1.00, 1.00, 1.00, min(1.00, 2.0 * abs(WaitTime))));
			}

			// 下半分の描画
			if (Situation == 1 || Situation == 2) {
				int offset[6] = { 0, 0, 0, 0, 0, 0 };
				if (WaitTime >= -0.05 && WaitTime <= 1.2) {
					for (int i = 0; i < 6; i++) offset[i] = max(0, (int)(400 * (i + 2) - 2400.0 * WaitTime));
				}
				if (Situation == 1) {
					Rect(20 - offset[0], 385, 24, 24).draw(ColorF(1.00 - 0.40 * ButtonA[0], 1.00 - 0.90 * ButtonA[0], 1.00 - 0.70 * ButtonA[0]));
					Rect(20 - offset[1], 419, 24, 24).draw(ColorF(1.00 - 0.40 * ButtonA[1], 1.00 - 0.90 * ButtonA[1], 1.00 - 0.70 * ButtonA[1]));
					Rect(20 - offset[2], 453, 24, 24).draw(ColorF(1.00 - 0.40 * ButtonA[2], 1.00 - 0.90 * ButtonA[2], 1.00 - 0.70 * ButtonA[2]));
					Rect(20 - offset[3], 487, 24, 24).draw(ColorF(1.00 - 0.40 * ButtonA[3], 1.00 - 0.90 * ButtonA[3], 1.00 - 0.70 * ButtonA[3]));
					Rect(20 - offset[4], 521, 24, 24).draw(ColorF(1.00 - 0.40 * ButtonA[4], 1.00 - 0.90 * ButtonA[4], 1.00 - 0.70 * ButtonA[4]));
					Rect(20 - offset[5], 555, 24, 24).draw(ColorF(1.00 - 0.40 * ButtonA[5], 1.00 - 0.90 * ButtonA[5], 1.00 - 0.70 * ButtonA[5]));
				}
				else {
					Rect(20 - offset[0], 385, 24, 24).draw(ColorF(1.00 - 0.75 * ButtonA[0], 1.00 - 0.75 * ButtonA[0], 1.00 - 0.45 * ButtonA[0]));
					Rect(20 - offset[1], 419, 24, 24).draw(ColorF(1.00 - 0.75 * ButtonA[1], 1.00 - 0.75 * ButtonA[1], 1.00 - 0.45 * ButtonA[1]));
					Rect(20 - offset[2], 453, 24, 24).draw(ColorF(1.00 - 0.75 * ButtonA[2], 1.00 - 0.75 * ButtonA[2], 1.00 - 0.45 * ButtonA[2]));
					Rect(20 - offset[3], 487, 24, 24).draw(ColorF(1.00 - 0.75 * ButtonA[3], 1.00 - 0.75 * ButtonA[3], 1.00 - 0.45 * ButtonA[3]));
					Rect(20 - offset[4], 521, 24, 24).draw(ColorF(1.00 - 0.75 * ButtonA[4], 1.00 - 0.75 * ButtonA[4], 1.00 - 0.45 * ButtonA[4]));
					Rect(20 - offset[5], 555, 24, 24).draw(ColorF(1.00 - 0.75 * ButtonA[5], 1.00 - 0.75 * ButtonA[5], 1.00 - 0.45 * ButtonA[5]));
				}
				Rect(20 - offset[0], 385, 24, 24).drawFrame(2, ColorF(0.20, 0.20, 0.20));
				Rect(20 - offset[1], 419, 24, 24).drawFrame(2, ColorF(0.20, 0.20, 0.20));
				Rect(20 - offset[2], 453, 24, 24).drawFrame(2, ColorF(0.20, 0.20, 0.20));
				Rect(20 - offset[3], 487, 24, 24).drawFrame(2, ColorF(0.20, 0.20, 0.20));
				Rect(20 - offset[4], 521, 24, 24).drawFrame(2, ColorF(0.20, 0.20, 0.20));
				Rect(20 - offset[5], 555, 24, 24).drawFrame(2, ColorF(0.20, 0.20, 0.20));

				// 文字の表示
				font(U"演説をする（資金 -20万、形勢向上）").draw(15, 55 - offset[0], 385, ColorF(0.20, 0.20, 0.20));
				font(U"演説の練習（演説力 +1）").draw(15, 55 - offset[1], 419, ColorF(0.20, 0.20, 0.20));
				font(U"寄付を募る（協力者×1万だけ資金増）").draw(15, 55 - offset[2], 453, ColorF(0.20, 0.20, 0.20));
				font(U"設備の購入（資金 -350万、演説回数 +1）").draw(15, 55 - offset[3], 487, ColorF(0.20, 0.20, 0.20));
				font(U"地盤の強化（協力者が 10+影響力 増える）").draw(15, 55 - offset[4], 521, ColorF(0.20, 0.20, 0.20));
				font(U"ＳＮＳ投稿（若者への影響力 +1）").draw(15, 55 - offset[5], 555, ColorF(0.20, 0.20, 0.20));
			}

			// 自分の行動
			if (Situation == 1) {
				font(U"クリックで").draw(20, 380, 520, ColorF(0.2, 0.2, 0.2, Periodic::Sine0_1(1.5s)));
				font(U"行動を選択！").draw(20, 380, 550, ColorF(0.2, 0.2, 0.2, Periodic::Sine0_1(1.5s)));
				font(U"資金が不足").draw(20, 380, 420, ColorF(0.6, 0.1, 0.3, ButtonA[6]));
				font(U"しています").draw(20, 380, 450, ColorF(0.6, 0.1, 0.3, ButtonA[6]));

				// 遷移状態の場合
				if (WaitTime <= 0.20) Rect(0, 380, 500, 220).draw(ColorF(0.90, 0.95, 1.00, 1.0 + 2.0 * WaitTime));
				if (WaitTime >= 0.00 && IsWaiting == 1) {
					for (int i = 0; i < 14; i++) ButtonA[i] = 0;
					IsWaiting = 0; Situation = 2; NextAI = AI_Choice();
				}
				if (WaitTime >= 0.00 && IsWaiting == 2) { Situation = 3; IsWaiting = 0; Life = min(Z1.NumSpc, Z1.Kaban / 20); ResetChoice(); }

				// 初めての場合
				if (Hajimete == true) {
					Rect(100, 100, 600, 400).draw(ColorF(1.0, 1.0, 1.0, 0.9));
					Rect(100, 100, 600, 400).drawFrame(3, ColorF(0.2, 0.2, 0.2));
					font(U"選挙戦へようこそ！").draw(50, 120, 110, ColorF(0.2, 0.2, 0.2));
					font(U"あなたは、とある議員選挙の立候補者です。今日から 2 週間かけて").draw(18, 120, 190, ColorF(0.2, 0.2, 0.2));
					font(U"選挙活動を行います。選挙で大切な三バン（地盤・カバン・看板）").draw(18, 120, 220, ColorF(0.2, 0.2, 0.2));
					font(U"を獲得し、SNS の力も活用して有利に選挙戦を進めましょう！").draw(18, 120, 250, ColorF(0.2, 0.2, 0.2));
					font(U"なお、このゲームの設定は、").draw(18, 120, 295, ColorF(0.2, 0.2, 0.2));
					font(U"実際の選挙とは一切関係のない").draw(18, 354, 295, ColorF(1.0, 0.0, 0.0));
					font(U"ことに注").draw(18, 606, 295, ColorF(0.2, 0.2, 0.2));
					font(U"意してください。実際の選挙の方が難しいです。").draw(18, 120, 325, ColorF(0.2, 0.2, 0.2));
					Rect(250, 390, 300, 70).draw(ColorF(0.40, 0.40, 0.70, 0.50 + 0.50 * ButtonA[14]));
					font(U"はじめる").draw(30, 340, 400, ColorF(1.00, 1.00, 1.00));
				}
			}

			// AI の行動
			if (Situation == 2) {
				// ボタン・時間の更新
				WaitTime += Scene::DeltaTime();
				if (WaitTime >= 1.4) {
					ButtonA[NextAI] = 1;
					WaitTime = -2.4; IsWaiting = 1;
					if (NextAI == 0) { AI_Enzetsu(); }
					if (NextAI == 1) { Z2.Speech += 1; }
					if (NextAI == 2) { Z2.Kaban += Z2.Jiban; }
					if (NextAI == 3) { Z2.Kaban -= 350; Z2.NumSpc += 1; }
					if (NextAI == 4) { Z2.Jiban += 10 + Z2.Kanban; }
					if (NextAI == 5) { Z2.Kanban += 1; }
				}

				// 遷移状態の場合
				if (WaitTime <= 0.20) Rect(0, 380, 500, 220).draw(ColorF(0.90, 0.95, 1.00, 1.0 + 2.0 * WaitTime));
				if (WaitTime >= 0.00 && IsWaiting == 1) {
					for (int i = 0; i < 14; i++) ButtonA[i] = 0;
					Turn += 1; IsWaiting = 0; Situation = 1;

					if (Turn == 43) { Refresh(); Situation = 6; }
					else if (Turn % 3 == 1 && HajimeteEnzetsu == true && AllZero() == false) { Refresh(); HajimeteEnzetsu = false; Situation = 8; }
					else if (Turn % 3 == 1) { Refresh(); Situation = 4; }

					// 履歴の更新
					if (Turn % 3 == 1) {
						History[(Turn - 1) / 3] = Exp1;
						for (int i = 0; i < 5; i++) {
							for (int j = 0; j < 9; j++) History2[(Turn - 1) / 3][i][j] = Keisei[i][j];
						}
					}
				}
			}

			// 演説場所を選ぶ
			if (Situation == 3) {
				font(U"黄色で表示されている選択肢の中から").draw(20, 90, 500, ColorF(0.2, 0.2, 0.2, Periodic::Sine0_1(1.5s)));
				font(U"演説する地区をクリックしてください").draw(20, 90, 530, ColorF(0.2, 0.2, 0.2, Periodic::Sine0_1(1.5s)));
				font(U"残り演説回数").draw(30, 100, 430, ColorF(0.2, 0.2, 0.2));
				font(Life).draw(60, 310, 400, ColorF(0.6, 0.1, 0.3));
				font(U"/").draw(60, 346, 400, ColorF(0.2, 0.2, 0.2));
				font(Z1.NumSpc).draw(60, 382, 400, ColorF(0.2, 0.2, 0.2));
				font(U"※演説した場所の形勢が [演説力レベル] だけ上がります。").draw(10, 245, 575, ColorF(0.2, 0.2, 0.2));

				// 遷移状態の場合
				if (WaitTime <= 0.2) Rect(0, 380, 500, 220).draw(ColorF(0.90, 0.95, 1.00, min(1.00, 1.0 + 2.0 * WaitTime)));
				if (WaitTime >= 0.0 && IsWaiting == 1) {
					for (int i = 0; i < 14; i++) ButtonA[i] = 0;
					IsWaiting = 0; Situation = 2; NextAI = AI_Choice();
				}
			}

			if (Situation == 8) {
				font(U"一日の終わりには").draw(20, 45, 405, ColorF(0.2, 0.2, 0.2));
				font(U"形勢が隣に拡散します").draw(20, 45, 435, ColorF(0.2, 0.2, 0.2));
				Rect(150, 500, 220, 55).draw(ColorF(1.00, 1.00 - 0.20 * ButtonA[7], 1.00 - 0.40 * ButtonA[7], min(1.00, abs(WaitTime) * 2.0)));
				font(U"確認する").draw(30, 200, 505, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));
				font(U"※ 3 日目以降も、一日の終わりには同様のことが起こります。").draw(10, 220, 575, ColorF(0.2, 0.2, 0.2));

				// 左側の正方形
				for (int i = 0; i < 3; i++) {
					for (int j = 0; j < 3; j++) {
						double ra = 0.85, rb = 0.85, rc = 0.85;
						if (i == 1 && j == 1) { ra = 0.60; rb = 0.10; rc = 0.30; }
						Rect(300 + i * 20, 405 + j * 20, 16, 16).draw(ColorF(ra, rb, rc));
					}
				}

				// 右側の正方形
				for (int i = 0; i < 3; i++) {
					for (int j = 0; j < 3; j++) {
						double ra = 0.85, rb = 0.85, rc = 0.85;
						if (i == 1 && j == 1) { ra = 0.60; rb = 0.10; rc = 0.30; }
						else if (abs(i - 1) + abs(j - 1) <= 1) { ra = 0.75; rb = 0.60; rc = 0.65; }
						Rect(420 + i * 20, 405 + j * 20, 16, 16).draw(ColorF(ra, rb, rc));
					}
				}

				// 矢印を描く
				Rect(370, 424, 30, 12).draw(ColorF(0.2, 0.2, 0.2));
				Triangle(400, 420, 400, 440, 415, 430).draw(ColorF(0.2, 0.2, 0.2));

				// 遷移状態の場合
				if (WaitTime >= 0.0 && IsWaiting == 1) {
					IsWaiting = 0; Situation = 4;
				}
			}

			if (Situation == 4) {
				font(U"深夜に、ＳＮＳを通じた選挙活動が").draw(20, 100, 420, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));
				font(U"若者に対して行われました").draw(20, 140, 450, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));
				Rect(150, 500, 220, 55).draw(ColorF(1.00, 1.00 - 0.20 * ButtonA[7], 1.00 - 0.40 * ButtonA[7], min(1.00, abs(WaitTime) * 2.0)));
				font(U"確認する").draw(30, 200, 505, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));
				font(U"※若者の多い地区 1 箇所の形勢が [影響力レベル] だけ上がります。").draw(10, 200, 575, ColorF(0.2, 0.2, 0.2));

				// 遷移状態の場合
				if (WaitTime >= 0.0 && IsWaiting == 1) {
					IsWaiting = 0; Situation = 5; Card = 0; CardID = -1;
					Apply_SNS();
				}
			}

			if (Situation == 5) {
				font(U"選挙戦").draw(20, 110, 420, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));
				font((Turn + 2) / 3).draw(20, 193 - 6 * to_string((Turn + 2) / 3).size(), 420, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));
				font(U"日目が始まりました！").draw(20, 210, 420, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));
				font(U"「チャンスカード」を引いてください").draw(20, 90, 450, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));
				Rect(150, 500, 220, 55).draw(ColorF(1.00, 1.00 - 0.20 * ButtonA[7], 1.00 - 0.40 * ButtonA[7], min(1.00, abs(WaitTime) * 2.0)));
				font(U"カードを引く").draw(30, 170, 505, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));

				// カードがある場合
				if (Card == 1 && WaitTime >= 0.05) {
					Rect(240, 180, 320, 240).draw(ColorF(1.0, 1.0, 0.5, 0.9));
					Rect(240, 180, 320, 240).drawFrame(3, ColorF(0.2, 0.2, 0.2));

					if (CardID == 0) {
						font(U"選挙への関心").draw(25, 250, 190, ColorF(0.2, 0.2, 0.2));
						font(U"若者の投票率が 10% 上昇する。").draw(15, 250, 240, ColorF(0.2, 0.2, 0.2));
					}
					if (CardID == 1) {
						font(U"物価の上昇").draw(25, 250, 190, ColorF(0.2, 0.2, 0.2));
						font(U"両候補の資金が 40 万円減少する。").draw(15, 250, 240, ColorF(0.2, 0.2, 0.2));
					}
					if (CardID == 2) {
						font(U"テレビ出演").draw(25, 250, 190, ColorF(0.2, 0.2, 0.2));
						font(U"影響力レベルが 2 上昇する。").draw(15, 250, 240, ColorF(0.2, 0.2, 0.2));
						font(U"投票率が 4% 上昇する。").draw(15, 250, 270, ColorF(0.2, 0.2, 0.2));
					}
					if (CardID == 3) {
						font(U"対立候補が新聞掲載").draw(25, 250, 190, ColorF(0.2, 0.2, 0.2));
						font(U"相手の影響力レベルが 2 上昇する。").draw(15, 250, 240, ColorF(0.2, 0.2, 0.2));
						font(U"投票率が 3% 上昇する。").draw(15, 250, 270, ColorF(0.2, 0.2, 0.2));
					}
					if (CardID == 4) {
						font(U"人柄が評価される").draw(25, 250, 190, ColorF(0.2, 0.2, 0.2));
						font(U"すべての地区における自分の形勢が").draw(15, 250, 240, ColorF(0.2, 0.2, 0.2));
						font(U"1 ポイント上昇する。").draw(15, 250, 270, ColorF(0.2, 0.2, 0.2));
					}
					if (CardID == 5) {
						font(U"まさかの失言！？").draw(25, 250, 190, ColorF(0.2, 0.2, 0.2));
						font(U"すべての地区における自分の形勢が").draw(15, 250, 240, ColorF(0.2, 0.2, 0.2));
						font(U"1 ポイント低下する。").draw(15, 250, 270, ColorF(0.2, 0.2, 0.2));
					}
					if (CardID == 6) {
						font(U"推薦状が得られた！").draw(25, 250, 190, ColorF(0.2, 0.2, 0.2));
						font(U"協力者が 25 人増える。").draw(15, 250, 240, ColorF(0.2, 0.2, 0.2));
					}
					if (CardID == 7) {
						font(U"相手が後援会を作った！").draw(25, 250, 190, ColorF(0.2, 0.2, 0.2));
						font(U"相手の協力者が 25 人増える。").draw(15, 250, 240, ColorF(0.2, 0.2, 0.2));
					}
					if (CardID == 8) {
						font(U"宝くじで当選").draw(25, 250, 190, ColorF(0.2, 0.2, 0.2));
						font(U"資金が 70 万円増える。").draw(15, 250, 240, ColorF(0.2, 0.2, 0.2));
					}
					if (CardID == 9) {
						font(U"交通事故発生").draw(25, 250, 190, ColorF(0.2, 0.2, 0.2));
						font(U"選挙カーが事故に遭う。").draw(15, 250, 240, ColorF(0.2, 0.2, 0.2));
						font(U"70 万円支払う。").draw(15, 250, 270, ColorF(0.2, 0.2, 0.2));
					}
					Rect(290, 330, 220, 55).draw(ColorF(1.00, 1.00 - 0.20 * ButtonA[8], 1.00 - 0.40 * ButtonA[8], min(1.00, abs(WaitTime) * 2.0)));
					font(U"確認する").draw(30, 340, 335, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));
				}

				// 遷移状態の場合
				if (WaitTime <= 0.2) Rect(0, 380, 500, 220).draw(ColorF(0.90, 0.95, 1.00, min(1.00, 1.0 + 2.0 * WaitTime)));
				if (WaitTime >= 0.0 && IsWaiting == 1) {
					IsWaiting = 0; Situation = 1; Card = 0;
					if (CardID == 0) { VoteRate1 += 10; }
					if (CardID == 1) { Z1.Kaban = max(0, Z1.Kaban - 40); Z2.Kaban = max(0, Z2.Kaban - 40); }
					if (CardID == 2) { Z1.Kanban += 2; VoteRate1 += 4; VoteRate2 += 4; }
					if (CardID == 3) { Z2.Kanban += 2; VoteRate1 += 3; VoteRate2 += 3; }
					if (CardID == 4) { for (int i = 0; i < 45; i++) Keisei[i / 9][i % 9] += 1; }
					if (CardID == 5) { for (int i = 0; i < 45; i++) Keisei[i / 9][i % 9] -= 1; }
					if (CardID == 6) { Z1.Jiban += 25; }
					if (CardID == 7) { Z2.Jiban += 25; }
					if (CardID == 8) { Z1.Kaban += 70; }
					if (CardID == 9) { Z1.Kaban = max(0, Z1.Kaban - 70); }
				}
			}

			if (Situation == 6) {
				font(U"いよいよ投票日です！").draw(20, 160, 420, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));
				Rect(150, 500, 220, 55).draw(ColorF(1.00, 1.00 - 0.20 * ButtonA[7], 1.00 - 0.40 * ButtonA[7], min(1.00, abs(WaitTime) * 2.0)));
				font(U"開票に進む").draw(30, 185, 505, ColorF(0.2, 0.2, 0.2, min(1.00, abs(WaitTime) * 2.0)));

				// 遷移状態の場合
				if (WaitTime <= 0.2 && IsWaiting == 1) Rect(0, 0, 800, 600).draw(ColorF(1.00, 1.00, 1.00, min(1.00, 1.0 + 1.25 * WaitTime)));
				if (WaitTime >= 0.0 && IsWaiting == 1) {
					IsWaiting = 0; Situation = 7; WaitTime = 0; Vote_Kakutei();
				}
			}


			// ################################################
			// ##                                            ##
			// ##            以下、マウスの情報              ##
			// ##                                            ##
			// ################################################
			if (Situation == 1 && Hajimete == true) {
				// マウスの状態
				int MouseState = -1;
				if (MouseX >= 250.0 && MouseX <= 550.0 && MouseY >= 390.0 && MouseY <= 460.0) MouseState = 0;
				if (MouseState == 0) ButtonA[14] = min(1.0, ButtonA[14] + 5.0 * Scene::DeltaTime());
				if (MouseState != 0) ButtonA[14] = max(0.0, ButtonA[14] - 5.0 * Scene::DeltaTime());

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down()) {
					GetLastClick = Scene::Time();
					if (MouseState == 0) Hajimete = false;
				}
			}
			if (Situation == 1 && Hajimete == false) {
				// マウスの状態
				int MouseState = -1;
				if (MouseX >= 20.0 && MouseX <= 44.0 && MouseY >= 385.0 && MouseY <= 409.0) MouseState = 0;
				if (MouseX >= 20.0 && MouseX <= 44.0 && MouseY >= 419.0 && MouseY <= 443.0) MouseState = 1;
				if (MouseX >= 20.0 && MouseX <= 44.0 && MouseY >= 453.0 && MouseY <= 477.0) MouseState = 2;
				if (MouseX >= 20.0 && MouseX <= 44.0 && MouseY >= 487.0 && MouseY <= 511.0) MouseState = 3;
				if (MouseX >= 20.0 && MouseX <= 44.0 && MouseY >= 521.0 && MouseY <= 545.0) MouseState = 4;
				if (MouseX >= 20.0 && MouseX <= 44.0 && MouseY >= 555.0 && MouseY <= 579.0) MouseState = 5;

				// ボタン・時間の更新
				for (int i = 0; i < 6; i++) {
					if (i != MouseState) ButtonA[i] = max(0.0, ButtonA[i] - 5.0 * Scene::DeltaTime());
					if (i == MouseState) ButtonA[i] = min(1.0, ButtonA[i] + 5.0 * Scene::DeltaTime());
				}
				ButtonA[6] = max(0.0, ButtonA[6] - 1.0 * Scene::DeltaTime());
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 1.2) {
					GetLastClick = Scene::Time();
					bool NextMove = false;
					if (MouseState == 0) {
						if (Z1.Kaban >= 20) { NextMove = true; }
						else ButtonA[6] = 1.0;
					}
					if (MouseState == 3) {
						if (Z1.Kaban >= 350) { NextMove = true; Z1.Kaban -= 350; Z1.NumSpc += 1; }
						else ButtonA[6] = 1.0;
					}
					if (MouseState == 1) { Z1.Speech += 1; NextMove = true; }
					if (MouseState == 2) { Z1.Kaban += Z1.Jiban; NextMove = true; }
					if (MouseState == 4) { Z1.Jiban += 10 + Z1.Kanban; NextMove = true; }
					if (MouseState == 5) { Z1.Kanban += 1; NextMove = true; }
					if (NextMove == true) {
						WaitTime = -0.8;
						if (MouseState == 0) IsWaiting = 2;
						if (MouseState != 0) IsWaiting = 1;
					}
				}
			}

			if (Situation == 3) {
				int StateX = -1, StateY = -1;
				if (InRange(MouseX, 45.0, 495.0) && InRange(MouseY, 40.0, 490.0)) {
					StateY = max(0, min(8, (int)((MouseX - 45.0) / 50.0)));
					StateX = max(0, min(4, (int)((MouseY - 40.0) / 50.0)));
				}
				for (int i = 0; i < 5; i++) {
					for (int j = 0; j < 9; j++) {
						if (i == StateX && j == StateY) ButtonB[i][j] = min(1.0, ButtonB[i][j] + 5.0 * Scene::DeltaTime());
						else ButtonB[i][j] = max(0.0, ButtonB[i][j] - 5.0 * Scene::DeltaTime());
					}
				}
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
					GetLastClick = Scene::Time();
					if (StateX != -1 && Choice[StateX][StateY] == true) {
						Keisei[StateX][StateY] += Z1.Speech;
						Life -= 1;
						Z1.Kaban -= 20;
						if (Life == 0) { WaitTime = -0.8; IsWaiting = 1; }
					}
				}
			}

			if (Situation == 8) {
				int MouseState = -1;
				if (MouseX >= 150.0 && MouseX <= 370.0 && MouseY >= 500.0 && MouseY <= 555.0) MouseState = 0;
				if (MouseState == 0) ButtonA[7] = min(1.0, ButtonA[7] + 5.0 * Scene::DeltaTime());
				if (MouseState != 0) ButtonA[7] = max(0.0, ButtonA[7] - 5.0 * Scene::DeltaTime());
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
					GetLastClick = Scene::Time();
					if (MouseState == 0) { WaitTime = -0.45; IsWaiting = 1; }
				}
			}

			if (Situation == 4) {
				int MouseState = -1;
				if (MouseX >= 150.0 && MouseX <= 370.0 && MouseY >= 500.0 && MouseY <= 555.0) MouseState = 0;
				if (MouseState == 0) ButtonA[7] = min(1.0, ButtonA[7] + 5.0 * Scene::DeltaTime());
				if (MouseState != 0) ButtonA[7] = max(0.0, ButtonA[7] - 5.0 * Scene::DeltaTime());
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
					GetLastClick = Scene::Time();
					if (MouseState == 0) { WaitTime = -0.45; IsWaiting = 1; }
				}
			}

			if (Situation == 5) {
				int MouseState = -1;
				if (MouseX >= 150.0 && MouseX <= 370.0 && MouseY >= 500.0 && MouseY <= 555.0) MouseState = 0;
				if (MouseX >= 290.0 && MouseX <= 510.0 && MouseY >= 330.0 && MouseY <= 385.0) MouseState = 1;
				if (MouseState == 0 && Card == 0) ButtonA[7] = min(1.0, ButtonA[7] + 5.0 * Scene::DeltaTime());
				if (MouseState != 0 || Card != 0) ButtonA[7] = max(0.0, ButtonA[7] - 5.0 * Scene::DeltaTime());
				if (MouseState == 1 && Card == 1) ButtonA[8] = min(1.0, ButtonA[8] + 5.0 * Scene::DeltaTime());
				if (MouseState != 1 || Card != 1) ButtonA[8] = max(0.0, ButtonA[8] - 5.0 * Scene::DeltaTime());
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
					GetLastClick = Scene::Time();
					if (MouseState == 0 && Card == 0) {
						Card = 1; int val = rand() % 200;
						if (val < 92) CardID = val / 23;
						else if (val < 100) CardID = 4;
						else if (val < 108) CardID = 5;
						else CardID = (val - 108) / 23 + 6;
					}
					if (MouseState == 1 && Card == 1) { WaitTime = -0.45; IsWaiting = 1; }
				}
			}

			if (Situation == 6) {
				int MouseState = -1;
				if (MouseX >= 150.0 && MouseX <= 370.0 && MouseY >= 500.0 && MouseY <= 555.0) MouseState = 0;
				if (MouseState == 0) ButtonA[7] = min(1.0, ButtonA[7] + 5.0 * Scene::DeltaTime());
				if (MouseState != 0) ButtonA[7] = max(0.0, ButtonA[7] - 5.0 * Scene::DeltaTime());
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
					GetLastClick = Scene::Time();
					if (MouseState == 0) { WaitTime = -0.80; IsWaiting = 1; }
				}
			}
		}


		// -----------------------------------------------------------------------------------------------------------------------
		// ------------------------------------------------- 開票中 --------------------------------------------------------------
		// -----------------------------------------------------------------------------------------------------------------------
		if (Situation == 7) {
			font(U"開票速報").draw(60, 20, 10, ColorF(0.2, 0.2, 0.2));
			font(U"投票率").draw(20, 520, 12, ColorF(0.2, 0.2, 0.2));
			font(U"18～39歳").draw(20, 600, 12, ColorF(0.2, 0.2, 0.2));
			font(U"40歳以上").draw(20, 600, 36, ColorF(0.2, 0.2, 0.2));
			font(U"全体　　").draw(20, 600, 60, ColorF(0.2, 0.2, 0.2));
			font(U"：", FinalVote1 / 100).draw(20, 690, 12, ColorF(0.2, 0.2, 0.2)); font(U".").draw(20, 734, 12, ColorF(0.2, 0.2, 0.2)); if (FinalVote1 % 100 < 10) { font(U"0", FinalVote1 % 100, U"%").draw(20, 742, 12, ColorF(0.2, 0.2, 0.2)); }
			else { font(FinalVote1 % 100, U"%").draw(20, 742, 12, ColorF(0.2, 0.2, 0.2)); }
			font(U"：", FinalVote2 / 100).draw(20, 690, 36, ColorF(0.2, 0.2, 0.2)); font(U".").draw(20, 734, 36, ColorF(0.2, 0.2, 0.2)); if (FinalVote2 % 100 < 10) { font(U"0", FinalVote2 % 100, U"%").draw(20, 742, 36, ColorF(0.2, 0.2, 0.2)); }
			else { font(FinalVote2 % 100, U"%").draw(20, 742, 36, ColorF(0.2, 0.2, 0.2)); }
			font(U"：", FinalVote3 / 100).draw(20, 690, 60, ColorF(0.2, 0.2, 0.2)); font(U".").draw(20, 734, 60, ColorF(0.2, 0.2, 0.2)); if (FinalVote3 % 100 < 10) { font(U"0", FinalVote3 % 100, U"%").draw(20, 742, 60, ColorF(0.2, 0.2, 0.2)); }
			else { font(FinalVote3 % 100, U"%").draw(20, 742, 60, ColorF(0.2, 0.2, 0.2)); }

			// 時刻の更新
			WaitTime += Scene::DeltaTime();
			while (WaitTime >= 1.0 && Kakutei < 45) { Kakutei += 1; WaitTime -= 1.0; }

			// 開票速報（予測）
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					double ra = 0, rb = 0, rc = 0;
					if (Keisei[i][j] >= 30) { ra = 1.00; rb = 0.10; rc = 0.55; }
					else if (Keisei[i][j] >= 15) {
						ra = 0.95 + 0.05 * (Keisei[i][j] - 15) / 15.0;
						rb = 0.35 - 0.25 * (Keisei[i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (Keisei[i][j] - 15) / 15.0;
					}
					else if (Keisei[i][j] >= 1) {
						ra = 0.86 + 0.09 * (Keisei[i][j] - 0) / 15.0;
						rb = 0.80 - 0.45 * (Keisei[i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (Keisei[i][j] - 0) / 15.0;
					}
					else if (Keisei[i][j] >= 0) {
						ra = 0.87;
						rb = 0.87;
						rc = 0.87;
					}
					else if (Keisei[i][j] >= -15) {
						ra = 0.81 - 0.36 * (-Keisei[i][j] - 0) / 15.0;
						rb = 0.81 - 0.36 * (-Keisei[i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (-Keisei[i][j] - 0) / 15.0;
					}
					else if (Keisei[i][j] >= -30) {
						ra = 0.45 - 0.20 * (-Keisei[i][j] - 15) / 15.0;
						rb = 0.45 - 0.20 * (-Keisei[i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (-Keisei[i][j] - 15) / 15.0;
					}
					else { ra = 0.25; rb = 0.25; rc = 0.55; }
					Rect(60 + j * 32, 120 + i * 32, 24, 24).draw(ColorF(ra, rb, rc));
					if (Wakamono[i][j] == true) font(U"▼").draw(6, 69 + j * 32, 120 + i * 32, ColorF(1.0, 1.0, 1.0));
				}
			}
			Rect(120, 288, 160, 75).draw(ColorF(0.2, 0.2, 0.2));
			font(U"予測").draw(30, 170, 290, ColorF(1.0, 1.0, 1.0));
			font(U"有権者は１万人／区画").draw(15, 125, 330, ColorF(1.0, 1.0, 1.0));

			// 開票結果（実際）
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					double ra = 0, rb = 0, rc = 0;
					if (i * 9 + j >= Kakutei || P1[i][j] == P2[i][j]) { ra = 0.85; rb = 0.85; rc = 0.85; }
					else if (P1[i][j] > P2[i][j]) { ra = 1.00; rb = 0.10; rc = 0.55; }
					else { ra = 0.25; rb = 0.25; rc = 0.55; }
					Rect(460 + j * 32, 120 + i * 32, 24, 24).draw(ColorF(ra, rb, rc));
					if (i * 9 + j < Kakutei) {
						int wari = (int)(100.0 * P1[i][j] / (P1[i][j] + P2[i][j]) + 0.5); wari = min(99, wari);
						if (wari < 10) font(wari).draw(15, 472 + j * 32, 120 + i * 32, ColorF(1.0, 1.0, 1.0));
						else font(wari).draw(15, 463 + j * 32, 120 + i * 32, ColorF(1.0, 1.0, 1.0));
					}
				}
			}
			Rect(520, 288, 160, 75).draw(ColorF(0.2, 0.2, 0.2));
			font(U"確定").draw(30, 570, 290, ColorF(1.0, 1.0, 1.0));
			font(U"数字はあなたの得票率").draw(15, 525, 330, ColorF(1.0, 1.0, 1.0));

			// 矢印を描く
			Rect(360, 193, 60, 14).draw(ColorF(0.2, 0.2, 0.2));
			Triangle(420, 185, 420, 215, 440, 200).draw(ColorF(0.2, 0.2, 0.2));

			// 現在の票数
			int CurrentVote1 = 0, CurrentVote2 = 0;
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					if (i * 9 + j < Kakutei) { CurrentVote1 += P1[i][j]; CurrentVote2 += P2[i][j]; }
				}
			}
			DisVote1 += (CurrentVote1 - DisVote1 + 5) / 6;
			DisVote2 += (CurrentVote2 - DisVote2 + 5) / 6;

			// 票数のグラフ
			Rect(40, 510, 720.0 * DisVote1 / TotalVote, 60).draw(ColorF(0.60, 0.10, 0.30));
			Rect(760.0 - 720.0 * DisVote2 / TotalVote, 510, 720.0 * DisVote2 / TotalVote, 60).draw(ColorF(0.25, 0.25, 0.55));
			Rect(40, 510, 720, 60).drawFrame(3, ColorF(0.2, 0.2, 0.2));
			Line(400, 510, 400, 570).draw(LineStyle::SquareDot, 2, Color(20, 20, 20)); font(U"▼").draw(20, 390, 485, ColorF(0.2, 0.2, 0.2));

			// 赤の票数
			font(DisVote1).draw(50, 40, 440, ColorF(0.60, 0.10, 0.30));
			font(U"票").draw(30, 40 + 32 * to_string(DisVote1).size(), 460, ColorF(0.60, 0.10, 0.30));

			// 青の票数
			font(DisVote2).draw(50, 730 - 32 * to_string(DisVote2).size(), 440, ColorF(0.25, 0.25, 0.55));
			font(U"票").draw(30, 730, 460, ColorF(0.25, 0.25, 0.55));

			// 勝敗確定
			if (WaitTime >= 2.5) {
				int wari1 = (int)(100.0 * CurrentVote1 / (CurrentVote1 + CurrentVote2) + 0.5);
				int wari2 = 100 - wari1;

				if (CurrentVote1 >= CurrentVote2) {
					Rect(0, 0, 800, 600).draw(ColorF(0.60, 0.10, 0.30, 0.90));
					font(U"当選！").draw(80, 280, 30, ColorF(1.0, 1.0, 1.0));
					Rect(200, 400, 400, 100).draw(ColorF(1.0, 1.0, 1.0, 0.5 + 0.5 * ButtonA[9]));
					font(U"結果画面へ").draw(50, 275, 416, ColorF(0.60, 0.10, 0.30));
				}
				else {
					Rect(0, 0, 800, 600).draw(ColorF(0.25, 0.25, 0.55, 0.90));
					font(U"落選…").draw(80, 280, 30, ColorF(1.0, 1.0, 1.0));
					Rect(200, 400, 400, 100).draw(ColorF(1.0, 1.0, 1.0, 0.5 + 0.5 * ButtonA[9]));
					font(U"結果画面へ").draw(50, 275, 416, ColorF(0.25, 0.25, 0.55));
				}
				font(U"あなた").draw(30, 160, 200, ColorF(1.0, 1.0, 1.0));
				font(U"AI").draw(30, 160, 240, ColorF(1.0, 1.0, 1.0));
				font(U"：").draw(30, 240, 200, ColorF(1.0, 1.0, 1.0));
				font(U"：").draw(30, 240, 240, ColorF(1.0, 1.0, 1.0));
				font(CurrentVote1, U"票（", wari1, U"%）").draw(30, 480 - 19 * to_string(CurrentVote1).size(), 200);
				font(CurrentVote2, U"票（", wari2, U"%）").draw(30, 480 - 19 * to_string(CurrentVote1).size(), 240);
				History[14] = wari1;
			}

			// マウス判定
			int MouseState = -1;
			if (MouseX >= 200.0 && MouseX <= 600.0 && MouseY >= 400.0 && MouseY <= 500.0) MouseState = 0;
			if (MouseState == 0) ButtonA[9] = min(1.0, ButtonA[9] + 5.0 * Scene::DeltaTime());
			if (MouseState != 0) ButtonA[9] = max(0.0, ButtonA[9] - 5.0 * Scene::DeltaTime());

			// クリックの状態
			if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
				GetLastClick = Scene::Time();
				if (MouseState == 0) { WaitTime = 0; Situation = 9; }
			}
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// ------------------------------------------------- 開票終了 ------------------------------------------------------------
		// -----------------------------------------------------------------------------------------------------------------------
		if (Situation == 9) {
			font(U"結果発表").draw(60, 20, 10, ColorF(0.2, 0.2, 0.2));
			WaitTime += Scene::DeltaTime();

			// 票数のグラフ
			Rect(40, 510, 720.0 * DisVote1 / TotalVote, 60).draw(ColorF(0.60, 0.10, 0.30));
			Rect(760.0 - 720.0 * DisVote2 / TotalVote, 510, 720.0 * DisVote2 / TotalVote, 60).draw(ColorF(0.25, 0.25, 0.55));
			Rect(40, 510, 720, 60).drawFrame(3, ColorF(0.2, 0.2, 0.2));
			Line(400, 510, 400, 570).draw(LineStyle::SquareDot, 2, Color(20, 20, 20)); font(U"▼").draw(20, 390, 485, ColorF(0.2, 0.2, 0.2));
			font(DisVote1).draw(50, 40, 440, ColorF(0.60, 0.10, 0.30));
			font(U"票").draw(30, 40 + 32 * to_string(DisVote1).size(), 460, ColorF(0.60, 0.10, 0.30));
			font(DisVote2).draw(50, 730 - 32 * to_string(DisVote2).size(), 440, ColorF(0.25, 0.25, 0.55));
			font(U"票").draw(30, 730, 460, ColorF(0.25, 0.25, 0.55));

			// 形勢の変動
			for (int i = 0; i < 14; i++) {
				if (WaitTime < 0.1 * i) continue;
				double ax = 80.0 + 300.0 * (i + 0) / 14, ay = 360.0 - 220.0 * History[i + 0] / 100; ax = min(ax, 80.0 + 3000.0 * WaitTime / 14.0);
				double bx = 80.0 + 300.0 * (i + 1) / 14, by = 360.0 - 220.0 * History[i + 1] / 100; bx = min(bx, 80.0 + 3000.0 * WaitTime / 14.0);
				Quad(Vec2(ax, ay), Vec2(bx, by), Vec2(bx, 360.0), Vec2(ax, 360.0)).draw(ColorF(0.60, 0.10, 0.30, 0.70));
				Quad(Vec2(ax, ay), Vec2(bx, by), Vec2(bx, 140.0), Vec2(ax, 140.0)).draw(ColorF(0.25, 0.25, 0.55, 0.70));
			}
			Rect(80, 140, 300, 220).drawFrame(3, ColorF(0.20, 0.20, 0.20));
			Line(80, 195, 380, 195).draw(LineStyle::SquareDot, 2, ColorF(0.20, 0.20, 0.20)); font(U"75%").draw(15, 45, 185, ColorF(0.20, 0.20, 0.20));
			Line(80, 250, 380, 250).draw(LineStyle::SquareDot, 2, ColorF(0.20, 0.20, 0.20)); font(U"50%").draw(15, 45, 240, ColorF(0.20, 0.20, 0.20));
			Line(80, 305, 380, 305).draw(LineStyle::SquareDot, 2, ColorF(0.20, 0.20, 0.20)); font(U"25%").draw(15, 45, 295, ColorF(0.20, 0.20, 0.20));
			font(U"0日目").draw(15, 80, 365, ColorF(0.20, 0.20, 0.20));
			font(U"7日目").draw(15, 210, 365, ColorF(0.20, 0.20, 0.20));
			font(U"14日目").draw(15, 335, 365, ColorF(0.20, 0.20, 0.20));

			// 勝率の変動
			int Amari = min(14, (int)(3.0 * WaitTime) % 20);
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					double ra = 0, rb = 0, rc = 0;
					if (History2[Amari][i][j] >= 30) { ra = 1.00; rb = 0.10; rc = 0.55; }
					else if (History2[Amari][i][j] >= 15) {
						ra = 0.95 + 0.05 * (History2[Amari][i][j] - 15) / 15.0;
						rb = 0.35 - 0.25 * (History2[Amari][i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (History2[Amari][i][j] - 15) / 15.0;
					}
					else if (History2[Amari][i][j] >= 1) {
						ra = 0.86 + 0.09 * (History2[Amari][i][j] - 0) / 15.0;
						rb = 0.80 - 0.45 * (History2[Amari][i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (History2[Amari][i][j] - 0) / 15.0;
					}
					else if (History2[Amari][i][j] >= 0) {
						ra = 0.87;
						rb = 0.87;
						rc = 0.87;
					}
					else if (History2[Amari][i][j] >= -15) {
						ra = 0.81 - 0.36 * (-History2[Amari][i][j] - 0) / 15.0;
						rb = 0.81 - 0.36 * (-History2[Amari][i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (-History2[Amari][i][j] - 0) / 15.0;
					}
					else if (History2[Amari][i][j] >= -30) {
						ra = 0.45 - 0.20 * (-History2[Amari][i][j] - 15) / 15.0;
						rb = 0.45 - 0.20 * (-History2[Amari][i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (-History2[Amari][i][j] - 15) / 15.0;
					}
					else { ra = 0.25; rb = 0.25; rc = 0.55; }
					Rect(440 + j * 32, 210 + i * 32, 24, 24).draw(ColorF(ra, rb, rc));
				}
			}
			font(Amari).draw(30, 440, 160, ColorF(0.2, 0.2, 0.2));
			font(U"日目時点での形勢").draw(20, 440 + 20 * to_string(Amari).size(), 170, ColorF(0.2, 0.2, 0.2));

			// 終了ボタン
			Rect(630, 20, 150, 60).draw(ColorF(1.0, 0.5, 0.5, 0.5 + 0.5 * ButtonA[15]));
			font(U"ツイート").draw(30, 645, 30, ColorF(0.0, 0.0, 0.0));

			// マウス判定
			int MouseState = -1;
			if (MouseX >= 650.0 && MouseX <= 780.0 && MouseY >= 20.0 && MouseY <= 80.0) MouseState = 0;
			if (MouseState == 0) ButtonA[15] = min(1.0, ButtonA[15] + 5.0 * Scene::DeltaTime());
			if (MouseState != 0) ButtonA[15] = max(0.0, ButtonA[15] - 5.0 * Scene::DeltaTime());

			// クリックの状態
			if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
				GetLastClick = Scene::Time();
				if (MouseState == 0) {
					if (DisVote1 >= DisVote2) Twitter::OpenTweetWindow(U"難易度「{}」の選挙が行われ、{} 票対 {} 票で勝利しました！ #election_game"_fmt(Level, ThousandsSeparate(DisVote1), ThousandsSeparate(DisVote2)));
					else Twitter::OpenTweetWindow(U"難易度「{}」の選挙が行われ、{} 票対 {} 票で敗北しました… #election_game"_fmt(Level, ThousandsSeparate(DisVote1), ThousandsSeparate(DisVote2)));
				}
			}
		}
	}
}
