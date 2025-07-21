#include "Game.h"

using namespace SpaceShooting;

Game::Game(const int width, const int height) :SCREEN_WIDTH(width), SCREEN_HEIGHT(height)
{
    // プレイヤーとプレイヤーが発射した弾の画像の初期化
    player.playerImage = GetPngImageFromResource(IDB_PLAYER);
    player.bulletImage = GetPngImageFromResource(IDB_BULLET);

    // HPバーの画像の初期化
    for (size_t i = 0; i < HPImage.size(); ++i)
        HPImage[i] = GetPngImageFromResource(IDB_HP0 + static_cast<UINT>(i));

    // 敵の画像の初期化
    enemyImage[TYPE_NORMAL] = GetPngImageFromResource(IDB_ENEMY_NORMAL);
    enemyImage[TYPE_SPEED] = GetPngImageFromResource(IDB_ENEMY_SPEED);
    enemyImage[TYPE_HEAVY] = GetPngImageFromResource(IDB_ENEMY_HEAVY);
    enemyImage[TYPE_BOSS] = GetPngImageFromResource(IDB_ENEMY_BOSS);

    // 敵が発射した弾の画像の初期化
    enemyBulletImage = GetPngImageFromResource(IDB_ENEMY_BULLET);

    // BOSS射撃砲門の変数初期化
    bossShootCheck = 0;

    // ゲームフェーズの初期化
    currentPhase = PHASE1;
    lastResponeTime = std::chrono::high_resolution_clock::now();
    killCount = 0;
    bossFlg = false;

    // 星ランダム生成
    std::uniform_int_distribution<int> dist(0, 99);
    for (auto y = 0; y < SCREEN_HEIGHT; ++y)
        for (auto x = 0; x < SCREEN_WIDTH; ++x)
            if (dist(engine) == 0) stars.push_back({ x, y });

    // ゲームオーバ画像の初期化
    gameOverImage = GetPngImageFromResource(IDB_GAMEOVER);
    // ゲームクリア画像の初期化
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
    // ゲーム中
    if ((!gameOverFlg) && (!gameClearFlg))
    {
        // マップ更新処理
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

        // キー入力受付
        KeyPress();

        // プレイヤーの弾の更新と削除
        for (auto it = player.bullets.begin(); it != player.bullets.end(); )
        {
            // 弾の移動
            it->y -= 7;

            // 弾の画面外判定
            if (it->y <= 0) it->isActive = false;

            // 弾と敵の当たり判定
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

            // 弾の消滅判定
            if (!it->isActive) it = player.bullets.erase(it);
            else ++it;
        }

        // 敵の出現ロジック
        // 経過時刻の計算
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastResponeTime).count();

        // フェーズ確認
        if (killCount <= 10) currentPhase = PHASE1;
        else if (killCount <= 20) currentPhase = PHASE2;
        else if (killCount <= 35) currentPhase = PHASE3;
        else currentPhase = PHASE4;

#ifdef _DEBUG   // フェーズ指定
        //currentPhase = PHASE1;
        //currentPhase = PHASE2;
        //currentPhase = PHASE3;
        //currentPhase = PHASE4;
#endif

        switch (currentPhase)
        {
            // 序盤（撃破数10まで）
        case PHASE1:
            // 2秒ごとに1体の敵を確定でリスポーン（画面内は最大3体の敵）
            if ((elapsedTime >= 2000) && (enemys.size() < 3)) RespawnEnemy();
            break;
            // 中盤（撃破数20まで）
        case PHASE2:
            // 2秒ごとに1体の敵を確定、1体の敵を50%の確率でリスポーン（画面内は最大4体の敵）
            if (elapsedTime >= 2000)
            {
                if (enemys.size() < 4) RespawnEnemy();

                // ボス以外の敵をランダムで50%の確立でリスポーン
                std::uniform_int_distribution<int> distResponeRatio(0, 1);
                if (distResponeRatio(engine) && (enemys.size() < 4)) RespawnEnemy();
            }
            break;
            // 終盤（撃破数35まで）
        case PHASE3:
            // 2秒ごとに2体の敵を確定でリスポーン（画面内は最大5体の敵）
            if (elapsedTime >= 2000)
            {
                if (enemys.size() < 5) RespawnEnemy();
                if (enemys.size() < 5) RespawnEnemy();
            }
            break;
            // ボス戦
        case PHASE4:
            // ボスが出現、1秒ごとに1体の敵を50%の確率でリスポーン（ボスを含む画面内は最大4体の敵）
            if ((!bossFlg) && (enemys.size() == 0))
            {
                enemys.push_back(ENEMY(TYPE_BOSS, (SCREEN_WIDTH - 400) / 2, -177, 400, 177, 25, 2));
            }
            // ボス以外の敵をランダムでリスポーン
            else if ((bossFlg) && (elapsedTime >= 1000) && (enemys.size() < 4)) RespawnEnemy();
            break;

        default:
            break;
        }

        // 敵の更新と削除
        for (auto it = enemys.begin(); it != enemys.end(); )
        {
            // 更新処理
            switch (it->type)
            {
            case TYPE_NORMAL:
                // 角度更新
                it->degree = GetDegreeToPlayer(it->x, it->y, it->width, it->height);
                
                // 出現直後
                if (it->status == STATUS_BRON)
                {
                    it->y += it->speed;
                    // Y座標が初めて100に到達したら、次のパターンに変更
                    if (it->y >= 100) it->status = STATUS_NEXT;
                }
                // 次のパターン
                else if (it->status == STATUS_NEXT)
                {
                    // nextX,nextYは-1で初期化し、指定座標に到達した時も-1にすることで、
                    // この処理で次の座標のランダム指定を可能にする
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

                        // 目標座標に近づいたらリセット
                        if ((std::abs(it->x - it->nextX) <= 1) || (std::abs(it->y - it->nextY) <= 1))
                        {
                            it->nextX = -1;
                            it->nextY = -1;
                        }
                    }
                }
                // 攻撃
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
                    // プレイヤーとの距離
                    double distance = std::sqrt((player.x - it->x) * (player.x - it->x) + (player.y - it->y) * (player.y - it->y));
                    // プレイヤーとの距離が280以上の場合、プレイヤーに向かって飛ぶ
                    if (distance >= 280)
                    {
                        // 角度更新
                        it->degree = GetDegreeToPlayer(it->x, it->y, it->width, it->height);

                        it->speed = 3;
                    }
                    // プレイヤーとの距離が300未満の場合は次のステータスになる
                    else it->status = STATUS_NEXT;
                }
                else if (it->status == STATUS_NEXT)
                {
                    it->speed = 10;
                }
                
                // 自爆攻撃をするように移動
                it->x = static_cast<int>(it->x + std::sin(it->degree) * it->speed);
                it->y = static_cast<int>(it->y + std::cos(it->degree) * it->speed);

                // 攻撃
                // 回転後のenemyのx,yはずれるため、width,heightの半分を誤差として当たり判定に計算
                if ((it->x <= player.x + player.width) &&
                    (it->x + it->width / 2 >= player.x) &&
                    (it->y <= player.y + player.height) &&
                    (it->y + it->height / 2 >= player.y))
                {
                    player.hp -= 3;
                    it->hp = 0;
                }

                // 画面外へ飛んだ場合は削除
                if ((it->x < -it->width) || (it->x > SCREEN_WIDTH) ||
                    (it->y < -it->height) || (it->y > SCREEN_HEIGHT))
                    it->hp = 0;
            }
            break;
            case TYPE_HEAVY:
                // 出現後は自然落下
                if (it->status == STATUS_BRON)
                {
                    it->y += it->speed;
                    if (it->y >= 100) it->status = STATUS_NEXT;
                }
                // yが100に落下してからx移動開始
                else if (it->status == STATUS_NEXT)
                {
                    // xのランダム移動
                    if (it->nextX == -1)
                    {
                        std::uniform_int_distribution<int> dist(0, SCREEN_WIDTH - it->width);
                        it->nextX = dist(engine);
                    }
                    else
                    {
                        double moveAngle = std::atan2(0, static_cast<double>(it->nextX - it->x));
                        it->x = static_cast<int>(it->x + std::cos(moveAngle) * it->speed);

                        // 目標座標に到達したらリセット
                        if (it->x == it->nextX) it->nextX = -1;
                    }

                    // 攻撃（落下移動後に攻撃開始）
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
                // 出現後は自然落下
                if (it->status == STATUS_BRON)
                {
                    it->y += it->speed;
                    if (it->y >= 50) it->status = STATUS_NEXT;
                }
                else if (it->status == STATUS_NEXT)
                {
                    bossFlg = true;
                    // xの移動
                    if (it->nextX == -1)
                    {
                        // 最初と右端に到達したら左へ移動
                        if ((it->x == (SCREEN_WIDTH - 400) / 2) || (it->x == SCREEN_WIDTH - it->width)) it->nextX = 0;
                        // 左端に到達したら右へ移動
                        if (it->x == 0) it->nextX = SCREEN_WIDTH - it->width;
                    }
                    else
                    {
                        if (it->nextX == 0) it->x--;
                        if (it->nextX == SCREEN_WIDTH - it->width) it->x++;

                        // 目標座標に到達したらリセット
                        if (it->x == it->nextX) it->nextX = -1;
                    }

                    // 攻撃（落下移動後に攻撃開始）
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

            // 敵の死亡（消滅）処理
            if (it->hp <= 0)
            {
                it = enemys.erase(it);
                killCount++;
            }
            else ++it;
        }

        // 敵の弾の更新と削除
        for (auto it = enemyBullets.begin(); it != enemyBullets.end(); )
        {
            // 弾の移動
            it->y = static_cast<int>(it->y + std::cos(it->degree) * 7);
            it->x = static_cast<int>(it->x + std::sin(it->degree) * 7);

            // 弾の画面外判定
            if ((it->y >= SCREEN_HEIGHT) ||
                (it->x >= SCREEN_WIDTH) ||
                (it->x < 0))
                it->isActive = false;

            // 弾とプレイヤーのあたり判定
            if ((it->x >= player.x) && (it->x <= player.x + player.width) &&
                (it->y >= player.y) && (it->y <= player.y + player.height))
            {
                it->isActive = false;
                player.hp--;
            }

            // 弾の削除処理
            if (!it->isActive) it = enemyBullets.erase(it);
            else ++it;
        }

#ifdef _DEBUG   // プレイヤー無敵
        //player.hp = 10;
#endif

        // ゲームオーバ処理
        if (player.hp <= 0)
        {
            player.hp = 0;
            gameOverFlg = true;
        }
    }

    // 描画処理
    DrawScreen(hdc);
}

