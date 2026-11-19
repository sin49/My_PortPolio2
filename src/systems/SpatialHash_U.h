// Spatial Hash: 방을 같은 크기 칸으로 나눠 "근처 후보"만 빠르게 (docs/design/spatial-hash.md). World·Entity는 모름
// 사용자 작성 (Codex 리뷰·중복 제거 예시 도움, 11/19). 이름 정리만 AI
#pragma once


#include "core/Handle_U.h"
#include "core/Rect_C.h"
#include <cstdint>                      
#include <unordered_map>                 
#include <vector>

class SpatialHash
{
public:
	static constexpr float kDefaultCellSize = 128.0f;

	explicit SpatialHash(float fCellSize = kDefaultCellSize);

	void Clear();

	void Insert(Handle hEntity, const Rect& rcBounds);

	void Query(const Rect& rcArea, std::vector<Handle>& vecOut) const;


private:
	static std::int64_t MakeKey(int iCellX, int iCellY);
	int ToCell(float fCoord) const;	

	float m_fCellSize=kDefaultCellSize;
	std::unordered_map<std::int64_t, std::vector<Handle>> m_mapCells;
};

