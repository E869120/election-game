#include <Siv3D.hpp>
#include <ctime>
#include <cmath>
#include <tuple>
#include <vector>
#include <algorithm>
using namespace std;

struct Player {
	int speech;    // 演説力
	int influence; // 影響力
	int corporate; // 協力者
	int money;     // 資金
	int numspc;    // 演説回数
};

// 基礎情報
int Situation = 1;
double Lamp[1009];
double LastClick = 0.0;
double TimeOffset = 0.0;

// ステータス (日数・資金・形勢・形勢の履歴など)
String PlayingLevel;
Player Play[2];
int CurrentDay;
int CurrentOp;
int NextOp;
int NextAIChoice;
int RemainSpeech, CapSpeech;
int CardDrawn;
int LevelSettei;
bool AIChoiceFlag;
double LessMoney;
double Keisei[5][14];
double CurrentResultA;
double CurrentResultB;

// プレイ結果 / 最初から決まっている変数
int ResultA[5][14];
int ResultB[5][14];
int CardNum[20];
bool Populated[5][14];
bool PossibleChoice[50][5][14];
double KeiseiH[50][5][14];
double VotingRate;

// AI の動作確率
double probAI[6][14] = {
	{ 0.40, 0.05, 0.05, 0.10, 0.15, 0.15, 0.15, 0.10, 0.10, 0.72, 0.82, 0.88, 0.88, 0.88 },  // 演説
	{ 0.01, 0.01, 0.23, 0.23, 0.23, 0.50, 0.15, 0.10, 0.10, 0.15, 0.05, 0.01, 0.01, 0.01 },  // 演説を練る
	{ 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.63, 0.73, 0.73, 0.10, 0.10, 0.10, 0.10, 0.10 },  // 資金集め
	{ 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.00, 0.00, 0.00 },  // 設備購入
	{ 0.17, 0.12, 0.20, 0.50, 0.45, 0.32, 0.05, 0.05, 0.05, 0.01, 0.01, 0.00, 0.00, 0.00 },  // 協力者集め
	{ 0.40, 0.80, 0.50, 0.15, 0.15, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01 }   // SNS投稿
};

// 演説力の目安
int Meyasu_Enzetsu[14] =   {  5,   6,   6,   7,   8,   9,  10,  10,  11,  11,  12,  12,  12,  12 };
int Meyasu_Corp[14]    =   { 80,  80,  85, 100, 115, 130, 145, 155, 165, 170, 175, 175, 175, 175 };
int Meyasu_Influence[14] = {  0,   1,   3,   4,   5,   5,   6,   6,   6,   6,   6,   6,   6,   6 };

// Lamp (ボタンの色) を初期化する関数
void Reset_Lamp() {
	for (int i = 0; i < 1000; i++) Lamp[i] = 0;
}

// 0～1 の実数をランダムに生成する関数
double Randouble() {
	double s = 0, t = 1;
	for (int i = 0; i < 3; i++) {
		t /= 1024.0;
		s += 1.0 * (rand() % 1024) * t;
	}
	return s;
}

// 盤面を初期化する関数
void Init_Banmen(int Level) {
	if (Level == 0) PlayingLevel = U"EASY";
	if (Level == 1) PlayingLevel = U"NORMAL";
	if (Level == 2) PlayingLevel = U"HARD";
	if (Level == 3) PlayingLevel = U"EXTREME";

	// 変数の初期化
	CurrentDay = 0;
	CurrentOp = 0;
	CurrentResultA = 0.0;
	CurrentResultB = 0.0;
	TimeOffset = 0;
	LessMoney = 0;
	NextOp = -1;
	NextAIChoice = -1;
	VotingRate = 20.0;
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 7; j++) Keisei[i][j] = 0.0;
		for (int j = 0; j < 7; j++) Populated[i][j] = false;
	}
	for (int i = 0; i < 20; i++) CardNum[i] = 0;
	for (int i = 0; i < 50; i++) {
		for (int j = 0; j < 5; j++) {
			for (int k = 0; k < 7; k++) KeiseiH[i][j][k] = 0;
		}
	}
	for (int i = 0; i < 42; i++) {
		for (int j = 0; j < 5; j++) {
			for (int k = 0; k < 7; k++) PossibleChoice[i][j][k] = false;
		}
	}

	// 人口の多いエリアを決める
	for (int i = 0; i < 10; i++) {
		while (true) {
			int px = rand() % 5;
			int py = rand() % 7;
			if (Populated[px][py] == false) { Populated[px][py] = true; break; }
		}
	}

	// カードを決める
	for (int i = 0; i < 14; i++) {
		CardNum[i] = rand() % 12;
	}

	// 各日に演説できる場所を決める
	for (int i = 0; i < 42; i++) {
		for (int j = 0; j < 8; j++) {
			while (true) {
				int px = rand() % 5;
				int py = rand() % 7;
				if (PossibleChoice[i][px][py] == false) { PossibleChoice[i][px][py] = true; break; }
			}
		}
	}

	// 最初のステータスを設定
	Play[0].speech = 5; Play[0].influence = 0; Play[0].money = 100; Play[0].numspc = 1;
	Play[1].speech = 5; Play[1].influence = 0; Play[1].money = 100; Play[1].numspc = 1;
	if (Level == 0) { Play[0].corporate = 100; Play[1].corporate = 30; }
	if (Level == 1) { Play[0].corporate = 50; Play[1].corporate = 50; }
	if (Level == 2) { Play[0].corporate = 50; Play[1].corporate = 100; }
	if (Level == 3) { Play[0].corporate = 50; Play[1].corporate = 100; Play[1].influence = 6; }
}

// 予想得票率を計算する関数
double GetScore(int Day) {
	double Vote1 = 0;
	double Vote2 = 0;
	if (Day == -1) {
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 7; j++) {
				double wari = 1.0 / (1.0 + exp(-0.1 * Keisei[i][j]));
				double vote = 5000.0; if (Populated[i][j] == true) vote = 7000.0;
				Vote1 += vote * wari;
				Vote2 += vote;
			}
		}
	}
	if (Day != -1) {
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 7; j++) {
				double wari = 1.0 / (1.0 + exp(-0.1 * KeiseiH[Day][i][j]));
				double vote = 5000.0; if (Populated[i][j] == true) vote = 7000.0;
				Vote1 += vote * wari;
				Vote2 += vote;
			}
		}
	}
	return Vote1 / Vote2;
}

// 確率を正規化する関数
vector<double> Normalization(vector<double> ProbVec) {
	double TotalProb = 0.0;
	for (int i = 0; i < ProbVec.size(); i++) TotalProb += ProbVec[i];
	for (int i = 0; i < ProbVec.size(); i++) ProbVec[i] /= TotalProb;
	return ProbVec;
}

// AI の選択を計算する関数
int AIChoice() {
	int Day = CurrentDay / 3, NextChoice = 5;
	double Forgive = 1.00;
	if (Play[1].numspc >= 4) Forgive *= 0.2;
	if (Play[1].numspc >= 5) Forgive *= 0.2;
	if (Play[1].numspc >= 6) Forgive *= 0.2;
	if (CurrentDay == 41) {
		if (Play[1].money < 15) NextChoice = 2;
		else NextChoice = 0;
	}
	else if (Day == 13) {
		if (Play[1].money < Play[1].numspc * 15) NextChoice = 2;
		else NextChoice = 0;
	}
	else if (Play[1].money >= 300 && Randouble() < Forgive) {
		NextChoice = 3;
	}
	else {
		while (true) {
			vector<double> Probability;
			for (int i = 0; i < 6; i++) Probability.push_back(probAI[i][Day]);
			Probability[1] *= exp(0.250 * (Meyasu_Enzetsu[Day] - Play[1].speech));
			Probability[4] *= exp(0.006 * (Meyasu_Corp[Day] - Play[1].corporate));
			Probability[5] *= exp(0.250 * (Meyasu_Influence[Day] - Play[1].influence));
			if (Play[1].numspc >= 3) { Probability[3] *= 0.5; }
			if (Play[1].numspc >= 4) { Probability[3] *= 0.5; }
			if (Play[1].numspc >= 5) { Probability[3] *= 0.5; }
			Probability = Normalization(Probability);

			double ret = Randouble();
			for (int i = 0; i < 6; i++) {
				if (ret < Probability[i]) { NextChoice = i; break; }
				ret -= Probability[i];
			}
			if (NextChoice == 0 && Play[1].money < Play[1].numspc * 15) continue;
			if (NextChoice == 3 && Play[1].money < 300) continue;
			if (NextChoice == 0 && Day >= 10 && Day <= 12 && Play[1].numspc == 1) continue;
			break;
		}
	}
	return NextChoice;
}

