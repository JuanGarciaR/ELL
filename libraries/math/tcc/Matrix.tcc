////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     Matrix.tcc (math)
//  Authors:  Ofer Dekel
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Exception.h"
#include "Operations.h"

// utilities
#include "Debug.h"
#include "Exception.h"

// stl
#include <algorithm> // for std::generate

namespace ell
{
namespace math
{
    //
    // RectangularMatrixBase
    //

    template <typename ElementType>
    RectangularMatrixBase<ElementType>::RectangularMatrixBase(size_t numRows, size_t numColumns, size_t increment, ElementType* pData)
        : _numRows(numRows), _numColumns(numColumns), _increment(increment), _pData(pData)
    {
    }

    template <typename ElementType>
    void RectangularMatrixBase<ElementType>::Swap(RectangularMatrixBase<ElementType>& other)
    {
        std::swap(_pData, other._pData);
        std::swap(_numRows, other._numRows);
        std::swap(_numColumns, other._numColumns);
        std::swap(_increment, other._increment);
    }

    //
    // MatrixBase
    //

    template <typename ElementType>
    MatrixBase<ElementType, MatrixLayout::rowMajor>::MatrixBase(size_t numRows, size_t numColumns, ElementType* pData)
        : RectangularMatrixBase<ElementType>(numRows, numColumns, numColumns, pData)
    {
    }

    template <typename ElementType>
    MatrixBase<ElementType, MatrixLayout::columnMajor>::MatrixBase(size_t numRows, size_t numColumns, ElementType* pData)
        : RectangularMatrixBase<ElementType>(numRows, numColumns, numRows, pData)
    {
    }

    //
    // ConstMatrixReference
    //
    template<typename ElementType, MatrixLayout layout>
    bool ConstMatrixReference<ElementType, layout>::IsContiguous() const
    {
        return (_increment == _intervalSize);
    }

    template <typename ElementType, MatrixLayout layout>
    ElementType ConstMatrixReference<ElementType, layout>::operator()(size_t rowIndex, size_t columnIndex) const
    {
        DEBUG_THROW(rowIndex >= _numRows || columnIndex >= _numColumns, utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange, "(rowIndex, columnIndex) exceeds matrix dimensions."));

        return _pData[rowIndex * _rowIncrement + columnIndex * _columnIncrement];
    }

    template <typename ElementType, MatrixLayout layout>
    auto ConstMatrixReference<ElementType, layout>::Transpose() const -> ConstMatrixReference<ElementType, TransposeMatrixLayout<layout>::value>
    {
        return ConstMatrixReference<ElementType, TransposeMatrixLayout<layout>::value>(_numColumns, _numRows, _increment, _pData);
    }

    template <typename ElementType, MatrixLayout layout>
    ConstMatrixReference<ElementType, layout> ConstMatrixReference<ElementType, layout>::GetSubMatrix(size_t firstRow, size_t firstColumn, size_t numRows, size_t numColumns) const
    {
        DEBUG_THROW(firstRow + numRows > _numRows || firstColumn + numColumns > _numColumns, utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange, "block exceeds matrix dimensions."));

