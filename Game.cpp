#include "Game.h"

using namespace SpaceShooting;

Game::Game(const int width, const int height) :SCREEN_WIDTH(width), SCREEN_HEIGHT(height)
{
    // �v���C���[�ƃv���C���[�����˂����e�̉摜�̏�����
    player.playerImage = GetPngImageFromResource(IDB_PLAYER);
    player.bulletImage = GetPngImageFromResource(IDB_BULLET);

    // HP�o�[�̉摜�̏�����
    for (size_t i = 0; i < HPImage.size(); ++i)
        HPImage[i] = GetPngImageFromResource(IDB_HP0 + static_cast<UINT>(i));

    // �G�̉摜�̏�����
    enemyImage[TYPE_NORMAL] = GetPngImageFromResource(IDB_ENEMY_NORMAL);
    enemyImage[TYPE_SPEED] = GetPngImageFromResource(IDB_ENEMY_SPEED);
    enemyImage[TYPE_HEAVY] = GetPngImageFromResource(IDB_ENEMY_HEAVY);
    enemyImage[TYPE_BOSS] = GetPngImageFromResource(IDB_ENEMY_BOSS);

    // �G�����˂����e�̉摜�̏�����
    enemyBulletImage = GetPngImageFromResource(IDB_ENEMY_BULLET);

    // BOSS�ˌ��C��̕ϐ�������
    bossShootCheck = 0;

    // �Q�[���t�F�[�Y�̏�����
    currentPhase = PHASE1;
    lastResponeTime = std::chrono::high_resolution_clock::now();
    killCount = 0;
    bossFlg = false;

    // �������_������
    std::uniform_int_distribution<int> dist(0, 99);
    for (auto y = 0; y < SCREEN_HEIGHT; ++y)
        for (auto x = 0; x < SCREEN_WIDTH; ++x)
            if (dist(engine) == 0) stars.push_back({ x, y });

    // �Q�[���I�[�o�摜�̏�����
    gameOverImage = GetPngImageFromResource(IDB_GAMEOVER);
    // �Q�[���N���A�摜�̏�����
    gameClearImage = GetPngImageFromResource(IDB_GAMECLEAR);
}

Game::~Game()
{
    delete enemyBulletImage;
    delete gameOverImage;
    delete gameClearImage;
}