// AI の演説場所を計算する関数
void AISpeech() {
	vector<vector<bool>> IsValid(5, vector<bool>(7, false));
	for (int i = 0; i < 8; i++) {
		while (true) {
			int px = rand() % 5;
			int py = rand() % 7;
			if (IsValid[px][py] == true) continue;
			IsValid[px][py] = true;
			break;
		}
	}

	for (int t = 0; t < Play[1].numspc; t++) {
		if (Play[1].money < 15) break;
		double min_diff = -1.0; int px = -1, py = -1;
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 7; j++) {
				if (IsValid[i][j] == false) continue;
				double prob1 = 1.0 / (1.0 + exp(0.1 * Keisei[i][j]));
				double prob2 = 1.0 / (1.0 + exp(0.1 * (Keisei[i][j] - Play[1].speech)));
				double rand_factor = 0.01 * Randouble() - 0.005;
				if (min_diff < (prob2 - prob1) + rand_factor) {
					min_diff = (prob2 - prob1) + rand_factor;
					px = i; py = j;
				}
			}
		}
		Keisei[px][py] -= Play[1].speech;
		Play[1].money -= 15;
	}
}

// 形勢を更新する関数
void UpdateKeisei() {
	vector<vector<double>> NewKeisei(5, vector<double>(7, 0.0));
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 7; j++) {
			int dx[4] = { 1, 0, -1, 0 };
			int dy[4] = { 0, 1, 0, -1 };

			double score = Keisei[i][j], maxs = Keisei[i][j], mins = Keisei[i][j];
			for (int k = 0; k < 4; k++) {
				int rx = i + dx[k], ry = j + dy[k];
				if (rx < 0 || ry < 0 || rx >= 5 || ry >= 7) continue;
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
		for (int j = 0; j < 7; j++) Keisei[i][j] = NewKeisei[i][j];
	}
}

// SNS の選挙活動を更新する関数
void ApplySNS() {
	double Sabun = 0;
	if (Play[0].influence > Play[1].influence) Sabun = +1.0 * pow(1.0 * (Play[0].influence - Play[1].influence), 0.6);
	if (Play[0].influence < Play[1].influence) Sabun = -1.0 * pow(1.0 * (Play[1].influence - Play[0].influence), 0.6);
	vector<vector<bool>> SNSUsed(5, vector<bool>(7, false));
	for (int i = 0; i < 2; i++) {
		while (true) {
			int px = rand() % 5;
			int py = rand() % 7;
			if (SNSUsed[px][py] == true) continue;
			SNSUsed[px][py] = true;
			Keisei[px][py] += Sabun;
			break;
		}
	}
	VotingRate += 1.0 * (Play[0].influence + Play[1].influence) / 12.0;
}

// 平均 0 / 標準偏差 1 の正規分布
double Norm() {
	double cnt = 0;
	for (int i = 0; i < 10000; i++) {
		if (rand() % 2 == 0) cnt += 1.0;
	}
	return (cnt - 5000.0) / 50.0;
}

// 投票結果を確定させる関数
void VoteKakutei() {
	double KeiseiError1 = 0.5 * Norm();
	double VotingError = 1.5 * Norm();
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 7; j++) {
			double KeiseiError2 = 0.8 * Norm();
			double FinalVoteRate = 0.01 * (VotingRate + VotingError);
			double Popula = 12000.0; if (Populated[i][j] == true) Popula = 16800.0;
			FinalVoteRate = min(0.9999, max(0.0001, FinalVoteRate));
			ResultA[i][j] = Popula * FinalVoteRate * (1.0 / (1.0 + exp(-(Keisei[i][j] + KeiseiError1 + KeiseiError2) / 10.0)));
			ResultB[i][j] = Popula * FinalVoteRate * (1.0 / (1.0 + exp(+(Keisei[i][j] + KeiseiError1 + KeiseiError2) / 10.0)));
		}
	}
}

// 投票結果を返す関数
pair<int, int> VoteResult(int pos) {
	int VotesA = 0;
	int VotesB = 0;
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 7; j++) {
			if (i * 7 + j >= pos) continue;
			VotesA += ResultA[i][j];
			VotesB += ResultB[i][j];
		}
	}
	return make_pair(VotesA, VotesB);
}

// 得票率から色を返す関数
tuple<double, double, double> ScoreToColor(double wari) {
	double ca, cb, cc;
	if (wari < 0.499) {
		ca = 0.20 + 0.50 * ((0.0 + wari) / 0.52);
		cb = 0.55 + 0.15 * ((0.0 + wari) / 0.52);
		cc = 0.90 - 0.15 * ((0.0 + wari) / 0.52);
	}
	else if (wari < 0.501) {
		ca = 0.70;
		cb = 0.70;
		cc = 0.75;
	}
	else {
		ca = 0.80 - 0.10 * ((1.0 - wari) / 0.52);
		cb = 0.30 + 0.40 * ((1.0 - wari) / 0.52);
		cc = 0.50 + 0.25 * ((1.0 - wari) / 0.52);
	}
	return make_tuple(ca, cb, cc);
}

// Day 日目の履歴を保存する関数
void UpdateHistory(int Day) {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 7; j++) KeiseiH[Day][i][j] = Keisei[i][j];
	}
}