        return ConstMatrixReference<ElementType, layout>(numRows, numColumns, _increment, _pData + firstRow * _rowIncrement + firstColumn * _columnIncrement);
    }

    template <typename ElementType, MatrixLayout layout>
    ConstVectorReference<ElementType, VectorOrientation::column> ConstMatrixReference<ElementType, layout>::GetColumn(size_t index) const
    {
        DEBUG_THROW(index >= _numColumns, utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange, "column index exceeds matrix dimensions."));

        return ConstVectorReference<ElementType, VectorOrientation::column>(_pData + index * _columnIncrement, _numRows, _rowIncrement);
    }

    template <typename ElementType, MatrixLayout layout>
    ConstVectorReference<ElementType, VectorOrientation::row> ConstMatrixReference<ElementType, layout>::GetRow(size_t index) const
    {
        DEBUG_THROW(index >= _numRows, utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange, "row index exceeds matrix dimensions."));

        return ConstVectorReference<ElementType, VectorOrientation::row>(_pData + index * _rowIncrement, _numColumns, _columnIncrement);
    }

    template <typename ElementType, MatrixLayout layout>
    ConstVectorReference<ElementType, VectorOrientation::column> ConstMatrixReference<ElementType, layout>::GetDiagonal() const
    {
        auto size = std::min(NumColumns(), NumRows());
        return ConstVectorReference<ElementType, VectorOrientation::column>(_pData, size, _increment + 1);
    }

    template<typename ElementType, MatrixLayout layout>
    ConstVectorReference<ElementType, VectorOrientation::column> ConstMatrixReference<ElementType, layout>::ReferenceAsVector() const
    {
        DEBUG_THROW(_increment != _intervalSize, utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange, "Can only flatten a matrix when its memory is contiguous"));

        return ConstVectorReference<ElementType, VectorOrientation::column>(_pData, _numRows * _numColumns, 1);
    }

    template <typename ElementType, MatrixLayout layout>
    bool ConstMatrixReference<ElementType, layout>::IsEqual(ConstMatrixReference<ElementType, layout> other, ElementType tolerance) const
    {
        if (NumRows() != other.NumRows() || NumColumns() != other.NumColumns())
        {
            return false;
        }

        for (size_t i = 0; i < NumIntervals(); ++i)
        {
            if (!GetMajorVector(i).IsEqual(other.GetMajorVector(i), tolerance))
            {
                return false;
            }
        }
        return true;
    }

    template <typename ElementType, MatrixLayout layout>
    bool ConstMatrixReference<ElementType, layout>::IsEqual(ConstMatrixReference<ElementType, TransposeMatrixLayout<layout>::value> other, ElementType tolerance) const
    {
        if (NumRows() != other.NumRows() || NumColumns() != other.NumColumns())
        {
            return false;
        }

        for (size_t i = 0; i < NumRows(); ++i)
        {
            if (!GetRow(i).IsEqual(other.GetRow(i), tolerance))
            {
                return false;
            }
        }
        return true;
    }

    template <typename ElementType, MatrixLayout layout>
    bool ConstMatrixReference<ElementType, layout>::operator==(const ConstMatrixReference<ElementType, layout>& other) const
    {
        return IsEqual(other);
    }

    template <typename ElementType, MatrixLayout layout>
    bool ConstMatrixReference<ElementType, layout>::operator==(const ConstMatrixReference<ElementType, TransposeMatrixLayout<layout>::value>& other) const
    {
        return IsEqual(other);
    }

    template <typename ElementType, MatrixLayout layout>
    template <MatrixLayout OtherLayout>
    bool ConstMatrixReference<ElementType, layout>::operator!=(const ConstMatrixReference<ElementType, OtherLayout>& other)
    {
        return !(*this == other);
    }

    template <typename ElementType, MatrixLayout layout>
    ElementType* ConstMatrixReference<ElementType, layout>::GetMajorVectorBegin(size_t index) const
    {
        return _pData + index * _increment;
    }

    //
    // MatrixReference
    //

    template <typename ElementType, MatrixLayout layout>
    MatrixReference<ElementType, layout>::MatrixReference(size_t numRows, size_t numColumns, ElementType* pData) :
        ConstMatrixReference<ElementType, layout>(numRows, numColumns, pData)
    {
    }

    template <typename ElementType, MatrixLayout layout>
    void MatrixReference<ElementType, layout>::Swap(MatrixReference<ElementType, layout>& other)
    {
        RectangularMatrixBase<ElementType>::Swap(other);
    }

    template <typename ElementType, MatrixLayout layout>
    ElementType& MatrixReference<ElementType, layout>::operator()(size_t rowIndex, size_t columnIndex)
    {
        DEBUG_THROW(rowIndex >= _numRows || columnIndex >= _numColumns, utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange, "(rowIndex, columnIndex) exceeds matrix dimensions."));

        return _pData[rowIndex * _rowIncrement + columnIndex * _columnIncrement];
    }

    template <typename ElementType, MatrixLayout layout>
    void MatrixReference<ElementType, layout>::Fill(ElementType value)
    {
        for (size_t i = 0; i < _numIntervals; ++i)
        {
            auto begin = GetMajorVectorBegin(i);
            std::fill(begin, begin + _intervalSize, value);
        }
    }

    template <typename ElementType, MatrixLayout layout>
    template <typename GeneratorType>
    void MatrixReference<ElementType, layout>::Generate(GeneratorType generator)
    {
        for (size_t i = 0; i < _numIntervals; ++i)
        {
            auto begin = GetMajorVectorBegin(i);
            std::generate(begin, begin + _intervalSize, generator);
        }
    }

    template <typename ElementType, MatrixLayout layout>
    MatrixReference<ElementType, layout> MatrixReference<ElementType, layout>::GetReference()
    {
        return MatrixReference<ElementType, layout>(_numRows, _numColumns, _increment, _pData);
    }

    template <typename ElementType, MatrixLayout layout>
    ConstMatrixReference<ElementType, layout> MatrixReference<ElementType, layout>::GetConstReference() const
    {
        return ConstMatrixReference<ElementType, layout>(_numRows, _numColumns, _increment, _pData);
    }

    template <typename ElementType, MatrixLayout layout>
    auto MatrixReference<ElementType, layout>::Transpose() const -> MatrixReference<ElementType, TransposeMatrixLayout<layout>::value>
    {
        return MatrixReference<ElementType, TransposeMatrixLayout<layout>::value>(_numColumns, _numRows, _increment, _pData);
    }

    template <typename ElementType, MatrixLayout layout>
    MatrixReference<ElementType, layout> MatrixReference<ElementType, layout>::GetSubMatrix(size_t firstRow, size_t firstColumn, size_t numRows, size_t numColumns)
    {
        DEBUG_THROW(firstRow + numRows > _numRows || firstColumn + numColumns > _numColumns, utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange, "block exceeds matrix dimensions."));

        return MatrixReference<ElementType, layout>(numRows, numColumns, _increment, _pData + firstRow * _rowIncrement + firstColumn * _columnIncrement);
    }

    template <typename ElementType, MatrixLayout layout>
    VectorReference<ElementType, VectorOrientation::column> MatrixReference<ElementType, layout>::GetColumn(size_t index)
    {
        DEBUG_THROW(index >= _numColumns, utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange, "column index exceeds matrix dimensions."));

        return VectorReference<ElementType, VectorOrientation::column>(_pData + index * _columnIncrement, _numRows, _rowIncrement);
    }

    template <typename ElementType, MatrixLayout layout>
    VectorReference<ElementType, VectorOrientation::row> MatrixReference<ElementType, layout>::GetRow(size_t index)
    {
        DEBUG_THROW(index >= _numRows, utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange, "row index exceeds matrix dimensions."));

        return VectorReference<ElementType, VectorOrientation::row>(_pData + index * _rowIncrement, _numColumns, _columnIncrement);
    }

    template <typename ElementType, MatrixLayout layout>
    VectorReference<ElementType, VectorOrientation::column> MatrixReference<ElementType, layout>::GetDiagonal()
    {
        auto size = std::min(NumColumns(), NumRows());
        return VectorReference<ElementType, VectorOrientation::column>(_pData, size, _increment + 1);
    }

    template<typename ElementType, MatrixLayout layout>
    VectorReference<ElementType, VectorOrientation::column> MatrixReference<ElementType, layout>::ReferenceAsVector()
    {
        DEBUG_THROW(_increment != _intervalSize, utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange, "Can only flatten a matrix when its memory is contiguous"));

        return VectorReference<ElementType, VectorOrientation::column>(_pData, _numRows * _numColumns, 1);
    }

    //
    // Matrix
    //

    template <typename ElementType, MatrixLayout layout>
    Matrix<ElementType, layout>::Matrix(size_t numRows, size_t numColumns)
        : MatrixReference<ElementType, layout>(numRows, numColumns, nullptr), _data(numRows * numColumns)
    {
        _pData = _data.data();
    }

    template <typename ElementType, MatrixLayout layout>
    Matrix<ElementType, layout>::Matrix(std::initializer_list<std::initializer_list<ElementType>> list)
        : MatrixReference<ElementType, layout>(list.size(), list.begin()->size(), nullptr), _data(list.size() * list.begin()->size())
    {
        _pData = _data.data();
        auto numColumns = list.begin()->size();

        size_t i = 0;
        for (auto rowIter = list.begin(); rowIter < list.end(); ++rowIter)
        {
            DEBUG_THROW(rowIter->size() != numColumns, utilities::InputException(utilities::InputExceptionErrors::sizeMismatch, "incorrect number of elements in initializer list"));

            size_t j = 0;
            for (auto elementIter = rowIter->begin(); elementIter < rowIter->end(); ++elementIter)
            {
                (*this)(i, j) = *elementIter;
                ++j;
            }
            ++i;
        }
    }

    template <typename ElementType, MatrixLayout layout>
    Matrix<ElementType, layout>::Matrix(size_t numRows, size_t numColumns, const std::vector<ElementType>& data)
        : MatrixReference<ElementType, layout>(numRows, numColumns, nullptr), _data(data)
    {
        _pData = _data.data();
    }

    template <typename ElementType, MatrixLayout layout>
    Matrix<ElementType, layout>::Matrix(size_t numRows, size_t numColumns, std::vector<ElementType>&& data)
        : MatrixReference<ElementType, layout>(numRows, numColumns, nullptr), _data(std::move(data))
    {
        _pData = _data.data();
    }

    template <typename ElementType, MatrixLayout layout>
    Matrix<ElementType, layout>::Matrix(Matrix<ElementType, layout>&& other)
        : MatrixReference<ElementType, layout>(other.NumRows(), other.NumColumns(), nullptr), _data(std::move(other._data))
    {
        _pData = _data.data();
    }

    template <typename ElementType, MatrixLayout layout>
    Matrix<ElementType, layout>::Matrix(const Matrix<ElementType, layout>& other)
        : MatrixReference<ElementType, layout>(other.NumRows(), other.NumColumns(), nullptr), _data(other._data)
    {
        _pData = _data.data();
    }

    template <typename ElementType, MatrixLayout layout>
    Matrix<ElementType, layout>::Matrix(ConstMatrixReference<ElementType, layout>& other)
        : MatrixReference<ElementType, layout>(other.NumRows(), other.NumColumns(), nullptr), _data(other.NumRows() * other.NumColumns())
    {
        _pData = _data.data();
        for (size_t i = 0; i < _numRows; ++i)
        {
            for (size_t j = 0; j < _numColumns; ++j)
            {
                (*this)(i, j) = other(i, j);
            }
        }
    }

    template <typename ElementType, MatrixLayout layout>
    Matrix<ElementType, layout>::Matrix(ConstMatrixReference<ElementType, TransposeMatrixLayout<layout>::value> other)
        : MatrixReference<ElementType, layout>(other.NumRows(), other.NumColumns(), nullptr), _data(other.NumRows() * other.NumColumns())
    {
        _pData = _data.data();
        for (size_t i = 0; i < _numRows; ++i)
        {
            for (size_t j = 0; j < _numColumns; ++j)
            {
                (*this)(i, j) = other(i, j);
            }
        }
    }

    template <typename ElementType, MatrixLayout layout>
    Matrix<ElementType, layout>& Matrix<ElementType, layout>::operator=(Matrix<ElementType, layout> other)
    {
        Swap(other);
        return *this;
    }

    template <typename ElementType, MatrixLayout layout>
    void Matrix<ElementType, layout>::Swap(Matrix<ElementType, layout>& other)
    {
        RectangularMatrixBase<ElementType>::Swap(other);
        std::swap(_data, other._data);
    }

    template <typename ElementType, MatrixLayout layout>
    void Matrix<ElementType, layout>::Fill(ElementType value)
    {
        std::fill(_data.begin(), _data.end(), value);
    }

    template <typename ElementType, MatrixLayout layout>
    template <typename GeneratorType>
    void Matrix<ElementType, layout>::Generate(GeneratorType generator)
    {
        std::generate(_data.begin(), _data.end(), generator);
    }
}
}