void Game::Update(HDC hdc)
{
    // �Q�[����
    if ((!gameOverFlg) && (!gameClearFlg))
    {
        // �}�b�v�X�V����
        for (auto& star : stars)
        {
            star.y++;
            if (star.y >= SCREEN_HEIGHT)
            {
                std::uniform_int_distribution<int> dist(0, SCREEN_WIDTH - 1);
                star.x = dist(engine);
                star.y = 0;
            }
        }

        // �L�[���͎�t
        KeyPress();

        // �v���C���[�̒e�̍X�V�ƍ폜
        for (auto it = player.bullets.begin(); it != player.bullets.end(); )
        {
            // �e�̈ړ�
            it->y -= 7;

            // �e�̉�ʊO����
            if (it->y <= 0) it->isActive = false;

            // �e�ƓG�̓����蔻��
            for (auto& enemy : enemys)
            {
                if ((it->x >= enemy.x) && (it->x <= enemy.x + enemy.width) &&
                    (it->y >= enemy.y) && (it->y <= enemy.y + enemy.height))
                {
                    it->isActive = false;
                    enemy.hp--;
                    if ((enemy.type == TYPE_BOSS) && (enemy.hp <= 0)) gameClearFlg = true;
                }
            }

            // �e�̏��Ŕ���
            if (!it->isActive) it = player.bullets.erase(it);
            else ++it;
        }

        // �G�̏o�����W�b�N
        // �o�ߎ����̌v�Z
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastResponeTime).count();

        // �t�F�[�Y�m�F
        if (killCount <= 10) currentPhase = PHASE1;
        else if (killCount <= 20) currentPhase = PHASE2;
        else if (killCount <= 35) currentPhase = PHASE3;
        else currentPhase = PHASE4;

#ifdef _DEBUG   // �t�F�[�Y�w��
        //currentPhase = PHASE1;
        //currentPhase = PHASE2;
        //currentPhase = PHASE3;
        //currentPhase = PHASE4;
#endif

        switch (currentPhase)
        {
            // ���Ձi���j��10�܂Łj
        case PHASE1:
            // 2�b���Ƃ�1�̂̓G���m��Ń��X�|�[���i��ʓ��͍ő�3�̂̓G�j
            if ((elapsedTime >= 2000) && (enemys.size() < 3)) RespawnEnemy();
            break;
            // ���Ձi���j��20�܂Łj
        case PHASE2:
            // 2�b���Ƃ�1�̂̓G���m��A1�̂̓G��50%�̊m���Ń��X�|�[���i��ʓ��͍ő�4�̂̓G�j
            if (elapsedTime >= 2000)
            {
                if (enemys.size() < 4) RespawnEnemy();

                // �{�X�ȊO�̓G�������_����50%�̊m���Ń��X�|�[��
                std::uniform_int_distribution<int> distResponeRatio(0, 1);
                if (distResponeRatio(engine) && (enemys.size() < 4)) RespawnEnemy();
            }
            break;
            // �I�Ձi���j��35�܂Łj
        case PHASE3:
            // 2�b���Ƃ�2�̂̓G���m��Ń��X�|�[���i��ʓ��͍ő�5�̂̓G�j
            if (elapsedTime >= 2000)
            {
                if (enemys.size() < 5) RespawnEnemy();
                if (enemys.size() < 5) RespawnEnemy();
            }
            break;
            // �{�X��
        case PHASE4:
            // �{�X���o���A1�b���Ƃ�1�̂̓G��50%�̊m���Ń��X�|�[���i�{�X���܂މ�ʓ��͍ő�4�̂̓G�j
            if ((!bossFlg) && (enemys.size() == 0))
            {
                enemys.push_back(ENEMY(TYPE_BOSS, (SCREEN_WIDTH - 400) / 2, -177, 400, 177, 25, 2));
            }
            // �{�X�ȊO�̓G�������_���Ń��X�|�[��
            else if ((bossFlg) && (elapsedTime >= 1000) && (enemys.size() < 4)) RespawnEnemy();
            break;

        default:
            break;
        }

        // �G�̍X�V�ƍ폜
        for (auto it = enemys.begin(); it != enemys.end(); )
        {
            // �X�V����
            switch (it->type)
            {
            case TYPE_NORMAL:
                // �p�x�X�V
                it->degree = GetDegreeToPlayer(it->x, it->y, it->width, it->height);
                
                // �o������
                if (it->status == STATUS_BRON)
                {
                    it->y += it->speed;
                    // Y���W�����߂�100�ɓ��B������A���̃p�^�[���ɕύX
                    if (it->y >= 100) it->status = STATUS_NEXT;
                }
                // ���̃p�^�[��
                else if (it->status == STATUS_NEXT)
                {
                    // nextX,nextY��-1�ŏ��������A�w����W�ɓ��B��������-1�ɂ��邱�ƂŁA
                    // ���̏����Ŏ��̍��W�̃����_���w����\�ɂ���
                    if ((it->nextX == -1) || (it->nextY == -1))
                    {
                        std::uniform_int_distribution<int> distX(0, SCREEN_WIDTH - it->width);
                        std::uniform_int_distribution<int> distY(0, SCREEN_HEIGHT / 2 - it->height);
                        it->nextX = distX(engine);
                        it->nextY = distY(engine);
                    }
                    else
                    {
                        double moveAngle = std::atan2(static_cast<double>(it->nextY - it->y), static_cast<double>(it->nextX - it->x));
                        it->x = static_cast<int>(it->x + std::cos(moveAngle) * it->speed);
                        it->y = static_cast<int>(it->y + std::sin(moveAngle) * it->speed);

                        // �ڕW���W�ɋ߂Â����烊�Z�b�g
                        if ((std::abs(it->x - it->nextX) <= 1) || (std::abs(it->y - it->nextY) <= 1))
                        {
                            it->nextX = -1;
                            it->nextY = -1;
                        }
                    }
                }
                // �U��
                if (std::chrono::duration_cast<std::chrono::milliseconds>
                    (std::chrono::high_resolution_clock::now() - it->lastShootTime).count() >= 2500)
                {
                    it->lastShootTime = std::chrono::high_resolution_clock::now();
                    enemyBullets.push_back({ it->x + (it->width - 8) / 2, it->y, 8, 74, it->degree, true });
                }
                break;
            case TYPE_SPEED:
            {
                if (it->status == STATUS_BRON)
                {
                    // �v���C���[�Ƃ̋���
                    double distance = std::sqrt((player.x - it->x) * (player.x - it->x) + (player.y - it->y) * (player.y - it->y));
                    // �v���C���[�Ƃ̋�����280�ȏ�̏ꍇ�A�v���C���[�Ɍ������Ĕ��
                    if (distance >= 280)
                    {
                        // �p�x�X�V
                        it->degree = GetDegreeToPlayer(it->x, it->y, it->width, it->height);

                        it->speed = 3;
                    }
                    // �v���C���[�Ƃ̋�����300�����̏ꍇ�͎��̃X�e�[�^�X�ɂȂ�
                    else it->status = STATUS_NEXT;
                }
                else if (it->status == STATUS_NEXT)
                {
                    it->speed = 10;
                }
                
                // �����U��������悤�Ɉړ�
                it->x = static_cast<int>(it->x + std::sin(it->degree) * it->speed);
                it->y = static_cast<int>(it->y + std::cos(it->degree) * it->speed);

                // �U��
                // ��]���enemy��x,y�͂���邽�߁Awidth,height�̔������덷�Ƃ��ē����蔻��Ɍv�Z
                if ((it->x <= player.x + player.width) &&
                    (it->x + it->width / 2 >= player.x) &&
                    (it->y <= player.y + player.height) &&
                    (it->y + it->height / 2 >= player.y))
                {
                    player.hp -= 3;
                    it->hp = 0;
                }

                // ��ʊO�֔�񂾏ꍇ�͍폜
                if ((it->x < -it->width) || (it->x > SCREEN_WIDTH) ||
                    (it->y < -it->height) || (it->y > SCREEN_HEIGHT))
                    it->hp = 0;
            }
            break;
            case TYPE_HEAVY:
                // �o����͎��R����
                if (it->status == STATUS_BRON)
                {
                    it->y += it->speed;
                    if (it->y >= 100) it->status = STATUS_NEXT;
                }
                // y��100�ɗ������Ă���x�ړ��J�n
                else if (it->status == STATUS_NEXT)
                {
                    // x�̃����_���ړ�
                    if (it->nextX == -1)
                    {
                        std::uniform_int_distribution<int> dist(0, SCREEN_WIDTH - it->width);
                        it->nextX = dist(engine);
                    }
                    else
                    {
                        double moveAngle = std::atan2(0, static_cast<double>(it->nextX - it->x));
                        it->x = static_cast<int>(it->x + std::cos(moveAngle) * it->speed);

                        // �ڕW���W�ɓ��B�����烊�Z�b�g
                        if (it->x == it->nextX) it->nextX = -1;
                    }

                    // �U���i�����ړ���ɍU���J�n�j
                    if (std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::high_resolution_clock::now() - it->lastShootTime).count() >= 1000)
                    {
                        it->lastShootTime = std::chrono::high_resolution_clock::now();
                        enemyBullets.push_back({ it->x + 40, it->y, 8, 74, it->degree, true });
                        enemyBullets.push_back({ it->x + 60, it->y + 10, 8, 74, it->degree, true });
                        enemyBullets.push_back({ it->x + 130, it->y + 10, 8, 74, it->degree, true });
                        enemyBullets.push_back({ it->x + 150, it->y, 8, 74, it->degree, true });
                    }
                }
                break;
            case TYPE_BOSS:
                // �o����͎��R����
                if (it->status == STATUS_BRON)
                {
                    it->y += it->speed;
                    if (it->y >= 50) it->status = STATUS_NEXT;
                }
                else if (it->status == STATUS_NEXT)
                {
                    bossFlg = true;
                    // x�̈ړ�
                    if (it->nextX == -1)
                    {
                        // �ŏ��ƉE�[�ɓ��B�����獶�ֈړ�
                        if ((it->x == (SCREEN_WIDTH - 400) / 2) || (it->x == SCREEN_WIDTH - it->width)) it->nextX = 0;
                        // ���[�ɓ��B������E�ֈړ�
                        if (it->x == 0) it->nextX = SCREEN_WIDTH - it->width;
                    }
                    else
                    {
                        if (it->nextX == 0) it->x--;
                        if (it->nextX == SCREEN_WIDTH - it->width) it->x++;

                        // �ڕW���W�ɓ��B�����烊�Z�b�g
                        if (it->x == it->nextX) it->nextX = -1;
                    }

                    // �U���i�����ړ���ɍU���J�n�j
                    if (std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::high_resolution_clock::now() - it->lastShootTime).count() >= 250)
                    {
                        if (bossShootCheck % 2)
                        {
                            enemyBullets.push_back({ it->x + 68, it->y + 50, 8, 74, it->degree, true });
                            enemyBullets.push_back({ it->x + 78, it->y + 50, 8, 74, it->degree, true });
                        }
                        else
                        {
                            enemyBullets.push_back({ it->x + 315, it->y + 50, 8, 74, it->degree, true });
                            enemyBullets.push_back({ it->x + 325, it->y + 50, 8, 74, it->degree, true });
                        }
                        bossShootCheck++;

                        it->lastShootTime = std::chrono::high_resolution_clock::now();
                    }
                }
                break;
            default:
                break;
            }

            // �G�̎��S�i���Łj����
            if (it->hp <= 0)
            {
                it = enemys.erase(it);
                killCount++;
            }
            else ++it;
        }

        // �G�̒e�̍X�V�ƍ폜
        for (auto it = enemyBullets.begin(); it != enemyBullets.end(); )
        {
            // �e�̈ړ�
            it->y = static_cast<int>(it->y + std::cos(it->degree) * 7);
            it->x = static_cast<int>(it->x + std::sin(it->degree) * 7);

            // �e�̉�ʊO����
            if ((it->y >= SCREEN_HEIGHT) ||
                (it->x >= SCREEN_WIDTH) ||
                (it->x < 0))
                it->isActive = false;

            // �e�ƃv���C���[�̂����蔻��
            if ((it->x >= player.x) && (it->x <= player.x + player.width) &&
                (it->y >= player.y) && (it->y <= player.y + player.height))
            {
                it->isActive = false;
                player.hp--;
            }

            // �e�̍폜����
            if (!it->isActive) it = enemyBullets.erase(it);
            else ++it;
        }

