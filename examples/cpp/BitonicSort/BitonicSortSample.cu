#include "BitonicSortSample.h"
#include "../../../src/cpp/Point.h"
#include "../../../src/cuda/BitonicSort.h"

#include <cfloat>
#include <chrono>
#include <iostream>
#include <random>

#define BLOCK_SIZE 32 // ToDo

SignalScatter::BitonicSortSample::BitonicSortSample(uint32_t size)
{
    std::random_device seed_gen;
    std::default_random_engine engine(seed_gen());
    std::uniform_real_distribution<float> dist(-256, 256);

    _size = size;

    _distanceMatrix = new float[size * size];
    _distanceMatrixMemorySize = size * size * sizeof(float);

    _packedIdMatrix = new uint32_t[size * size];
    _packedIdMatrixMemorySize = size * size * sizeof(uint32_t);

    _points = new Point[size];
    _pointListMemorySize = size * sizeof(Point);

    for (int i = 0; i < size; i++)
    {
        _points[i].Id = i;
        _points[i].PositionX = dist(engine);
        _points[i].PositionY = dist(engine);
        _points[i].PositionZ = dist(engine);
    }

    std::cout << "--------------------" << std::endl;
    std::cout << "Bitonic Sort Sample" << std::endl;
    std::cout << "--------------------" << std::endl;
    std::cout << "----------" << std::endl;
    std::cout << "N: " << _size << std::endl;
    std::cout << "N x N: " << (_size * _size) << std::endl;
    std::cout << "PointListMemorySize: " << _pointListMemorySize << " [Bytes]" << std::endl;
    std::cout << "DistanceMatrixMemorySize: " << _distanceMatrixMemorySize << " [Bytes]" << std::endl;
    std::cout << "PackedIdMatrixMemorySize: " << _packedIdMatrixMemorySize << " [Bytes]" << std::endl;
    std::cout << "----------" << std::endl;

    cudaError_t err;

	err = cudaMalloc((void **)&_d_PointList, _pointListMemorySize);
    std::cout << "CudaMalloc: " << err << std::endl;

	err = cudaMalloc((void **)&_d_DistanceMatrix, _distanceMatrixMemorySize);
    std::cout << "CudaMalloc: " << err << std::endl;

	err = cudaMalloc((void **)&_d_PackedIdMatrix, _packedIdMatrixMemorySize);
    std::cout << "CudaMalloc: " << err << std::endl;

	err = cudaMalloc((void **)&_d_DistanceMatrixOut, _distanceMatrixMemorySize);
    std::cout << "CudaMalloc: " << err << std::endl;

	err = cudaMalloc((void **)&_d_PackedIdMatrixOut, _packedIdMatrixMemorySize);
    std::cout << "CudaMalloc: " << err << std::endl;
}

SignalScatter::BitonicSortSample::~BitonicSortSample()
{
    cudaError_t err;

    err = cudaFree(_d_PointList);
    std::cout << "CudaFree: " << err << std::endl;

    err = cudaFree(_d_DistanceMatrix);
    std::cout << "CudaFree: " << err << std::endl;

    err = cudaFree(_d_PackedIdMatrix);
    std::cout << "CudaFree: " << err << std::endl;

    err = cudaFree(_d_DistanceMatrixOut);
    std::cout << "CudaFree: " << err << std::endl;

    err = cudaFree(_d_PackedIdMatrixOut);
    std::cout << "CudaFree: " << err << std::endl;
}

__global__ void CalculateDistanceKernel(int n, SignalScatter::Point *points, 
                                        float *distanceMatrix, uint32_t *packedIdMatrix)
{
	unsigned int col = threadIdx.x + blockIdx.x * blockDim.x;
	unsigned int row = threadIdx.y + blockIdx.y * blockDim.y;

	unsigned int a_index = row;
	unsigned int b_index = col;
	unsigned int c_index = row * n + col;

	if (row >= n || col >= n)
	{
		return;
	}

    float dx = points[a_index].PositionX - points[b_index].PositionX;
    float dy = points[a_index].PositionY - points[b_index].PositionY;
    float dz = points[a_index].PositionZ - points[b_index].PositionZ;

	distanceMatrix[c_index] = dx * dx + dy * dy + dz * dz; // Squared Distance
    packedIdMatrix[c_index] = (uint)((a_index & 0xFFFF) << 16 | b_index);
}

