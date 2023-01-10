/// <summary>
/// 総合マネージャー
/// 
/// すべての処理をここで実行
/// </summary>
enum NowScene
{
    Title = 0,
    Main,
};

class MainManager
{
public:
    static void Init();     // 初期化
    static void Uninit();   // 終了
    static void Update();   // 更新
    static void Draw();     // 描画

private:
    static NowScene m_nowscene;
};