#ifdef _DEBUG   // �v���C���[���G
        //player.hp = 10;
#endif

        // �Q�[���I�[�o����
        if (player.hp <= 0)
        {
            player.hp = 0;
            gameOverFlg = true;
        }
    }

    // �`�揈��
    DrawScreen(hdc);
}

void Game::RespawnEnemy()
{
    // �{�X�ȊO�̓G�������_���Ń��X�|�[��
    std::uniform_int_distribution<int> dist(1, 8);
    int respawnEnemyTypeRatio = dist(engine);

    ENEMY_TYPE respawnEnemyType;	    // �G�^�C�v
    int respawnEnemyX = 0;	            // �G��X���W
    int respawnEnemyY = 0;              // �G��Y���W
    int respawnEnemyW, respawnEnemyH;	// �G�̕��E����
    int respawnEnemyHP;					// �G��HP
    int respawnEnemySPD;				// �ړ����x

#ifdef _DEBUG   // �o������G�^�C�v�̎w��
    //respawnEnemyTypeRatio = 4;
    //respawnEnemyTypeRatio = 7;
    //respawnEnemyTypeRatio = 8;
#endif
    // �{�X�펞�͉Η͌^�o�������A�ʏ�^�Ǝ����^���X�̊m���ŏo��
    if ((currentPhase == PHASE4) && (respawnEnemyTypeRatio == 8)) respawnEnemyTypeRatio = 7;

    if (respawnEnemyTypeRatio <= 4)
    {
        respawnEnemyType = TYPE_NORMAL;
        respawnEnemyW = 50;
        std::uniform_int_distribution<int> dist(0, SCREEN_WIDTH - respawnEnemyW - 1);
        respawnEnemyX = dist(engine);
        respawnEnemyH = 72;
        respawnEnemyY = -respawnEnemyH;
        respawnEnemyHP = 3;
        respawnEnemySPD = 2;
    }
    else if (respawnEnemyTypeRatio <= 7)
    {
        // �o���ꏊ���[�A�E�[�A��[�����_��
        std::uniform_int_distribution<int> distRespawnPoint(0, 2);
        respawnEnemyType = TYPE_SPEED;
        respawnEnemyW = 50;
        respawnEnemyH = 29;
        switch (distRespawnPoint(engine))
        {
        // ��[����o��
        case 0:
        {
            std::uniform_int_distribution<int> dist(0, SCREEN_WIDTH - respawnEnemyW - 1);
            respawnEnemyX = dist(engine);
            respawnEnemyY = -respawnEnemyH;
        }
        break;
        // ���[����o��
        case 1:
        {
            respawnEnemyX = -respawnEnemyW;
            std::uniform_int_distribution<int> dist(0, (SCREEN_HEIGHT - respawnEnemyH) / 4);
            respawnEnemyY = dist(engine);
        }
        break;
        // �E�[����o��
        case 2:
        {
            respawnEnemyX = SCREEN_WIDTH;
            std::uniform_int_distribution<int> dist(0, (SCREEN_HEIGHT - respawnEnemyH) / 4);
            respawnEnemyY = dist(engine);
        }
        break;
        default:
            break;
        }
        respawnEnemyHP = 1;
        respawnEnemySPD = 2;
    }
    else
    {
        respawnEnemyType = TYPE_HEAVY;
        respawnEnemyW = 200;
        std::uniform_int_distribution<int> dist(0, SCREEN_WIDTH - respawnEnemyW - 1);
        respawnEnemyX = dist(engine);
        respawnEnemyH = 84;
        respawnEnemyY = -respawnEnemyH;
        respawnEnemyHP = 5;
        respawnEnemySPD = 1;
    }
    enemys.push_back(ENEMY(respawnEnemyType, respawnEnemyX, respawnEnemyY, respawnEnemyW, respawnEnemyH, respawnEnemyHP, respawnEnemySPD));
    lastResponeTime = std::chrono::high_resolution_clock::now();
}

