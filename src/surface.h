#pragma once

#include <vector>

struct Surface
{
    std::vector<float> xyzArray;

    Surface(float (*f)(float, float),
            std::size_t VertexCountPerSide = 60)
    {
        const std::size_t CoordinateCount = VertexCountPerSide * VertexCountPerSide * 3;
        const float vertexStep = 2.0 / (VertexCountPerSide - 1);

        xyzArray.resize(CoordinateCount);

        for (std::size_t row = 0; row < VertexCountPerSide; ++row)
        {
            for (std::size_t column = 0; column < VertexCountPerSide; ++column)
            {
                const auto x = -1.0f + column * vertexStep;
                const auto y = -1.0f + row * vertexStep;
                const auto z = f(x, y);

                const auto vertexStartIndex = (row * VertexCountPerSide + column) * 3;

                xyzArray[vertexStartIndex + 0] = x;
                xyzArray[vertexStartIndex + 1] = y;
                xyzArray[vertexStartIndex + 2] = z;
            }
        }
    }
};