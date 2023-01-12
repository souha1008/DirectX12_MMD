#pragma once
#include<d3d12.h>
#include<string>

class Helper
{
public:
    Helper();
    ~Helper();
};

///���U���g���`�F�b�N���A�_����������false��Ԃ�
///@param result DX�֐�����̖߂�l
///@param errBlob �G���[������Ȃ�G���[���o��
///@remarks �f�o�b�O���ɂ�errBlob���f�o�b�O�o�͂��s��
///���̂܂܃N���b�V������
extern bool CheckResult(HRESULT& result, ID3DBlob* errBlob = nullptr);

///�A���C�����g���l��Ԃ�
///@param size �A���C�����g�Ώۂ̃T�C�Y
///@param alignment �A���C�����g�T�C�Y
///@retval �A���C�����g����Ă��܂����T�C�Y
extern unsigned int AligmentedValue(unsigned int size, unsigned int alignment = 16);


//�P�o�C�gstring�����C�h����wstring�ɕϊ�����
extern std::wstring WStringFromString(const std::string& str);

///�g���q��Ԃ�
///@param path ���̃p�X������
///@return �g���q������
extern std::wstring GetExtension(const std::wstring& path);

///�K�E�X�E�F�C�g�l��Ԃ�
///@param count �Б������̃E�F�C�g�l�������Ă�����
///@param s �K�E�X�֐��ɂ�����Вl
///@retval �K�E�X�E�F�C�g�z��(count�Ŏw�肳�ꂽ�ϒ��z��)
///@remarks �S�ẴE�F�C�g�l�𑫂�����1�ɂȂ�͂��Ȃ̂�
extern std::vector<float> GetGaussianWeights(size_t count, float s);