double Game::GetDegreeToPlayer(int X, int Y,int W, int H)
{
    // �v���C���[�̒��S���W
    int playerCenterX = player.x + player.width / 2;
    int playerCenterY = player.y + player.height / 2;
    // �G�̒��S���W
    int enemyCenterX = X + W / 2;
    int enemyCenterY = Y + H / 2;
    // ��]�p�v�Z
    return std::atan2(static_cast<double>(playerCenterX - enemyCenterX), static_cast<double>(playerCenterY - enemyCenterY));
}

void Game::DrawScreen(HDC hdc)
{
    Gdiplus::Graphics graphics(hdc);
    graphics.Clear(Gdiplus::Color(0, 0, 0));

    // ����̕`��
    for (auto it = stars.begin(); it != stars.end(); ++it)
        SetPixel(hdc, it->x, it->y, RGB(255, 255, 255));

    // �v���C���[�̕`��
    if (!gameOverFlg)
        graphics.DrawImage(player.playerImage, player.x, player.y, player.width, player.height);

    // HP�o�[�̕`��
    graphics.DrawImage(HPImage[player.hp], 10, SCREEN_HEIGHT - 30, 150, 11);

    // �v���C���[�̒e�̕`��
    for (const auto& bullet : player.bullets)
        if (bullet.isActive) 
            graphics.DrawImage(player.bulletImage, bullet.x, bullet.y, 5, 65);

    // �G�̕`��
    for (const auto& enemy : enemys)
    {
        // ��]��̍��W�ݒ�
        Gdiplus::Point destinationPoints[3];
        GetDestinationPoints(enemy.x, enemy.y, enemy.width, enemy.height, enemy.degree, destinationPoints);
        graphics.DrawImage(enemyImage[enemy.type], destinationPoints, 3);
    }

    // �G�����˂����e�̕`��
    for (const auto& bullet : enemyBullets)
        if (bullet.isActive)
        {
            Gdiplus::Point destinationPoints[3];
            GetDestinationPoints(bullet.x, bullet.y, bullet.width, bullet.height, bullet.degree, destinationPoints);
            graphics.DrawImage(enemyBulletImage, destinationPoints, 3);
        }

    // �Q�[���I�[�o
    if (gameOverFlg) graphics.DrawImage(gameOverImage, (SCREEN_WIDTH - 320) / 2, (SCREEN_HEIGHT - 32) / 2, 320, 32);
    // �Q�[���N���A
    else if (gameClearFlg) graphics.DrawImage(gameClearImage, (SCREEN_WIDTH - 320) / 2, (SCREEN_HEIGHT - 32) / 2, 320, 32);
}

