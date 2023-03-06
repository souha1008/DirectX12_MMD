#include "PeraHeader.hlsli"

float4 ps(Output input) : SV_TARGET
{
	float4 col = tex.Sample(smp, input.uv);
	
	// そのまま
	//return tex.Sample(smp, input.uv);

	// グレースケール
	//float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
	//return float4(Y, Y, Y, 1);

	// 色反転
	//return float4(float3(1, 1, 1) - col.rgb, col.a);

	// 色調を落とす
	//return float4(col.rgb - fmod(col.rgb, 0.25f), col.a);


	return col;

}

float4 BlurPS(Output input) : SV_TARGET
{
	// ぼかし
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy));	// 左上
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy));			// 上
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy));		// 右上

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0));			// 左
	ret += tex.Sample(smp, input.uv);								// 自分
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0));			// 右

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy));	// 左上
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy));			// 上
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy));		// 右上

	return ret / 9.0f;
}

float4 EmbossPS(Output input) : SV_TARGET
{
	// エンボス加工
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 2;	// 左上
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy));				// 上
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;		// 右上

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0));			// 左
	ret += tex.Sample(smp, input.uv);								// 自分
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;			// 右

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy))* 2;	// 左上
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;			// 下
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 0;		// 右上

	float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));

	return float4(Y, Y, Y, ret.a);
}

float4 OutlinePS(Output input) : SV_TARGET
{
	// 輪郭線抽出
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1;			// 上
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1;			// 左
	ret += tex.Sample(smp, input.uv) * 4;								// 自分
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;			// 右
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;			// 上

	// 色反転
	float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));
	// 強調
	Y = pow(1.0f - Y, 10.0f);
	// 余計な輪郭線を消す
	Y = step(0.2, Y);

	return float4(Y, Y, Y, ret.a);
}

float4 GaussianBlur(Output input) : SV_TARGET
{
	// ぼかし
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 2.0f / w;
	float dy = 2.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	//今のピクセルを中心に縦横5つずつになるよう加算する
	//最上段
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, 2 * dy)) * 6 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, 2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 1 / 256;
	//ひとつ上段
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, 1 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, 1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 1 * dy)) * 4 / 256;
	//中心列
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) * 6 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, 0 * dy)) * 36 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, 0 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0 * dy)) * 6 / 256;
	//一つ下段
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, -1 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, -1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -1 * dy)) * 4 / 256;
	//最下段
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 1 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, -2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, -2 * dy)) * 6 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, -2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 1 / 256;

	return ret;
}

float4 VerticalBokehPS(Output input) : SV_TARGET
{
	float w, h, level;
	tex.GetDimensions(0, w, h, level);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	float4 col = tex.Sample(smp, input.uv);
	ret += bkWeights[0] * col;
	for (int i = 1; i < 8; ++i) {
		ret += bkWeights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, dy * i));
		ret += bkWeights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, -dy * i));	// float2の中身を逆にすると縦方向にぼかせる
	}
	return float4(ret.rgb, col.a);
}

float4 HorizontalBokehPS(Output input) : SV_TARGET
{
	float w, h, level;
	tex.GetDimensions(0, w, h, level);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);



	float4 col = tex.Sample(smp, input.uv);
	ret += bkWeights[0] * col;
	for (int i = 1; i < 8; ++i) {
		ret += bkWeights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(i * dx, 0));
		ret += bkWeights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(-i * dx, 0));
	}
	return float4(ret.rgb,col.a);
}