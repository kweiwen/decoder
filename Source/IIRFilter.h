#pragma once

#include <cstddef>
#include <stdint.h>
#include <iostream>

template <typename T>
class IIRFilter
{
public:
    IIRFilter() 
    {
        lenB = 0;
        lenA = 0;
        i_b = 0;
        i_a = 0;
        filtered = 0;
    }

    ~IIRFilter() {}

    std::vector<T> coeff_b;
    std::vector<T> coeff_a;
    void init(std::vector<T> b, std::vector<T> _a);
    T process_sample(T value);
    void process(T* input, T* output, int block_size);
    T filtered;

private:
    uint8_t lenB, lenA;
    uint8_t i_b;
    uint8_t i_a;
    std::vector<T> x;
    std::vector<T> y;
};

template <typename T>
void IIRFilter<T>::init(std::vector<T> b, std::vector<T> _a)
{
    // create DF-1 structure
    i_b = 0;
    i_a = 0;

    lenB = b.size();
    lenA = _a.size();
        
    x = std::vector<T>(lenB, 0);
    y = std::vector<T>(lenA, 0);

    coeff_b = std::vector<T>(2 * lenB - 1, 0);
    coeff_a = std::vector<T>(2 * lenA - 1, 0);

    // normalize
    double a0 = _a[0];
    const double* a = &_a[1];
    for (uint8_t i = 0; i < 2 * lenB - 1; i++)
    {
        coeff_b[i] = b[(2 * lenB - 1 - i) % lenB] / a0;
    }
    for (uint8_t i = 0; i < 2 * lenA - 1; i++)
    {
        coeff_a[i] = a[(2 * lenA - 2 - i) % lenA] / a0;
    }
}

template <typename T>
T IIRFilter<T>::process_sample(T value)
{
    x[i_b] = value;

    // moving average
    T b_terms = 0;
    for (size_t i = 0; i < lenB; i++)
    {
        b_terms += x[i] * coeff_b[lenB - i_b - 1 + i];
    }
    
    // auto regressive
    T a_terms = 0;
    for (size_t i = 0; i < lenA; i++)
    {
        a_terms += y[i] * coeff_a[lenA - i_a - 1 + i];
    }
    
    filtered = b_terms - a_terms;
    y[i_a] = filtered;
    
    i_b++;
    if (i_b == lenB)
    {
        i_b = 0;
    }
    
    i_a++;
    if (i_a == lenA)
    {
        i_a = 0;
    }
    
    return filtered;
}

template <typename T>
void IIRFilter<T>::process(T* input, T* output, int block_size)
{
    for (int i = 0; i < block_size; i++)
    {
        output[i] = process_sample(input[i]);
    }
}

template <typename T>
class CoupledAllPass
{
public:
    CoupledAllPass() 
    {
        neg = 0;
        pos = 0;
    }

    ~CoupledAllPass() {}

    T pos;
    T neg;

    void init(std::vector<T> d1, std::vector<T> d2);
    void process_sample(T input);

    IIRFilter<T> H1;
    IIRFilter<T> H2;
};

template <typename T>
void CoupledAllPass<T>::init(std::vector<T> d1, std::vector<T> d2)
{
    std::vector<T> d1_(d1.size());
    std::reverse_copy(std::begin(d1), std::end(d1), std::begin(d1_));
    H1.init(d1_, d1);

    std::vector<T> d2_(d2.size());
    std::reverse_copy(std::begin(d2), std::end(d2), std::begin(d2_));
    H2.init(d2_, d2);
}

template <typename T>
void CoupledAllPass<T>::process_sample(T input)
{
    T A1 = H1.process_sample(input);
    T A2 = H2.process_sample(input);
    pos = (A1 + A2) * 0.5;
    neg = (A1 - A2) * 0.5;
}


template <typename T>
class StereoIIRFilter
{
public:
    StereoIIRFilter() {}
    ~StereoIIRFilter() {}

    void init(std::vector<T> b, std::vector<T> a);
    void process_sample(T inputL, T inputR);

    IIRFilter<T> L;
    IIRFilter<T> R;
};

template <typename T>
void StereoIIRFilter<T>::init(std::vector<T> b, std::vector<T> a)
{
    L.init(b, a);
    R.init(b, a);
}

template <typename T>
void StereoIIRFilter<T>::process_sample(T inputL, T inputR)
{
    L.filtered = L.process_sample(inputL);
    R.filtered = R.process_sample(inputR);
}


template <typename T>
class StereoCoupledAllPass
{
public:
    StereoCoupledAllPass() {}
    ~StereoCoupledAllPass() {}

    void init(std::vector<T> d1, std::vector<T> d2);
    void process_sample(T inputL, T inputR);

    CoupledAllPass<T> L;
    CoupledAllPass<T> R;
};

template <typename T>
void StereoCoupledAllPass<T>::init(std::vector<T> d1, std::vector<T> d2)
{
    L.init(d1, d2);
    R.init(d1, d2);
}

template <typename T>
void StereoCoupledAllPass<T>::process_sample(T inputL, T inputR)
{
    L.process_sample(inputL);
    R.process_sample(inputR);
}