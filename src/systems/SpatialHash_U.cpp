#include "systems/SpatialHash_U.h"
#include <algorithm>                     
#include <cmath>                         

SpatialHash::SpatialHash(float fCellSize)
	: m_fCellSize(fCellSize)
{

}
int SpatialHash::ToCell(float fCoord) const
{
	return static_cast<int>(std::floor(fCoord / m_fCellSize));
}
std::int64_t SpatialHash::MakeKey(int iCellX, int iCellY)
{
	return (static_cast<std::int64_t>(iCellX) << 32) | static_cast<std::uint32_t>(iCellY);
}

void SpatialHash::Clear()
{
	for(auto& [llKey, vecHandles] : m_mapCells)
	{
		vecHandles.clear();
	}
}

void SpatialHash::Insert(Handle hEntity, const Rect& rcBounds)
{
	int iMinX = ToCell(rcBounds.fX);
	int iMinY = ToCell(rcBounds.fY);
	int iMaxX = ToCell(rcBounds.fX + rcBounds.fW);
	int iMaxY = ToCell(rcBounds.fY + rcBounds.fH);
	for(int iY = iMinY; iY <= iMaxY; ++iY)
	{
		for(int iX = iMinX; iX <= iMaxX; ++iX)
		{
			std::int64_t llKey = MakeKey(iX, iY);
			m_mapCells[llKey].push_back(hEntity);
		}
	}
}

void SpatialHash::Query(const Rect& rcArea, std::vector<Handle>& vecOut) const
{
	vecOut.clear();
	int iMinX = ToCell(rcArea.fX);
	int iMinY = ToCell(rcArea.fY);
	int iMaxX = ToCell(rcArea.fX + rcArea.fW);
	int iMaxY = ToCell(rcArea.fY + rcArea.fH);
	for(int iY = iMinY; iY <= iMaxY; ++iY)
	{
		for(int iX = iMinX; iX <= iMaxX; ++iX)
		{
			std::int64_t llKey = MakeKey(iX, iY);
			auto it = m_mapCells.find(llKey);
			if(it != m_mapCells.end())
			{
				const std::vector<Handle>& vecHandles = it->second;
				vecOut.insert(vecOut.end(), vecHandles.begin(), vecHandles.end());
			}
		}
	}
	std::sort(vecOut.begin(), vecOut.end(), [](const Handle& a, const Handle& b) {
		return a.uIndex < b.uIndex || (a.uIndex == b.uIndex && a.uGen < b.uGen);
	});
	vecOut.erase(std::unique(vecOut.begin(), vecOut.end()), vecOut.end());
}

