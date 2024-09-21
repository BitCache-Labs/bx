#pragma once

/*
#include "Engine/Core/Math.hpp"
#include "Engine/Containers/List.hpp"

#include <unordered_set>

struct GridCell
{
    std::unordered_set<SizeType> indices;
};

template <typename T>
class GridMap
{
public:
    GridMap(f32 width, f32 height, f32 cellSize)
        : m_width(width)
        , m_height(height)
        , m_cellSize(cellSize)
    {
        m_numCols = static_cast<i32>(width / cellSize);
        m_numRows = static_cast<i32>(height / cellSize);
        m_cells.resize(numRows * numCols);
    }

    inline void Insert(const T& data, const Vec3& pos, const Box3& box)
    {
        i32 minX = static_cast<i32>(pos.x + box.min.x / m_cellSize);
        i32 maxX = static_cast<i32>(pos.x + box.max.x / m_cellSize);
        i32 minY = static_cast<i32>(pos.y + box.min.y / m_cellSize);
        i32 maxY = static_cast<i32>(pos.y + box.max.y / m_cellSize);
        i32 minZ = static_cast<i32>(pos.z + box.min.z / m_cellSize);
        i32 maxZ = static_cast<i32>(pos.z + box.max.z / m_cellSize);

        for (i32 z = minZ; z <= maxZ; ++z)
        {
            for (i32 y = minY; y <= maxY; ++y)
            {
                for (i32 x = minX; x <= maxX; ++x)
                {
                    m_data.emplace_back(data);
                    auto& cell = m_cells[GetCellIndex(x, y, z)];
                    cell.indices.insert(m_data.size() - 1);
                }
            }
        }
    }

    inline const List<T>& Query(const Box3& box) const
    {
        std::unordered_set<SizeType> result;

        i32 minX = static_cast<i32>(pos.x + box.min.x / m_cellSize);
        i32 maxX = static_cast<i32>(pos.x + box.max.x / m_cellSize);
        i32 minY = static_cast<i32>(pos.y + box.min.y / m_cellSize);
        i32 maxY = static_cast<i32>(pos.y + box.max.y / m_cellSize);
        i32 minZ = static_cast<i32>(pos.z + box.min.z / m_cellSize);
        i32 maxZ = static_cast<i32>(pos.z + box.max.z / m_cellSize);

        for (i32 z = minZ; z <= maxZ; ++z)
        {
            for (i32 y = minY; y <= maxY; ++y)
            {
                for (i32 x = minX; x <= maxX; ++x)
                {
                    const GridCell& cell = m_cells[GetCellIndex(x, y, z)];
                    for (SizeType index : cell.indices)
                    {
                        uniqueResults.insert(index);
                    }
                }
            }
        }

        m_query.assign(result.begin(), result.end());
        return m_query;
    }

private:
    f32 m_width = 0;
    f32 m_height = 0;
    f32 m_cellSize = 0;
    i32 m_numCols = 0;
    i32 m_numRows = 0;
    List<GridCell> m_cells;
    List<T> m_data;
    List<T> m_query;

    SizeType GetCellIndex(i32 x, i32 y, i32 z)
    {
        return m_cells[z * m_numCols * m_numRows + y * m_numCols + x];
    }
};
*/