void CalculateDistance(int n, SignalScatter::Point *points, 
                       float *distanceMatrix, uint32_t *packedIdMatrix)
{
	int blockWidth  = BLOCK_SIZE;
	int blockHeight = BLOCK_SIZE;
	int gridWidth   = ceil((float)n/blockWidth);
	int gridHeight  = ceil((float)n/blockHeight);

    std::cout << "----------" << std::endl;
    std::cout << "gridWidth: "  << gridWidth << std::endl;
    std::cout << "gridHeight: " << gridHeight << std::endl;
    std::cout << "blockWidth: "  << blockWidth << std::endl;
    std::cout << "blockHeight: " << blockHeight << std::endl;
    std::cout << "----------" << std::endl;

	dim3 blockSize(blockWidth, blockHeight, 1);
	dim3 gridSize(gridWidth, gridHeight, 1);

    CalculateDistanceKernel<<<gridSize, blockSize>>>(n, points, distanceMatrix, packedIdMatrix);

	cudaError_t err = cudaGetLastError();

    std::cout << "----------" << std::endl;
    std::cout << "CalculateDistance: " << err << std::endl;
    std::cout << "----------" << std::endl;
}

void PrintDistanceMatrix(std::string title, uint32_t size, float *distanceMatrix, uint32_t *packedIdMatrix)
{
    std::cout << "-----" << std::endl;
    std::cout << title << std::endl;
    std::cout << "-----" << std::endl;
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            int pi = packedIdMatrix[i*size + j] & 0xFFFF0000 >> 16;
            int pj = packedIdMatrix[i*size + j] & 0xFFFF;
            std::cout << "D[" << i << "][" << j << "]: " << distanceMatrix[i*size + j] << " (" << pi << ", " << pj << ")" << std::endl;
        }
        std::cout << "-----" << std::endl;
    }
    std::cout << "-----" << std::endl;
}

void SignalScatter::BitonicSortSample::Run()
{
    cudaError_t err;
    std::chrono::system_clock::time_point start, end;
    double elapsedTimeMilliseconds;

    std::cout << "======================================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "----------" << std::endl;
    std::cout << "Distance Calculation on Accelerator (using CUDA)" << std::endl;
    std::cout << "----------" << std::endl;

	err = cudaMemcpy(_d_PointList, _points, _pointListMemorySize, cudaMemcpyHostToDevice);
    std::cout << "CudaMemcpyHostToDevice: " << err << std::endl;

    start = std::chrono::system_clock::now();

    std::cout << "Start CalculateDistance" << std::endl;
    CalculateDistance(_size, _d_PointList, _d_DistanceMatrix, _d_PackedIdMatrix);

    end = std::chrono::system_clock::now();
    elapsedTimeMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "----------" << std::endl;
    std::cout << "Elapsed time: " << elapsedTimeMilliseconds << " [ms]" << std::endl;
    std::cout << "----------" << std::endl;

    std::cout << "----------" << std::endl;
    std::cout << "Bitonic Sort on Accelerator (using CUDA)" << std::endl;
    std::cout << "----------" << std::endl;

	err = cudaMemcpy(_distanceMatrix, _d_DistanceMatrix, _distanceMatrixMemorySize, cudaMemcpyDeviceToHost);
    std::cout << "CudaMemcpyDeviceToHost: " << err << std::endl;

	err = cudaMemcpy(_packedIdMatrix, _d_PackedIdMatrix, _packedIdMatrixMemorySize, cudaMemcpyDeviceToHost);
    std::cout << "CudaMemcpyDeviceToHost: " << err << std::endl;

    PrintDistanceMatrix("Before BitonicSort", _size, _distanceMatrix, _packedIdMatrix);

    start = std::chrono::system_clock::now();

    uint ascending = 1;
    uint batchSize = _size;
    uint arrayLength = _size;

    std::cout << "Start BitonicSort" << std::endl;
    bitonicSort(_d_DistanceMatrixOut, _d_PackedIdMatrixOut, _d_DistanceMatrix, _d_PackedIdMatrix, batchSize, arrayLength, ascending);

    end = std::chrono::system_clock::now();
    elapsedTimeMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "----------" << std::endl;
    std::cout << "Elapsed time: " << elapsedTimeMilliseconds << " [ms]" << std::endl;
    std::cout << "----------" << std::endl;

    std::cout << "----------" << std::endl;
    std::cout << "Memory Copy (Device -> Host)" << std::endl;
    std::cout << "----------" << std::endl;

    start = std::chrono::system_clock::now();

	err = cudaMemcpy(_distanceMatrix, _d_DistanceMatrixOut, _distanceMatrixMemorySize, cudaMemcpyDeviceToHost);
    std::cout << "CudaMemcpyDeviceToHost: " << err << std::endl;

	err = cudaMemcpy(_packedIdMatrix, _d_PackedIdMatrixOut, _packedIdMatrixMemorySize, cudaMemcpyDeviceToHost);
    std::cout << "CudaMemcpyDeviceToHost: " << err << std::endl;

    end = std::chrono::system_clock::now();
    elapsedTimeMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "----------" << std::endl;
    std::cout << "Elapsed time: " << elapsedTimeMilliseconds << " [ms]" << std::endl;
    std::cout << "----------" << std::endl;

    PrintDistanceMatrix("After BitonicSort", _size, _distanceMatrix, _packedIdMatrix);

    std::cout << "======================================================================" << std::endl;
}