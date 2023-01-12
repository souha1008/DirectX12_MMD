#include "framework.h"
#include<cmath>
#include "Helper.h"

using namespace std;

Helper::Helper()
{
}


Helper::~Helper()
{
}

std::vector<float>
GetGaussianWeights(size_t count, float s) {
	std::vector<float> weights(count);//�E�F�C�g�z��ԋp�p
	float x = 0.0f;
	float total = 0.0f;
	for (auto& wgt : weights) {
		wgt = expf(-(x * x) / (2 * s * s));
		total += wgt;
		x += 1.0f;
	}
	//�^�񒆂𒆐S�ɍ��E�ɍL����悤�ɍ��܂��̂�
	//���E�Ƃ�������2�{���܂��B���������̏ꍇ�͒��S��0�̃s�N�Z����
	//�d�����Ă��܂��܂��̂�e^0=1�Ƃ������ƂōŌ��1�������Ē��낪�����悤�ɂ��Ă��܂��B
	total = total * 2.0f - 1.0f;
	//�����ĂP�ɂȂ�悤�ɂ���
	for (auto& wgt : weights) {
		wgt /= total;
	}
	return weights;
}

//�P�o�C�gstring�����C�h����wstring�ɕϊ�����
wstring WStringFromString(const std::string& str) {
	wstring wstr;
	auto wcharNum = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), nullptr, 0);
	wstr.resize(wcharNum);
	wcharNum = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(),
		&wstr[0], wstr.size());
	return wstr;
}

///�g���q��Ԃ�
///@param path ���̃p�X������
///@return �g���q������
wstring GetExtension(const wstring& path) {
	int index = path.find_last_of(L'.');
	return path.substr(index + 1, path.length() - index);
}

bool CheckResult(HRESULT& result, ID3DBlob* errBlob)
{
	if (FAILED(result)) {
#ifdef _DEBUG
		if (errBlob != nullptr) {
			std::string outmsg;
			outmsg.resize(errBlob->GetBufferSize());
			std::copy_n(static_cast<char*>(errBlob->GetBufferPointer()),
				errBlob->GetBufferSize(),
				outmsg.begin());
			OutputDebugString(outmsg.c_str());//�o�̓E�B���h�E�ɏo�͂��Ă�
		}
		assert(SUCCEEDED(result));
#endif
		return false;
	}
	else {
		return true;
	}
}

unsigned int
AligmentedValue(unsigned int size, unsigned int alignment) {
	return (size + alignment - (size % alignment));
}