void Main() {
	// 背景を黒にする
	srand((unsigned)time(NULL));
	Scene::SetBackground(ColorF(0.15, 0.20, 0.30));

	// フォントを用意
	const Font font80{ 80, Typeface::Medium };
	const Font font60{ 60, Typeface::Medium };
	const Font font50{ 50, Typeface::Medium };
	const Font font40{ 40, Typeface::Medium };
	const Font font30{ 30, Typeface::Medium };
	const Font font25{ 25, Typeface::Medium };
	const Font font20{ 20, Typeface::Medium };
	const Font font17{ 17, Typeface::Medium };
	const Font font15{ 15, Typeface::Medium };
	const Font font13{ 13, Typeface::Regular };
	const Font font6{ 6, Typeface::Regular };

	// アイコンを用意
	const Texture icon0{ 0xf05cb_icon, 50 };
	const Texture icon1{ 0xf036c_icon, 50 };
	const Texture icon2{ 0xf0116_icon, 50 };
	const Texture icon3{ 0xf020f_icon, 50 };
	const Texture icon4{ 0xf0849_icon, 50 };
	const Texture icon5{ 0xf0547_icon, 50 };

	while (System::Update()) {
		double MouseX = Cursor::PosF().x;
		double MouseY = Cursor::PosF().y;
		double Delta = Scene::DeltaTime();
		TimeOffset += Delta;
		LessMoney = max(0.0, LessMoney - Delta);

		// ################################################################################################################################
		// # 状態 1: 待ち受け画面
		// ################################################################################################################################
		if (Situation == 1) {
			font80(U"選挙で勝とう").draw(160, 90, ColorF(1.00, 1.00, 1.00));
			font40(U"2023年版").draw(312, 220, ColorF(1.00, 1.00, 1.00));
			Rect(300, 320, 200, 60).draw(ColorF(1.00, 1.00, 1.00, 0.40 + 0.60 * Lamp[0]));
			Rect(300, 400, 200, 60).draw(ColorF(1.00, 1.00, 1.00, 0.40 + 0.60 * Lamp[1]));
			font30(U"PLAY").draw(364, 332, ColorF(0.15, 0.20, 0.30));
			font30(U"RULES").draw(355, 412, ColorF(0.15, 0.20, 0.30));

			// Lamp の操作
			int Target = -1;
			if (MouseX >= 300.0 && MouseX <= 500.0 && MouseY >= 320.0 && MouseY <= 380.0) Target = 0;
			if (MouseX >= 300.0 && MouseX <= 500.0 && MouseY >= 400.0 && MouseY <= 460.0) Target = 1;
			for (int i = 0; i <= 1; i++) {
				if (Target == i) Lamp[i] = min(1.00, Lamp[i] + 3.00 * Delta);
				if (Target != i) Lamp[i] = max(0.00, Lamp[i] - 3.00 * Delta);
			}

			// Click の操作
			if (Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				LastClick = Scene::Time();
				if (Target == 0) { Reset_Lamp(); Situation = 2; }
				if (Target == 1) { Reset_Lamp(); Situation = 7; }
			}
		}

		// ################################################################################################################################
		// # 状態 2: レベル選択
		// ################################################################################################################################
		if (Situation == 2) {
			font40(U"レベルを選択してください").draw(160, 150, ColorF(1.00, 1.00, 1.00));
			Rect(80, 290, 130, 170).draw(ColorF(1.00, 1.00, 1.00, 0.40 + 0.60 * Lamp[0]));
			Rect(250, 290, 130, 170).draw(ColorF(1.00, 1.00, 1.00, 0.40 + 0.60 * Lamp[1]));
			Rect(420, 290, 130, 170).draw(ColorF(1.00, 1.00, 1.00, 0.40 + 0.60 * Lamp[2]));
			Rect(590, 290, 130, 170).draw(ColorF(1.00, 1.00, 1.00, 0.40 + 0.60 * Lamp[3]));
			font20(U"EASY").draw(119, 340, ColorF(0.15, 0.20, 0.30));
			font20(U"NORMAL").draw(273, 340, ColorF(0.15, 0.20, 0.30));
			font20(U"HARD").draw(459, 340, ColorF(0.15, 0.20, 0.30));
			font20(U"EXTREME").draw(609, 340, ColorF(0.15, 0.20, 0.30));
			font20(U"自分が現職").draw(95, 380, ColorF(0.15, 0.20, 0.30));
			font20(U"新人対決").draw(275, 380, ColorF(0.15, 0.20, 0.30));
			font20(U"相手が現職").draw(435, 380, ColorF(0.15, 0.20, 0.30));
			font20(U"相手が十選").draw(605, 380, ColorF(0.15, 0.20, 0.30));

			// Lamp の操作
			int Target = -1;
			if (MouseX >= 80.0 && MouseX <= 210.0 && MouseY >= 290.0 && MouseY <= 460.0) Target = 0;
			if (MouseX >= 250.0 && MouseX <= 380.0 && MouseY >= 290.0 && MouseY <= 460.0) Target = 1;
			if (MouseX >= 420.0 && MouseX <= 550.0 && MouseY >= 290.0 && MouseY <= 460.0) Target = 2;
			if (MouseX >= 590.0 && MouseX <= 720.0 && MouseY >= 290.0 && MouseY <= 460.0) Target = 3;
			for (int i = 0; i <= 3; i++) {
				if (Target == i) Lamp[i] = min(1.00, Lamp[i] + 3.00 * Delta);
				if (Target != i) Lamp[i] = max(0.00, Lamp[i] - 3.00 * Delta);
			}

			// Click の操作
			if (Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				LastClick = Scene::Time();
				if (Target >= 0 && Target <= 3 && TimeOffset <= 800000.0) {
					TimeOffset = 1000000.0;
					LevelSettei = Target;
				}
			}
			if (TimeOffset >= 1000000.0) {
				Rect(0, 0, 800, 600).draw(ColorF(0.15, 0.20, 0.30, TimeOffset - 1000000.0));
			}
			if (TimeOffset >= 1000001.0) {
				Reset_Lamp(); Init_Banmen(LevelSettei); TimeOffset = 0.0; Situation = 3;
			}
		}

		// ################################################################################################################################
		// # 状態 3: 選挙戦へようこそ
		// ################################################################################################################################
		if (Situation == 3) {
			font60(U"選挙戦へようこそ！").draw(40, 30, ColorF(1.00, 1.00, 1.00));
			font20(U"あなたは、とある議員選挙の立候補者です。今日から二週間かけて選挙活動").draw(60, 140, ColorF(1.00, 1.00, 1.00));
			font20(U"を行います。選挙で大切な三バン（地盤・カバン・看板）を獲得し、SNSの").draw(60, 170, ColorF(1.00, 1.00, 1.00));
			font20(U"力も活用して選挙戦を有利に進めましょう！").draw(60, 200, ColorF(1.00, 1.00, 1.00));
			font20(U"なお、このゲームの内容は、実際の選挙とは一切関係ないことに注意してく").draw(60, 250, ColorF(1.00, 1.00, 1.00));
			font20(U"ださい。実際の選挙の方が難しいです。").draw(60, 280, ColorF(1.00, 1.00, 1.00));
			Rect(200, 420, 400, 100).draw(ColorF(1.00, 1.00, 1.00, 0.3 + 0.7 * Lamp[6]));
			font40(U"はじめる").draw(320, 446, ColorF(0.15, 0.20, 0.30));

			// Lamp の操作
			int Target = -1;
			if (MouseX >= 200.0 && MouseX <= 600.0 && MouseY >= 420.0 && MouseY <= 520.0) Target = 6;
			for (int i = 0; i <= 9; i++) {
				if (Target == i) Lamp[i] = min(1.00, Lamp[i] + 3.00 * Delta);
				if (Target != i) Lamp[i] = max(0.00, Lamp[i] - 3.00 * Delta);
			}

			// Click の操作
			if (Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				LastClick = Scene::Time();
				if (Target == 6 && TimeOffset >= 0.3 && TimeOffset <= 800000.0) TimeOffset = 1000000.0;
			}
			if (TimeOffset <= 0.3) {
				Rect(0, 0, 800, 600).draw(ColorF(0.15, 0.20, 0.30, 1.0 - TimeOffset / 0.3));
			}
			if (TimeOffset >= 1000000.0) {
				Rect(0, 0, 800, 600).draw(ColorF(0.15, 0.20, 0.30, TimeOffset - 1000000.0));
			}
			if (TimeOffset >= 1000001.0) {
				Situation = 4; TimeOffset = 0.0;
			}
		}

		// ################################################################################################################################
		// # 状態 4: 選挙運動中
		// ################################################################################################################################
		if (Situation == 4) {
			Rect(0, 0, 800, 330).draw(ColorF(0.25, 0.30, 0.40));
			Rect(0, 330, 800, 270).draw(ColorF(0.15, 0.20, 0.30));

			// 形勢の描画
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 7; j++) {
					int px = 40 + 50 * j;
					int py = 40 + 50 * i;
					if (CurrentOp == 1 && PossibleChoice[CurrentDay][i][j] == true) {
						double keisuu = max(0.0, min(1.0, 1000001.0 - TimeOffset));
						Rect(px - 5, py - 5, 48, 48).draw(ColorF(1.0, 1.0, 0.0, (0.3 + 0.7 * Lamp[10 + i * 7 + j]) * keisuu));
					}
					double wari = 1.0 / (1.0 + exp(-0.1 * Keisei[i][j]));
					tuple<double, double, double> col = ScoreToColor(wari);
					Rect(px, py, 38, 38).draw(ColorF(get<0>(col), get<1>(col), get<2>(col)));
					if (Populated[i][j] == true) Triangle(px + 14, py + 5, px + 26, py + 5, px + 20, py + 13).draw(ColorF(1.00, 1.00, 1.00));
				}
			}

			// ステータスの描画
			font20(U"現在のステータス").draw(420, 35, ColorF(1.0, 1.0, 1.0));
			Rect(420, 70, 50, 90).draw(ColorF(1.00, 1.00, 1.00, 0.25));
			Rect(470, 70, 300, 30).draw(ColorF(1.00, 1.00, 1.00, 0.25));
			Line(420, 70, 770, 70).draw(2, ColorF(1.0, 1.0, 1.0));
			Line(420, 100, 770, 100).draw(2, ColorF(1.0, 1.0, 1.0));
			Line(420, 130, 770, 130).draw(2, ColorF(1.0, 1.0, 1.0));
			Line(420, 160, 770, 160).draw(2, ColorF(1.0, 1.0, 1.0));
			Line(420, 70, 420, 160).draw(2, ColorF(1.0, 1.0, 1.0));
			Line(470, 70, 470, 160).draw(2, ColorF(1.0, 1.0, 1.0));
			Line(525, 70, 525, 160).draw(2, ColorF(1.0, 1.0, 1.0));
			Line(580, 70, 580, 160).draw(2, ColorF(1.0, 1.0, 1.0));
			Line(635, 70, 635, 160).draw(2, ColorF(1.0, 1.0, 1.0));
			Line(700, 70, 700, 160).draw(2, ColorF(1.0, 1.0, 1.0));
			Line(770, 70, 770, 160).draw(2, ColorF(1.0, 1.0, 1.0));
			font20(U"資金が足りません").draw(40, 290, ColorF(0.8, 0.3, 0.5, LessMoney));
			font13(U"※形勢は赤いほど自分が有利、青いほど相手が有利。▼は人口の多い地区 (他の地区の1.4倍)").draw(227, 297, ColorF(1.0, 1.0, 1.0));

			// 表の中の描画
			font15(U"演説力").draw(475, 73, ColorF(1.0, 1.0, 1.0));
			font15(U"影響力").draw(530, 73, ColorF(1.0, 1.0, 1.0));
			font15(U"協力者").draw(585, 73, ColorF(1.0, 1.0, 1.0));
			font15(U"資金").draw(652, 73, ColorF(1.0, 1.0, 1.0));
			font15(U"演説回数").draw(705, 73, ColorF(1.0, 1.0, 1.0));
			font15(U"自分").draw(430, 103, ColorF(1.0, 1.0, 1.0));
			font15(U"AI").draw(435, 133, ColorF(1.0, 1.0, 1.0));
			for (int i = 0; i < 2; i++) {
				double ca = 0.0, cb = 0.0, cc = 0.0;
				if (i == 0) { ca = 0.86; cb = 0.51; cc = 0.65; }
				if (i == 1) { ca = 0.44; cb = 0.68; cc = 0.93; }
				font15(U"Lv").draw(520 - 10 * (2 + to_string(Play[i].speech).size()), 103 + i * 30, ColorF(ca, cb, cc));
				font15(U"Lv").draw(575 - 10 * (2 + to_string(Play[i].influence).size()), 103 + i * 30, ColorF(ca, cb, cc));
				font15(Play[i].speech).draw(520 - 10 * to_string(Play[i].speech).size(), 103 + i * 30, ColorF(ca, cb, cc));
				font15(Play[i].influence).draw(575 - 10 * to_string(Play[i].influence).size(), 103 + i * 30, ColorF(ca, cb, cc));
				font15(Play[i].corporate).draw(615 - 10 * to_string(Play[i].corporate).size(), 103 + i * 30, ColorF(ca, cb, cc));
				font15(Play[i].money).draw(680 - 10 * to_string(Play[i].money).size(), 103 + i * 30, ColorF(ca, cb, cc));
				font15(Play[i].numspc).draw(750 - 10 * to_string(Play[i].numspc).size(), 103 + i * 30, ColorF(ca, cb, cc));
				font15(U"人").draw(615, 103 + i * 30, ColorF(ca, cb, cc));
				font15(U"万").draw(680, 103 + i * 30, ColorF(ca, cb, cc));
				font15(U"回").draw(750, 103 + i * 30, ColorF(ca, cb, cc));
			}

			// 予想得票率の描画
			double CurrentKeisei = GetScore(-1);
			int CurrentKeiseiI = min(99, max(1, (int)(100.0 * CurrentKeisei + 0.5)));
			font20(U"現在の予想得票率").draw(420, 195, ColorF(1.0, 1.0, 1.0));
			Rect(420, 230, 350.0, 48).draw(ColorF(0.20, 0.55, 0.90));
			Rect(420, 230, 350.0 * CurrentKeisei, 48).draw(ColorF(0.80, 0.30, 0.50));
			if (CurrentKeiseiI < 10) {
				font30(CurrentKeiseiI).draw(430, 234, ColorF(1.00, 1.00, 1.00));
				font20(U"%").draw(450, 244, ColorF(1.00, 1.00, 1.00));
			}
			else {
				font30(CurrentKeiseiI).draw(430, 234, ColorF(1.00, 1.00, 1.00));
				font20(U"%").draw(470, 244, ColorF(1.00, 1.00, 1.00));
			}
			if (CurrentKeisei > 90) {
				font30(100 - CurrentKeiseiI).draw(725, 234, ColorF(1.00, 1.00, 1.00));
				font20(U"%").draw(745, 244, ColorF(1.00, 1.00, 1.00));
			}
			else {
				font30(100 - CurrentKeiseiI).draw(705, 234, ColorF(1.00, 1.00, 1.00));
				font20(U"%").draw(745, 244, ColorF(1.00, 1.00, 1.00));
			}
			Line(595, 230, 595, 278).draw(LineStyle::SquareDot, 2, ColorF(1.00, 1.00, 1.00));

			// ターン選択を描画
			Rect(40, 360, 270, 40).draw(ColorF(1.00, 1.00, 1.00));
			font20(U"選挙戦").draw(55, 366, ColorF(0.15, 0.20, 0.30));
			font20(CurrentDay / 3 + 1).draw(131 - 7 * to_string(CurrentDay / 3 + 1).size(), 366, ColorF(0.15, 0.20, 0.30));
			font20(U"日目").draw(147, 366, ColorF(0.15, 0.20, 0.30));
			if (CurrentDay % 3 == 0) font20(U"朝 [全14日]").draw(187, 366, ColorF(0.15, 0.20, 0.30));
			if (CurrentDay % 3 == 1) font20(U"昼 [全14日]").draw(187, 366, ColorF(0.15, 0.20, 0.30));
			if (CurrentDay % 3 == 2) font20(U"夜 [全14日]").draw(187, 366, ColorF(0.15, 0.20, 0.30));
			if (CurrentOp == 0 || CurrentOp == 1) {
				Rect(325, 360, 160, 40).draw(ColorF(0.80, 0.30, 0.50));
				font20(U"あなたのターン").draw(335, 366, ColorF(1.00, 1.00, 1.00));
			}
			if (CurrentOp == 2) {
				Rect(325, 360, 160, 40).draw(ColorF(0.20, 0.55, 0.90));
				font20(U"AIのターン").draw(350, 366, ColorF(1.00, 1.00, 1.00));
			}
			if (CurrentOp == 0 && TimeOffset <= 800000.0) {
				font20(U"クリックで行動を選択").draw(510, 366, ColorF(1.00, 1.00, 1.00, Periodic::Sine0_1(1.5s)));
			}

			// 行動選択を描画 (場面 0～2)
			if (CurrentOp == 0 || CurrentOp == 2) {
				double Zahyou[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
				for (int i = 0; i < 6; i++) {
					double sa = TimeOffset - 0.15 * i;
					if (sa < 0) Zahyou[i] = 200.0;
					else if (sa < 0.20) Zahyou[i] = 200.0 - 1000.0 * sa;
					else Zahyou[i] = 0.0;
				}
				for (int i = 0; i < 6; i++) {
					double ca = 0.55 + 0.45 * Lamp[i];
					double cb = 0.55 + 0.45 * Lamp[i];
					double cc = 0.60 - 0.60 * Lamp[i];
					Rect(40 + 123 * i, 420 + Zahyou[i], 105, 150).draw(ColorF(ca, cb, cc));
				}
				font17(U"演説をする").draw(50, 500 + Zahyou[0], ColorF(0.15, 0.20, 0.30));
				font17(U"演説の練習").draw(173, 500 + Zahyou[1], ColorF(0.15, 0.20, 0.30));
				font17(U"寄付を募る").draw(296, 500 + Zahyou[2], ColorF(0.15, 0.20, 0.30));
				font17(U"設備を購入").draw(419, 500 + Zahyou[3], ColorF(0.15, 0.20, 0.30));
				font17(U"地盤の強化").draw(542, 500 + Zahyou[4], ColorF(0.15, 0.20, 0.30));
				font17(U"SNS投稿").draw(674, 500 + Zahyou[5], ColorF(0.15, 0.20, 0.30));
				font13(U"資金 -15万").draw(60, 525 + Zahyou[0], ColorF(0.15, 0.20, 0.30));
				font13(U"形勢アップ").draw(60, 540 + Zahyou[0], ColorF(0.15, 0.20, 0.30));
				font13(U"演説力 +1").draw(186, 525 + Zahyou[1], ColorF(0.15, 0.20, 0.30));
				font13(U"資金").draw(325, 525 + Zahyou[2], ColorF(0.15, 0.20, 0.30));
				font13(U"+(1万×協力者)").draw(295, 540 + Zahyou[2], ColorF(0.15, 0.20, 0.30));
				font13(U"資金 -300万").draw(426, 525 + Zahyou[3], ColorF(0.15, 0.20, 0.30));
				font13(U"演説可能回数 +1").draw(413, 540 + Zahyou[3], ColorF(0.15, 0.20, 0.30));
				font13(U"協力者").draw(565, 525 + Zahyou[4], ColorF(0.15, 0.20, 0.30));
				font13(U"+(10+影響力)").draw(545, 540 + Zahyou[4], ColorF(0.15, 0.20, 0.30));
				font13(U"影響力 +1").draw(678, 525 + Zahyou[5], ColorF(0.15, 0.20, 0.30));
				icon0.drawAt(92, 460 + Zahyou[0], ColorF{ 0.15, 0.20, 0.30 });
				icon1.drawAt(215, 460 + Zahyou[1], ColorF{ 0.15, 0.20, 0.30 });
				icon2.drawAt(338, 460 + Zahyou[2], ColorF{ 0.15, 0.20, 0.30 });
				icon3.drawAt(461, 460 + Zahyou[3], ColorF{ 0.15, 0.20, 0.30 });
				icon4.drawAt(584, 460 + Zahyou[4], ColorF{ 0.15, 0.20, 0.30 });
				icon5.drawAt(707, 460 + Zahyou[5], ColorF{ 0.15, 0.20, 0.30 });
			}
			if (CurrentOp == 1) {
				font20(U"黄色の選択肢の中から、演説場所をクリックしてください").draw(40, 420, ColorF(1.00, 1.00, 1.00));
				font20(U"(選んだ場所の形勢が [演説力レベル] の分だけ上がります)").draw(40, 450, ColorF(1.00, 1.00, 1.00));
				font20(U"残り演説回数").draw(540, 550, ColorF(1.00, 1.00, 1.00));
				font60(RemainSpeech).draw(680, 510, ColorF(1.00, 1.00, 0.00));
				font40(U"/").draw(720, 530, ColorF(1.00, 1.00, 1.00));
				font40(CapSpeech).draw(745, 530, ColorF(1.00, 1.00, 1.00));
			}

			// 行動選択を描画 (場面 3～4)
			if (CurrentOp == 3) {
				font20(U"重要：一日の終わりには、形勢が隣に拡散します (2日目以降も同様)").draw(40, 420, ColorF(1.00, 1.00, 1.00));
				for (int t = 0; t < 2; t++) {
					for (int i = 0; i < 3; i++) {
						for (int j = 0; j < 3; j++) {
							double ca = 0.70, cb = 0.70, cc = 0.75;
							if (abs(i - 1) + abs(j - 1) == 0 && t >= 0) { ca = 0.80; cb = 0.30; cc = 0.50; }
							if (abs(i - 1) + abs(j - 1) == 1 && t >= 1) { ca = 0.73; cb = 0.58; cc = 0.68; }
							int px = 40 + i * 40 + 190 * t;
							int py = 460 + j * 40;
							Rect(px, py, 30, 30).draw(ColorF(ca, cb, cc));
						}
					}
				}
				Rect(165, 508, 30, 14).draw(ColorF(1.00, 1.00, 1.00));
				Triangle(195, 500, 195, 530, 215, 515).draw(ColorF(1.00, 1.00, 1.00));
				Rect(630, 530, 140, 40).draw(ColorF(1.00, 1.00, 1.00, 0.30 + 0.70 * Lamp[6]));
				font20(U"確認する").draw(660, 536, ColorF(0.15, 0.20, 0.30));
			}
			if (CurrentOp == 4) {
				int Sabun = Play[0].influence - Play[1].influence;
				font20(U"深夜に、SNS を通じた選挙活動が行われました").draw(40, 420, ColorF(1.00, 1.00, 1.00));
				if (Sabun >= 5) {
					font20(U"(影響力の大きい").draw(40, 450, ColorF(1.00, 1.00, 1.00));
					font20(U"あなたの形勢").draw(190, 450, ColorF(0.80, 0.30, 0.50));
					font20(U"が上がります)").draw(310, 450, ColorF(1.00, 1.00, 1.00));
				}
				else if (Sabun >= 1) {
					font20(U"(影響力の大きい").draw(40, 450, ColorF(1.00, 1.00, 1.00));
					font20(U"あなたの形勢").draw(190, 450, ColorF(0.80, 0.30, 0.50));
					font20(U"がわずかに上がります)").draw(310, 450, ColorF(1.00, 1.00, 1.00));
				}
				else if (Sabun > -1) {
					font20(U"(影響力は同じなので、形勢変動はありません)").draw(40, 450, ColorF(1.00, 1.00, 1.00));
				}
				else if (Sabun > -5) {
					font20(U"(影響力の大きい").draw(40, 450, ColorF(1.00, 1.00, 1.00));
					font20(U"AIの形勢").draw(190, 450, ColorF(0.20, 0.55, 0.90));
					font20(U"がわずかに上がります)").draw(274, 450, ColorF(1.00, 1.00, 1.00));
				}
				else {
					font20(U"(影響力の大きい").draw(40, 450, ColorF(1.00, 1.00, 1.00));
					font20(U"AIの形勢").draw(190, 450, ColorF(0.20, 0.55, 0.90));
					font20(U"が上がります)").draw(274, 450, ColorF(1.00, 1.00, 1.00));
				}
				Rect(630, 530, 140, 40).draw(ColorF(1.00, 1.00, 1.00, 0.30 + 0.70 * Lamp[7]));
				font20(U"確認する").draw(660, 536, ColorF(0.15, 0.20, 0.30));
			}

			// 行動選択を描画 (場面5)
			if (CurrentOp == 5) {
				if (CardDrawn == 0) {
					font20(U"選挙戦").draw(40, 420, ColorF(1.00, 1.00, 1.00));
					font20(CurrentDay / 3 + 1).draw(100, 420, ColorF(1.00, 1.00, 1.00));
					if (CurrentDay <= 26) font20(U"日目が始まりました").draw(112, 420, ColorF(1.00, 1.00, 1.00));
					if (CurrentDay >= 27) font20(U"日目が始まりました").draw(124, 420, ColorF(1.00, 1.00, 1.00));
					font20(U"チャンスカードを引いてください").draw(40, 450, ColorF(1.00, 1.00, 1.00));
					Rect(630, 530, 140, 40).draw(ColorF(1.00, 1.00, 1.00, 0.30 + 0.70 * Lamp[8]));
					font20(U"引く").draw(680, 536, ColorF(0.15, 0.20, 0.30));
				}

				// カードが既に引かれた場合
				if (CardDrawn >= 1) {
					Rect(40, 420, 270, 150).draw(ColorF(1.00, 1.00, 0.00, 1.00));
					if (CardNum[CurrentDay / 3] == 0) {
						font20(U"選挙への関心").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"投票率が 10% 上がる").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; VotingRate += 10.0; }
					}
					if (CardNum[CurrentDay / 3] == 1) {
						font20(U"物価の上昇").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"両候補の資金が 20 万円減少する").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Play[0].money = max(0, Play[0].money - 20); Play[1].money = max(0, Play[1].money - 20); }
					}
					if (CardNum[CurrentDay / 3] == 2) {
						font20(U"テレビ出演").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"影響力レベルが 1 上昇する").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Play[0].influence += 1; }
					}
					if (CardNum[CurrentDay / 3] == 3) {
						font20(U"対立候補が新聞掲載").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"相手の影響力レベルが 1 上昇する").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Play[1].influence += 1; }
					}
					if (CardNum[CurrentDay / 3] == 4) {
						font20(U"人柄が評価される").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"1 つの地区の形勢が 15 上昇する").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Keisei[rand() % 5][rand() % 7] += 15; }
					}
					if (CardNum[CurrentDay / 3] == 5) {
						font20(U"悪いうわさが出回る").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"1 つの地区の形勢が 15 下落する").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Keisei[rand() % 5][rand() % 7] -= 15; }
					}
					if (CardNum[CurrentDay / 3] == 6) {
						font20(U"推薦状が得られた").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"協力者が 15 人増える").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Play[0].corporate += 15; }
					}
					if (CardNum[CurrentDay / 3] == 7) {
						font20(U"相手が後援会を作った").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"相手の協力者が 15 人増える").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Play[1].corporate += 15; }
					}
					if (CardNum[CurrentDay / 3] == 8) {
						font20(U"宝くじで当選").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"資金が 30 万円増える").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Play[0].money += 30; }
					}
					if (CardNum[CurrentDay / 3] == 9) {
						font20(U"選挙カーが事故に遭う").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"資金が 30 万円減る").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Play[0].money = max(0, Play[0].money - 30); }
					}
					if (CardNum[CurrentDay / 3] == 10) {
						font20(U"話し方を身に付けた").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"演説力レベルが 1 上昇する").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Play[0].speech += 1; }
					}
					if (CardNum[CurrentDay / 3] == 11) {
						font20(U"相手がセミナーに参加").draw(55, 435, ColorF(0.15, 0.20, 0.30));
						font15(U"相手の演説力レベルが 1 上昇する").draw(55, 465, ColorF(0.15, 0.20, 0.30));
						if (CardDrawn == 1) { CardDrawn = 2; Play[1].speech += 1; }
					}
					Rect(630, 530, 140, 40).draw(ColorF(1.00, 1.00, 1.00, 0.30 + 0.70 * Lamp[8]));
					font20(U"OK").draw(688, 536, ColorF(0.15, 0.20, 0.30));
				}
			}

			// 行動選択を描画 (場面6)
			if (CurrentOp == 6) {
				font20(U"いよいよ投票日です！").draw(40, 420, ColorF(1.00, 1.00, 1.00));
				Rect(630, 530, 140, 40).draw(ColorF(1.00, 1.00, 1.00, 0.30 + 0.70 * Lamp[9]));
				font20(U"開票へ").draw(670, 536, ColorF(0.15, 0.20, 0.30));
			}

			// Lamp の操作 + AI の行動処理
			int Target = -1;
			if (CurrentOp == 0) {
				if (MouseX >= 40.0 && MouseX <= 145.0 && MouseY >= 420.0 && MouseY <= 570.0) Target = 0;
				if (MouseX >= 163.0 && MouseX <= 268.0 && MouseY >= 420.0 && MouseY <= 570.0) Target = 1;
				if (MouseX >= 286.0 && MouseX <= 391.0 && MouseY >= 420.0 && MouseY <= 570.0) Target = 2;
				if (MouseX >= 409.0 && MouseX <= 514.0 && MouseY >= 420.0 && MouseY <= 570.0) Target = 3;
				if (MouseX >= 532.0 && MouseX <= 637.0 && MouseY >= 420.0 && MouseY <= 570.0) Target = 4;
				if (MouseX >= 655.0 && MouseX <= 760.0 && MouseY >= 420.0 && MouseY <= 570.0) Target = 5;
			}
			if (CurrentOp == 1) {
				if (MouseX >= 35.0 && MouseX <= 385.0 && MouseY >= 35.0 && MouseY <= 285.0) {
					int py = max(0, min(6, (int)((MouseX - 35.0) / 50.0)));
					int px = max(0, min(4, (int)((MouseY - 35.0) / 50.0)));
					Target = 10 + px * 7 + py;
				}
			}
			if (CurrentOp == 2) {
				if (TimeOffset >= 0.95 && TimeOffset <= 1.95) { Lamp[NextAIChoice] = 1.00; Target = NextAIChoice; }
				if (TimeOffset >= 0.95 && AIChoiceFlag == false) {
					if (NextAIChoice == 0) { AISpeech(); }
					if (NextAIChoice == 1) { Play[1].speech += 1; }
					if (NextAIChoice == 2) { Play[1].money += Play[1].corporate; }
					if (NextAIChoice == 3) { Play[1].money -= 300; Play[1].numspc += 1; }
					if (NextAIChoice == 4) { Play[1].corporate += 10 + Play[1].influence; }
					if (NextAIChoice == 5) { Play[1].influence += 1; }
					if (CurrentDay % 3 != 2) { NextOp = 0; }
					if (CurrentDay % 3 == 2) { NextOp = 3; if (CurrentDay >= 5) NextOp = 4; if (CurrentDay == 41) NextOp = 6; }
					AIChoiceFlag = true;
				}
			}
			if (CurrentOp >= 3 && CurrentOp <= 6) {
				if (MouseX >= 630.0 && MouseX <= 770.0 && MouseY >= 530.0 && MouseY <= 570.0) {
					Target = 6 + (CurrentOp - 3);
				}
			}
			for (int i = 0; i <= 99; i++) {
				if (Target == i) Lamp[i] = min(1.00, Lamp[i] + 3.00 * Delta);
				if (Target != i) Lamp[i] = max(0.00, Lamp[i] - 3.00 * Delta);
			}

			// Click の操作 (Op = 0: 自分の行動の選択)
			if (CurrentOp == 0 && Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				LastClick = Scene::Time();
				if (Target >= 0 && Target <= 5 && TimeOffset >= 0.95 && TimeOffset <= 800000.0) {
					if (Target == 0) {
						if (Play[0].money < 15) { LessMoney = 1.0; }
						else { NextOp = 1; TimeOffset = 1000000.0; CapSpeech = min(Play[0].numspc, Play[0].money / 15); RemainSpeech = CapSpeech; }
					}
					if (Target == 1) { NextOp = 2; TimeOffset = 1000000.0; Play[0].speech += 1; }
					if (Target == 2) { NextOp = 2; TimeOffset = 1000000.0; Play[0].money += Play[0].corporate; }
					if (Target == 3) {
						if (Play[0].money < 300) { LessMoney = 1.0; }
						else { NextOp = 2; TimeOffset = 1000000.0; Play[0].money -= 300; Play[0].numspc += 1; }
					}
					if (Target == 4) { NextOp = 2; TimeOffset = 1000000.0; Play[0].corporate += 10 + Play[0].influence; }
					if (Target == 5) { NextOp = 2; TimeOffset = 1000000.0; Play[0].influence += 1; }
				}
			}

			// Click の操作 (Op = 1: 演説場所の選択)
			if (CurrentOp == 1 && Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				LastClick = Scene::Time();
				if (Target >= 10 && Target <= 44 && TimeOffset >= 0.15 && TimeOffset <= 800000.0) {
					int px = (Target - 10) / 7;
					int py = (Target - 10) % 7;
					if (PossibleChoice[CurrentDay][px][py] == true) {
						Keisei[px][py] += Play[0].speech;
						Play[0].money -= 15;
						RemainSpeech -= 1;
						if (RemainSpeech == 0) { TimeOffset = 1000000.0; NextOp = 2; }
					}
				}
			}

			// Click の操作 (Op = 3: 拡散の説明)
			if (CurrentOp == 3 && Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				if (Target == 6 && TimeOffset <= 800000.0) {
					UpdateKeisei(); TimeOffset = 1000000.0; NextOp = 4;
				}
			}

			// Click の操作 (Op = 4: SNS活動の説明)
			if (CurrentOp == 4 && Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				if (Target == 7 && TimeOffset <= 800000.0) {
					ApplySNS(); TimeOffset = 1000000.0; NextOp = 5;
				}
			}

			// Click の操作 (Op = 5: カードを引く)
			if (CurrentOp == 5 && Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				LastClick = Scene::Time();
				if (Target == 8 && TimeOffset <= 800000.0) {
					if (CardDrawn == 0) { CardDrawn = 1; }
					else { TimeOffset = 1000000.0; NextOp = 0; }
				}
			}

			// Click の操作 (Op = 6: 開票へ進む)
			if (CurrentOp == 6 && Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				LastClick = Scene::Time();
				if (Target == 9 && TimeOffset <= 800000.0) { TimeOffset = 1000000.0; NextOp = 0; }
			}

			// 画面の切り替え
			if (CurrentOp == 0 && NextOp != -1) {
				if (TimeOffset >= 1000000.0) { Rect(0, 410, 800, 190).draw(ColorF(0.15, 0.20, 0.30, TimeOffset - 1000000.0)); }
				if (TimeOffset >= 1000001.0) {
					TimeOffset = 0; CurrentOp = NextOp; NextOp = -1;
					if (CurrentOp == 2) { NextAIChoice = AIChoice(); AIChoiceFlag = false; }
				}
			}
			else if (CurrentOp == 1 && NextOp != -1) {
				if (TimeOffset >= 1000000.0) { Rect(0, 410, 800, 190).draw(ColorF(0.15, 0.20, 0.30, TimeOffset - 1000000.0)); }
				if (TimeOffset >= 1000001.0) {
					TimeOffset = 0; CurrentOp = NextOp; NextOp = -1;
					if (CurrentOp == 2) { NextAIChoice = AIChoice(); AIChoiceFlag = false; }
				}
			}
			else if (CurrentOp == 2 && NextOp != -1) {
				if (TimeOffset >= 2.0) { Rect(0, 410, 800, 190).draw(ColorF(0.15, 0.20, 0.30, TimeOffset - 2.0)); }
				if (TimeOffset >= 3.0) {
					TimeOffset = 0; CurrentOp = NextOp; NextOp = -1;
				}
				if (CurrentOp == 4 || CurrentOp == 6) UpdateKeisei();
				if (CurrentOp == 0) { CurrentDay += 1; UpdateHistory(CurrentDay); }
			}
			else if ((CurrentOp == 3 || CurrentOp == 4) && NextOp != -1) {
				if (TimeOffset >= 1000000.0) { Rect(0, 410, 800, 190).draw(ColorF(0.15, 0.20, 0.30, TimeOffset - 1000000.0)); }
				if (TimeOffset >= 1000001.0) {
					TimeOffset = 0; CurrentOp = NextOp; NextOp = -1;
				}
				if (CurrentOp == 5) { CurrentDay += 1; UpdateHistory(CurrentDay); CardDrawn = 0; }
			}
			else if (CurrentOp == 5 && NextOp != -1) {
				if (TimeOffset >= 1000000.0) { Rect(0, 410, 800, 190).draw(ColorF(0.15, 0.20, 0.30, TimeOffset - 1000000.0)); }
				if (TimeOffset >= 1000001.0) {
					TimeOffset = 0; CurrentOp = NextOp; NextOp = -1;
				}
			}
			else if (CurrentOp == 6 && NextOp != -1) {
				if (TimeOffset >= 1000000.0) { Rect(0, 0, 800, 600).draw(ColorF(0.15, 0.20, 0.30, TimeOffset - 1000000.0)); }
				if (TimeOffset >= 1000001.0) {
					TimeOffset = 0; CurrentOp = NextOp; NextOp = -1; Situation = 5;
					VotingRate += 20.0 * exp(-1.0 * abs(CurrentKeiseiI - 50) / 6.0); VoteKakutei();
					CurrentDay += 1; UpdateHistory(CurrentDay);
				}
			}
		}


		// ################################################################################################################################
		// # 状態 5: 開票作業中
		// ################################################################################################################################
		if (Situation == 5) {
			int FinalRes1 = VoteResult(35).first;
			int FinalRes2 = VoteResult(35).second;
			int ActualVoteRate = (int)(10000.0 * (FinalRes1 + FinalRes2) / 444000.0);
			int Bango = min(35, (int)TimeOffset);

			// 上の部分の描画
			font50(U"開票速報").draw(30, 23, ColorF(1.00, 1.00, 1.00));
			font30(U"投票率").draw(550, 25, ColorF(1.00, 1.00, 1.00));
			font30(ActualVoteRate / 100).draw(659, 25, ColorF(1.00, 1.00, 1.00));
			font30(U".").draw(699, 25, ColorF(1.00, 1.00, 1.00));
			font30((ActualVoteRate / 10) % 10).draw(710, 25, ColorF(1.00, 1.00, 1.00));
			font30((ActualVoteRate / 1) % 10).draw(730, 25, ColorF(1.00, 1.00, 1.00));
			font30(U"%").draw(750, 25, ColorF(1.00, 1.00, 1.00));

			// 中の部分の描画
			Rect(30, 110, 330, 280).draw(ColorF(1.00, 1.00, 1.00, 0.20));
			Rect(440, 110, 330, 280).draw(ColorF(1.00, 1.00, 1.00, 0.20));
			font25(U"最終日時点").draw(45, 120, ColorF(1.00, 1.00, 1.00));
			font25(U"結果 (数字は得票率)").draw(455, 120, ColorF(1.00, 1.00, 1.00));
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 7; j++) {
					int px = 55 + 41 * j;
					int py = 170 + 41 * i;
					double wari = 1.0 / (1.0 + exp(-0.1 * Keisei[i][j]));
					tuple<double, double, double> col = ScoreToColor(wari);
					Rect(px, py, 34, 34).draw(ColorF(get<0>(col), get<1>(col), get<2>(col)));
					if (Populated[i][j] == true) Triangle(px + 12, py + 3, px + 22, py + 3, px + 17, py + 10).draw(ColorF(1.00, 1.00, 1.00));
				}
			}
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 7; j++) {
					int px = 465 + 41 * j;
					int py = 170 + 41 * i;
					double wari = 1.0 * ResultA[i][j] / (ResultA[i][j] + ResultB[i][j]);
					int Intwari = min(99, max(1, (int)(100.0 * wari + 0.5)));
					tuple<double, double, double> col = ScoreToColor(wari);
					if (i * 7 + j < Bango) {
						Rect(px, py, 34, 34).draw(ColorF(get<0>(col), get<1>(col), get<2>(col)));
						if (Populated[i][j] == true) Triangle(px + 12, py + 3, px + 22, py + 3, px + 17, py + 10).draw(ColorF(1.00, 1.00, 1.00));
						if (Intwari >= 10) font20(Intwari / 10).draw(px + 4, py + 8, ColorF(1.00, 1.00, 1.00));
						if (Intwari >= 0) font20(Intwari % 10).draw(px + 17, py + 8, ColorF(1.00, 1.00, 1.00));
					}
					else {
						Rect(px, py, 34, 34).draw(ColorF(0.45, 0.47, 0.54));
						if (Populated[i][j] == true) Triangle(px + 12, py + 3, px + 22, py + 3, px + 17, py + 10).draw(ColorF(1.00, 1.00, 1.00));
						font20(U"??").draw(px + 4, py + 8, ColorF(0.30, 0.33, 0.42));
					}
				}
			}

			// 下の部分の描画
			int pows[6] = { 100000, 10000, 1000, 100, 10, 1 };
			int px1 = 740.0 * CurrentResultA / (FinalRes1 + FinalRes2);
			int px2 = 740.0 * (FinalRes1 + FinalRes2 - CurrentResultB) / (FinalRes1 + FinalRes2);
			CurrentResultA += (VoteResult(Bango).first - CurrentResultA) * min(1.00, Delta * 12.0);
			CurrentResultB += (VoteResult(Bango).second - CurrentResultB) * min(1.00, Delta * 12.0);
			for (int i = 0; i < 6; i++) {
				int res = (int)(CurrentResultA + 0.5); if (res < 10) res = 0;
				int keta = (res / pows[i]) % 10;
				if (res < pows[i] && i <= 4) font60(keta).draw(30 + i * 38, 432, ColorF(0.28, 0.22, 0.30));
				else font60(keta).draw(30 + i * 38, 432, ColorF(0.80, 0.30, 0.50));
			}
			for (int i = 0; i < 6; i++) {
				int res = (int)(CurrentResultB + 0.5); if (res < 10) res = 0;
				int keta = (res / pows[i]) % 10;
				if (res < pows[i] && i <= 4) font60(keta).draw(512 + i * 38, 432, ColorF(0.16, 0.25, 0.40));
				else font60(keta).draw(512 + i * 38, 432, ColorF(0.20, 0.55, 0.90));
			}
			font30(U"票").draw(258, 462, ColorF(0.80, 0.30, 0.50));
			font30(U"票").draw(740, 462, ColorF(0.20, 0.55, 0.90));
			font20(U"あなた").draw(30, 415, ColorF(0.80, 0.30, 0.50));
			font20(U"AI").draw(747, 415, ColorF(0.20, 0.55, 0.90));
			Rect(30, 510, 740, 60).draw(ColorF(0.45, 0.47, 0.54));
			Rect(30, 510, px1, 60).draw(ColorF(0.80, 0.30, 0.50));
			Rect(30 + px2, 510, 740 - px2, 60).draw(ColorF(0.20, 0.55, 0.90));
			Line(400, 510, 400, 570).draw(LineStyle::SquareDot, 3, ColorF(1.00, 1.00, 1.00));

			// 結果の表示
			if (TimeOffset >= 38.0) {
				if (FinalRes1 >= FinalRes2) {
					Rect(0, 0, 800, 600).draw(ColorF(0.80, 0.30, 0.50));
					font80(U"当選！").draw(280, 100, ColorF(1.00, 1.00, 1.00));
					Rect(250, 420, 300, 60).draw(ColorF(1.00, 1.00, 1.00, 0.30 * Lamp[0] + 0.70));
					font30(U"結果発表へ").draw(325, 431, ColorF(0.80, 0.30, 0.50));
				}
				else {
					Rect(0, 0, 800, 600).draw(ColorF(0.20, 0.35, 0.50));
					font80(U"落選…").draw(280, 100, ColorF(1.00, 1.00, 1.00));
					Rect(250, 420, 300, 60).draw(ColorF(1.00, 1.00, 1.00, 0.30 * Lamp[0] + 0.70));
					font30(U"結果発表へ").draw(325, 431, ColorF(0.20, 0.35, 0.50));
				}

				// 票数表示
				font30(U"あなた：").draw(190, 230, ColorF(1.00, 1.00, 1.00));
				font30(U"AI：").draw(250, 270, ColorF(1.00, 1.00, 1.00));
				for (int i = 0; i < 6; i++) {
					int keta = (FinalRes1 / pows[i]) % 10;
					if (FinalRes1 >= pows[i]) font30(keta).draw(310 + 19 * i, 230, ColorF(1.00, 1.00, 1.00));
				}
				for (int i = 0; i < 6; i++) {
					int keta = (FinalRes2 / pows[i]) % 10;
					if (FinalRes2 >= pows[i]) font30(keta).draw(310 + 19 * i, 270, ColorF(1.00, 1.00, 1.00));
				}
				font30(U"票").draw(424, 230, ColorF(1.00, 1.00, 1.00));
				font30(U"票").draw(424, 270, ColorF(1.00, 1.00, 1.00));

				// 得票率表示
				int Rate1 = (10000.0 * FinalRes1 / (FinalRes1 + FinalRes2) + 0.5);
				int Rate2 = 10000 - Rate1;
				font30(U"(").draw(467, 230, ColorF(1.00, 1.00, 1.00));
				font30(U"(").draw(467, 270, ColorF(1.00, 1.00, 1.00));
				font30(U".").draw(519, 230, ColorF(1.00, 1.00, 1.00));
				font30(U".").draw(519, 270, ColorF(1.00, 1.00, 1.00));
				font30(U"%)").draw(568, 230, ColorF(1.00, 1.00, 1.00));
				font30(U"%)").draw(568, 270, ColorF(1.00, 1.00, 1.00));
				int place[4] = { 481, 500, 530, 549 };
				for (int i = 0; i < 4; i++) {
					int keta = (Rate1 / pows[2 + i]) % 10;
					if (Rate1 >= pows[2 + i]) font30(keta).draw(place[i], 230, ColorF(1.00, 1.00, 1.00));
				}
				for (int i = 0; i < 4; i++) {
					int keta = (Rate2 / pows[2 + i]) % 10;
					if (Rate2 >= pows[2 + i]) font30(keta).draw(place[i], 270, ColorF(1.00, 1.00, 1.00));
				}

				// Lamp の操作
				int Target = -1;
				if (MouseX >= 250.0 && MouseX <= 550.0 && MouseY >= 420.0 && MouseY <= 480.0) Target = 0;
				for (int i = 0; i <= 3; i++) {
					if (Target == i) Lamp[i] = min(1.00, Lamp[i] + 3.00 * Delta);
					if (Target != i) Lamp[i] = max(0.00, Lamp[i] - 3.00 * Delta);
				}

				// Click の操作
				if (Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
					LastClick = Scene::Time();
					if (Target == 0) { TimeOffset = 0.0; Reset_Lamp(); Situation = 6; }
				}
			}
		}

		// ################################################################################################################################
		// # 状態 6: 結果発表
		// ################################################################################################################################
		if (Situation == 6) {
			int FinalRes1 = VoteResult(35).first;
			int FinalRes2 = VoteResult(35).second;

			// 上の画面の描画
			font50(U"結果発表").draw(30, 23, ColorF(1.00, 1.00, 1.00));
			Rect(30, 110, 350, 280).draw(ColorF(1.00, 1.00, 1.00, 0.20));
			Rect(420, 110, 350, 280).draw(ColorF(1.00, 1.00, 1.00, 0.20));

			// 形勢グラフの描画
			font25(U"形勢グラフ").draw(45, 120, ColorF(1.00, 1.00, 1.00));
			font15(U"70%").draw(60, 165, ColorF(1.00, 1.00, 1.00));
			font15(U"50%").draw(60, 245, ColorF(1.00, 1.00, 1.00));
			font15(U"30%").draw(60, 325, ColorF(1.00, 1.00, 1.00));
			font15(U"3日目").draw(134, 348, ColorF(1.00, 1.00, 1.00));
			font15(U"7日目").draw(205, 348, ColorF(1.00, 1.00, 1.00));
			font15(U"11日目").draw(273, 348, ColorF(1.00, 1.00, 1.00));
			Line(100, 175, 352, 175).draw(LineStyle::SquareDot, 2, ColorF(1.00, 1.00, 1.00));
			Line(100, 255, 352, 255).draw(LineStyle::SquareDot, 2, ColorF(1.00, 1.00, 1.00));
			Line(100, 335, 352, 335).draw(LineStyle::SquareDot, 2, ColorF(1.00, 1.00, 1.00));
			Line(154, 340, 154, 348).draw(LineStyle::SquareDot, 2, ColorF(1.00, 1.00, 1.00));
			Line(226, 340, 226, 348).draw(LineStyle::SquareDot, 2, ColorF(1.00, 1.00, 1.00));
			Line(298, 340, 298, 348).draw(LineStyle::SquareDot, 2, ColorF(1.00, 1.00, 1.00));
			for (int i = 0; i < 42; i++) {
				double zahyou1 = min(335.0, max(175.0, 255.0 - 80.0 * (GetScore(i + 0) - 0.5) / 0.2));
				double zahyou2 = min(335.0, max(175.0, 255.0 - 80.0 * (GetScore(i + 1) - 0.5) / 0.2));
				int px1 = 100 + 6 * (i + 0);
				int px2 = 100 + 6 * (i + 1);
				Line(px1, zahyou1, px2, zahyou2).draw(2, ColorF(1.00, 1.00, 0.00));
			}
			if (TimeOffset < 1.20) Rect(100 + 210.0 * TimeOffset, 170, 260 - 210.0 * TimeOffset, 170).draw(ColorF(0.15, 0.20, 0.30));
			Rect(100, 170, 252, 170).drawFrame(2, ColorF(1.0, 1.0, 1.0));

			// 形勢変動の描画
			int DisplayDay = min(13, (int)(2.0 * TimeOffset) % 18) + 1;
			if (DisplayDay <= 9) font25(DisplayDay / 10).draw(435, 120, ColorF(0.45, 0.47, 0.54));
			else font25(DisplayDay / 10).draw(435, 120, ColorF(1.00, 1.00, 1.00));
			font25(DisplayDay % 10).draw(451, 120, ColorF(1.00, 1.00, 1.00));
			font25(U"日目時点での形勢").draw(467, 120, ColorF(1.00, 1.00, 1.00));
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 7; j++) {
					int px = 455 + 41 * j;
					int py = 170 + 41 * i;
					double wari = 1.0 / (1.0 + exp(-0.1 * KeiseiH[DisplayDay * 3][i][j]));
					tuple<double, double, double> col = ScoreToColor(wari);
					Rect(px, py, 34, 34).draw(ColorF(get<0>(col), get<1>(col), get<2>(col)));
					if (Populated[i][j] == true) Triangle(px + 12, py + 3, px + 22, py + 3, px + 17, py + 10).draw(ColorF(1.00, 1.00, 1.00));
				}
			}

			// 下の部分の描画
			int pows[6] = { 100000, 10000, 1000, 100, 10, 1 };
			int zahyou = 740.0 * FinalRes1 / (FinalRes1 + FinalRes2);
			for (int i = 0; i < 6; i++) {
				int keta = (FinalRes1 / pows[i]) % 10;
				if (FinalRes1 < pows[i] && i <= 4) font60(keta).draw(30 + i * 38, 432, ColorF(0.28, 0.22, 0.30));
				else font60(keta).draw(30 + i * 38, 432, ColorF(0.80, 0.30, 0.50));
			}
			for (int i = 0; i < 6; i++) {
				int keta = (FinalRes2 / pows[i]) % 10;
				if (FinalRes2 < pows[i] && i <= 4) font60(keta).draw(512 + i * 38, 432, ColorF(0.16, 0.25, 0.40));
				else font60(keta).draw(512 + i * 38, 432, ColorF(0.20, 0.55, 0.90));
			}
			font30(U"票").draw(258, 462, ColorF(0.80, 0.30, 0.50));
			font30(U"票").draw(740, 462, ColorF(0.20, 0.55, 0.90));
			font20(U"あなた").draw(30, 415, ColorF(0.80, 0.30, 0.50));
			font20(U"AI").draw(747, 415, ColorF(0.20, 0.55, 0.90));
			Rect(30, 510, 740, 60).draw(ColorF(0.20, 0.55, 0.90));
			Rect(30, 510, zahyou, 60).draw(ColorF(0.80, 0.30, 0.50));
			Line(400, 510, 400, 570).draw(LineStyle::SquareDot, 3, ColorF(1.00, 1.00, 1.00));

			// ツイートボタン
			Rect(570, 25, 200, 60).draw(ColorF(0.80, 0.30, 0.50, 0.30 + 0.70 * Lamp[1]));
			font30(U"ツイート").draw(610, 36, ColorF(1.00, 1.00, 1.00));

			// Lamp の操作
			int Target = -1;
			if (MouseX >= 570.0 && MouseX <= 770.0 && MouseY >= 25.0 && MouseY <= 85.0) Target = 1;
			for (int i = 0; i <= 3; i++) {
				if (Target == i) Lamp[i] = min(1.00, Lamp[i] + 3.00 * Delta);
				if (Target != i) Lamp[i] = max(0.00, Lamp[i] - 3.00 * Delta);
			}

			// Click の操作
			if (Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				LastClick = Scene::Time();
				if (Target == 1) {
					if (FinalRes1 >= FinalRes2) {
						Twitter::OpenTweetWindow(U"難易度「{}」の選挙が行われ、{} 票対 {} 票で勝利しました！ #election_game \nhttps://e869120.github.io/election-game/index.html"_fmt(
							PlayingLevel, ThousandsSeparate(FinalRes1), ThousandsSeparate(FinalRes2)));
					}
					else {
						Twitter::OpenTweetWindow(U"難易度「{}」の選挙が行われ、{} 票対 {} 票で敗北しました… #election_game \nhttps://e869120.github.io/election-game/index.html"_fmt(
							PlayingLevel, ThousandsSeparate(FinalRes1), ThousandsSeparate(FinalRes2)));
					}
				}
			}
		}

		// ################################################################################################################################
		// # 状態 7: ルール説明
		// ################################################################################################################################
		if (Situation == 7) {
			font60(U"このゲームについて").draw(40, 30, ColorF(1.00, 1.00, 1.00));
			font20(U"このゲームは、立候補者のつもりになって選挙戦を戦うゲームです。選挙で").draw(60, 140, ColorF(1.00, 1.00, 1.00));
			font20(U"選挙で勝つには演説をすることも重要ですが、資金・協力者・SNSでの影響").draw(60, 170, ColorF(1.00, 1.00, 1.00));
			font20(U"力などの要素も大切です。戦略的に選挙戦を進めていきましょう！").draw(60, 200, ColorF(1.00, 1.00, 1.00));
			font20(U"なお、ゲームの大まかなルールは遊んでいるうちにわかると思いますが、詳").draw(60, 250, ColorF(1.00, 1.00, 1.00));
			font20(U"しいルールを知りたい方は以下を参照してください。").draw(60, 280, ColorF(1.00, 1.00, 1.00));
			font20(U"　・https://github.com/E869120/election-game").draw(60, 320, ColorF(1.00, 1.00, 1.00));
			Rect(200, 420, 400, 100).draw(ColorF(1.00, 1.00, 1.00, 0.3 + 0.7 * Lamp[6]));
			font40(U"もどる").draw(340, 446, ColorF(0.15, 0.20, 0.30));

			// Lamp の操作
			int Target = -1;
			if (MouseX >= 200.0 && MouseX <= 600.0 && MouseY >= 420.0 && MouseY <= 520.0) Target = 6;
			for (int i = 0; i <= 9; i++) {
				if (Target == i) Lamp[i] = min(1.00, Lamp[i] + 3.00 * Delta);
				if (Target != i) Lamp[i] = max(0.00, Lamp[i] - 3.00 * Delta);
			}

			// Click の操作
			if (Scene::Time() - LastClick >= 0.1 && MouseL.down()) {
				LastClick = Scene::Time();
				if (Target == 6 && TimeOffset >= 0.3 && TimeOffset <= 800000.0) TimeOffset = 1000000.0;
			}
			if (TimeOffset >= 1000000.0) {
				Situation = 1; TimeOffset = 0.0;
			}
		}
	}
}
