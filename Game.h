#pragma once

#include <Windows.h>
#include <gdiplus.h>
#include <array>
#include <vector>
#include <chrono>
#include <random>
#include "resource.h"

#pragma comment(lib,"gdiplus.lib")

#define KEY_W 0x57
#define KEY_A 0x41
#define KEY_S 0x53
#define KEY_D 0x44

namespace SpaceShooting
{

class Game
{
public:
	Game(const int, const int);
	~Game();

	void Update(HDC);
private:
	// 星構造体
	struct STAR
	{
		int x, y;						// 星の座標
	};
	std::vector<STAR> stars;			// マップ内の星達

	// 弾構造体
	struct BULLET
	{
		int x, y;						// 弾の座標
		int width, height;				// 弾の幅・高さ
		double degree;					// 発射角度
		bool isActive;					// 弾の存在フラグ
	};

	// プレイヤー構造体
	struct PLAYER
	{
		Gdiplus::Bitmap* playerImage;	// プレイヤーの画像
		int x, y;						// プレイヤーの座標
		int width, height;				// プレイヤーの幅・高さ
		int hp;							// プレイヤーのHP
		std::vector<BULLET> bullets;	// プレイヤーが発射した弾達
		Gdiplus::Bitmap* bulletImage;	// プレイヤーが発射した弾の画像
										// プレイヤーが最後に射撃した時刻
		std::chrono::high_resolution_clock::time_point lastShootTime;

		PLAYER() :
			playerImage(nullptr),
			x(275), y(650),
			width(50),height(62),
			hp(10),
			bulletImage(nullptr), 
			lastShootTime(std::chrono::high_resolution_clock::now()){ }
		~PLAYER() { delete playerImage, bulletImage; }
	};
	PLAYER player;									// プレイヤー
	std::array<Gdiplus::Bitmap*, 11> HPImage;		// HPバーの画像

	// 敵列挙体
	enum ENEMY_TYPE
	{
		TYPE_NORMAL,					// 通常型
		TYPE_SPEED,						// 自爆型
		TYPE_HEAVY,						// 火力型
		TYPE_BOSS,						// ボス
		TYPE_MAX						// 合計
	};
	// 敵状態構造体
	enum ENEMY_STATUS
	{
		STATUS_BRON,					// 出現したばかり
		STATUS_NEXT						// 出現直後からパターン変更時
	};
	// 敵構造体
	struct ENEMY
	{
		ENEMY_TYPE type;				// 敵タイプ
		int x, y;						// 敵の座標
		double degree;					// プレイヤーの中心までの回転角
		int width, height;				// 敵の幅・高さ
		int hp;							// 敵のHP
		int speed;						// 移動速度
		ENEMY_STATUS status;			// 移動・攻撃パターンを判断するためのステータス
		int nextX, nextY;				// ランダム移動する際の次の座標
										// 敵が最後に射撃した時刻
		std::chrono::high_resolution_clock::time_point lastShootTime;

		ENEMY(ENEMY_TYPE T,int X,int Y,int W,int H,int HP,int SPD) :
			type(T),
			x(X), y(Y),
			degree(0.0),
			width(W), height(H),
			hp(HP),
			speed(SPD),
			status(STATUS_BRON),
			nextX(-1), nextY(-1),
			lastShootTime(std::chrono::high_resolution_clock::now())
		{}
	};
	std::vector<ENEMY> enemys;			// 敵達
										// 敵達の画像
	std::array<Gdiplus::Bitmap*, TYPE_MAX> enemyImage;
	std::vector<BULLET> enemyBullets;	// 敵の発射した弾
	Gdiplus::Bitmap* enemyBulletImage;	// 敵が発射した弾の画像
	int bossShootCheck;					// BOSSの射撃した砲門の判別用変数

	void RespawnEnemy(void);
	double GetDegreeToPlayer(int, int, int, int);

	// ゲームフェーズ
	enum PHASE
	{
		PHASE1,
		PHASE2,
		PHASE3,
		PHASE4
	};
	PHASE currentPhase;
										// フェーズ管理用最終リスポーン時刻
	std::chrono::high_resolution_clock::time_point lastResponeTime;
	int killCount;						// 敵機撃破数
	bool bossFlg;						// ボス戦フラグ

	bool gameOverFlg;					// ゲームオーバフラグ
	bool gameClearFlg;					// ゲームクリアフラグ
	Gdiplus::Bitmap* gameOverImage;		// ゲームオーバ画像
	Gdiplus::Bitmap* gameClearImage;	// ゲームクリア画像

	const int SCREEN_WIDTH;				// 画面幅
	const int SCREEN_HEIGHT;			// 画面高さ

										// 乱数エンジン
	std::mt19937 engine{ std::random_device{ }() };

	// リソースから画像を取得する関数
	static Gdiplus::Bitmap* GetPngImageFromResource(UINT);
	// 画像を回転した際、回転後の左上・右上・左下相対座標を設定する関数
	void GetDestinationPoints(int x, int y, int width, int height, double degree, Gdiplus::Point destinationPoints[]);

	void KeyPress(void);				// キー入力受付関数
	void DrawScreen(HDC);				// 画面描画関数
};

} // End Of Namespace SpaceShooting
