/// <summary>
/// �����}�l�[�W���[
/// 
/// ���ׂĂ̏����������Ŏ��s
/// </summary>
enum NowScene
{
    Title = 0,
    Main,
};

class MainManager
{
public:
    static void Init();     // ������
    static void Uninit();   // �I��
    static void Update();   // �X�V
    static void Draw();     // �`��

private:
    static NowScene m_nowscene;
};