void Game::KeyPress()
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - player.lastShootTime).count();

    if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && (elapsedTime >= 300))
    {
        player.bullets.push_back({ player.x + (player.width - 5) / 2, player.y, 5, 65, 0, true });
        player.lastShootTime = currentTime;
    }

    if (GetAsyncKeyState(KEY_W) & 0x8000)
    {
        player.y -= 3;
        if (player.y < 0) player.y = 0;
    }

    if (GetAsyncKeyState(KEY_A) & 0x8000)
    {
        player.x -= 3;
        if (player.x < 0) player.x = 0;
    }

    if (GetAsyncKeyState(KEY_S) & 0x8000)
    {
        player.y += 3;
        if (player.y > SCREEN_HEIGHT - player.height) player.y = SCREEN_HEIGHT - player.height;
    }

    if (GetAsyncKeyState(KEY_D) & 0x8000)
    {
        player.x += 3;
        if (player.x > SCREEN_WIDTH - player.width) player.x = SCREEN_WIDTH - player.width;
    }
}

Gdiplus::Bitmap* Game::GetPngImageFromResource(UINT uId)
{
    HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCE(uId), TEXT("png"));
    if (!hResource) return nullptr;

    DWORD imageSize = SizeofResource(nullptr, hResource);
    if (imageSize == 0) return nullptr;

    HGLOBAL hGlobal = LoadResource(nullptr, hResource);
    if (!hGlobal) return nullptr;

    const void* pResourceData = LockResource(hGlobal);
    if (!pResourceData) return nullptr;

    HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
    if (!hBuffer) return nullptr;

    void* pBuffer = GlobalLock(hBuffer);
    if (!pBuffer)
    {
        GlobalFree(hBuffer);
        return nullptr;
    }

    CopyMemory(pBuffer, pResourceData, imageSize);

    IStream* pStream = nullptr;
    HRESULT hr = CreateStreamOnHGlobal(hBuffer, FALSE, &pStream);
    if (hr != S_OK)
    {
        GlobalUnlock(hBuffer);
        GlobalFree(hBuffer);
        return nullptr;
    }

    Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(pStream, FALSE);

    pStream->Release();
    GlobalUnlock(hBuffer);
    GlobalFree(hBuffer);

    if (bitmap->GetLastStatus() != Gdiplus::Ok)
    {
        delete bitmap;
        return nullptr;
    }

    return bitmap;
}

void Game::GetDestinationPoints(int x, int y, int width, int height, double degree, Gdiplus::Point destinationPoints[])
{
    // �摜�̒��S�_���΍��W
    int cx0 = width / 2;
    int cy0 = height / 2;

    // �摜�̒��S����4���_�܂ł̋���
    double d = std::sqrt(static_cast<double>(cx0 * cx0 + cy0 * cy0));

    // ����E�E��E�����̑��΍��W
    int cx[3] = {0, width, 0};
    int cy[3] = {0, 0, height};

    for (int i = 0; i < 3; ++i)
    {
        // ���_���璆�S�_�̂��鐅�����̊p�x
        double angle = std::atan2(static_cast<double>(cy[i] - cy0), static_cast<double>(cx[i] - cx0));

        // ��]��̊p�x
        angle -= degree;

        // ��]��̒��_�̍��W
        cx[i] = static_cast<int>(std::cos(angle) * d) + cx0;
        cy[i] = static_cast<int>(std::sin(angle) * d) + cy0;

        // ��]��̒��_���W��ϊ��s��ɑ��
        destinationPoints[i] = Gdiplus::Point(cx[i] + x, cy[i] + y);
    }
}