void Game::RespawnEnemy()
{
    // ボス以外の敵をランダムでリスポーン
    std::uniform_int_distribution<int> dist(1, 8);
    int respawnEnemyTypeRatio = dist(engine);

    ENEMY_TYPE respawnEnemyType;	    // 敵タイプ
    int respawnEnemyX = 0;	            // 敵のX座標
    int respawnEnemyY = 0;              // 敵のY座標
    int respawnEnemyW, respawnEnemyH;	// 敵の幅・高さ
    int respawnEnemyHP;					// 敵のHP
    int respawnEnemySPD;				// 移動速度

#ifdef _DEBUG   // 出現する敵タイプの指定
    //respawnEnemyTypeRatio = 4;
    //respawnEnemyTypeRatio = 7;
    //respawnEnemyTypeRatio = 8;
#endif
    // ボス戦時は火力型出現無し、通常型と自爆型半々の確率で出現
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
        // 出現場所左端、右端、上端ランダム
        std::uniform_int_distribution<int> distRespawnPoint(0, 2);
        respawnEnemyType = TYPE_SPEED;
        respawnEnemyW = 50;
        respawnEnemyH = 29;
        switch (distRespawnPoint(engine))
        {
        // 上端から出現
        case 0:
        {
            std::uniform_int_distribution<int> dist(0, SCREEN_WIDTH - respawnEnemyW - 1);
            respawnEnemyX = dist(engine);
            respawnEnemyY = -respawnEnemyH;
        }
        break;
        // 左端から出現
        case 1:
        {
            respawnEnemyX = -respawnEnemyW;
            std::uniform_int_distribution<int> dist(0, (SCREEN_HEIGHT - respawnEnemyH) / 4);
            respawnEnemyY = dist(engine);
        }
        break;
        // 右端から出現
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
    // プレイヤーの中心座標
    int playerCenterX = player.x + player.width / 2;
    int playerCenterY = player.y + player.height / 2;
    // 敵の中心座標
    int enemyCenterX = X + W / 2;
    int enemyCenterY = Y + H / 2;
    // 回転角計算
    return std::atan2(static_cast<double>(playerCenterX - enemyCenterX), static_cast<double>(playerCenterY - enemyCenterY));
}

void Game::DrawScreen(HDC hdc)
{
    Gdiplus::Graphics graphics(hdc);
    graphics.Clear(Gdiplus::Color(0, 0, 0));

    // 星空の描画
    for (auto it = stars.begin(); it != stars.end(); ++it)
        SetPixel(hdc, it->x, it->y, RGB(255, 255, 255));

    // プレイヤーの描画
    if (!gameOverFlg)
        graphics.DrawImage(player.playerImage, player.x, player.y, player.width, player.height);

    // HPバーの描画
    graphics.DrawImage(HPImage[player.hp], 10, SCREEN_HEIGHT - 30, 150, 11);

    // プレイヤーの弾の描画
    for (const auto& bullet : player.bullets)
        if (bullet.isActive) 
            graphics.DrawImage(player.bulletImage, bullet.x, bullet.y, 5, 65);

    // 敵の描画
    for (const auto& enemy : enemys)
    {
        // 回転後の座標設定
        Gdiplus::Point destinationPoints[3];
        GetDestinationPoints(enemy.x, enemy.y, enemy.width, enemy.height, enemy.degree, destinationPoints);
        graphics.DrawImage(enemyImage[enemy.type], destinationPoints, 3);
    }

    // 敵が発射した弾の描画
    for (const auto& bullet : enemyBullets)
        if (bullet.isActive)
        {
            Gdiplus::Point destinationPoints[3];
            GetDestinationPoints(bullet.x, bullet.y, bullet.width, bullet.height, bullet.degree, destinationPoints);
            graphics.DrawImage(enemyBulletImage, destinationPoints, 3);
        }

    // ゲームオーバ
    if (gameOverFlg) graphics.DrawImage(gameOverImage, (SCREEN_WIDTH - 320) / 2, (SCREEN_HEIGHT - 32) / 2, 320, 32);
    // ゲームクリア
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
    // 画像の中心点相対座標
    int cx0 = width / 2;
    int cy0 = height / 2;

    // 画像の中心から4頂点までの距離
    double d = std::sqrt(static_cast<double>(cx0 * cx0 + cy0 * cy0));

    // 左上・右上・左下の相対座標
    int cx[3] = {0, width, 0};
    int cy[3] = {0, 0, height};

    for (int i = 0; i < 3; ++i)
    {
        // 頂点から中心点のある水平線の角度
        double angle = std::atan2(static_cast<double>(cy[i] - cy0), static_cast<double>(cx[i] - cx0));

        // 回転後の角度
        angle -= degree;

        // 回転後の頂点の座標
        cx[i] = static_cast<int>(std::cos(angle) * d) + cx0;
        cy[i] = static_cast<int>(std::sin(angle) * d) + cy0;

        // 回転後の頂点座標を変換行列に代入
        destinationPoints[i] = Gdiplus::Point(cx[i] + x, cy[i] + y);
    }
}
