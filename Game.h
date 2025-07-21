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
	// ���\����
	struct STAR
	{
		int x, y;						// ���̍��W
	};
	std::vector<STAR> stars;			// �}�b�v���̐��B

	// �e�\����
	struct BULLET
	{
		int x, y;						// �e�̍��W
		int width, height;				// �e�̕��E����
		double degree;					// ���ˊp�x
		bool isActive;					// �e�̑��݃t���O
	};

	// �v���C���[�\����
	struct PLAYER
	{
		Gdiplus::Bitmap* playerImage;	// �v���C���[�̉摜
		int x, y;						// �v���C���[�̍��W
		int width, height;				// �v���C���[�̕��E����
		int hp;							// �v���C���[��HP
		std::vector<BULLET> bullets;	// �v���C���[�����˂����e�B
		Gdiplus::Bitmap* bulletImage;	// �v���C���[�����˂����e�̉摜
										// �v���C���[���Ō�Ɏˌ���������
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
	PLAYER player;									// �v���C���[
	std::array<Gdiplus::Bitmap*, 11> HPImage;		// HP�o�[�̉摜

	// �G�񋓑�
	enum ENEMY_TYPE
	{
		TYPE_NORMAL,					// �ʏ�^
		TYPE_SPEED,						// �����^
		TYPE_HEAVY,						// �Η͌^
		TYPE_BOSS,						// �{�X
		TYPE_MAX						// ���v
	};
	// �G��ԍ\����
	enum ENEMY_STATUS
	{
		STATUS_BRON,					// �o�������΂���
		STATUS_NEXT						// �o�����ォ��p�^�[���ύX��
	};
	// �G�\����
	struct ENEMY
	{
		ENEMY_TYPE type;				// �G�^�C�v
		int x, y;						// �G�̍��W
		double degree;					// �v���C���[�̒��S�܂ł̉�]�p
		int width, height;				// �G�̕��E����
		int hp;							// �G��HP
		int speed;						// �ړ����x
		ENEMY_STATUS status;			// �ړ��E�U���p�^�[���𔻒f���邽�߂̃X�e�[�^�X
		int nextX, nextY;				// �����_���ړ�����ۂ̎��̍��W
										// �G���Ō�Ɏˌ���������
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
	std::vector<ENEMY> enemys;			// �G�B
										// �G�B�̉摜
	std::array<Gdiplus::Bitmap*, TYPE_MAX> enemyImage;
	std::vector<BULLET> enemyBullets;	// �G�̔��˂����e
	Gdiplus::Bitmap* enemyBulletImage;	// �G�����˂����e�̉摜
	int bossShootCheck;					// BOSS�̎ˌ������C��̔��ʗp�ϐ�

	void RespawnEnemy(void);
	double GetDegreeToPlayer(int, int, int, int);

	// �Q�[���t�F�[�Y
	enum PHASE
	{
		PHASE1,
		PHASE2,
		PHASE3,
		PHASE4
	};
	PHASE currentPhase;
										// �t�F�[�Y�Ǘ��p�ŏI���X�|�[������
	std::chrono::high_resolution_clock::time_point lastResponeTime;
	int killCount;						// �G�@���j��
	bool bossFlg;						// �{�X��t���O

	bool gameOverFlg;					// �Q�[���I�[�o�t���O
	bool gameClearFlg;					// �Q�[���N���A�t���O
	Gdiplus::Bitmap* gameOverImage;		// �Q�[���I�[�o�摜
	Gdiplus::Bitmap* gameClearImage;	// �Q�[���N���A�摜

	const int SCREEN_WIDTH;				// ��ʕ�
	const int SCREEN_HEIGHT;			// ��ʍ���

										// �����G���W��
	std::mt19937 engine{ std::random_device{ }() };

	// ���\�[�X����摜���擾����֐�
	static Gdiplus::Bitmap* GetPngImageFromResource(UINT);
	// �摜����]�����ہA��]��̍���E�E��E�������΍��W��ݒ肷��֐�
	void GetDestinationPoints(int x, int y, int width, int height, double degree, Gdiplus::Point destinationPoints[]);

	void KeyPress(void);				// �L�[���͎�t�֐�
	void DrawScreen(HDC);				// ��ʕ`��֐�
};

} // End Of Namespace SpaceShooting
