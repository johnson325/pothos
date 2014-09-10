// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include "WidgetUtils.hpp"
#include <cmath>
#include <complex>
#include <valarray>
#include <string>
#include <functional>
#include <cassert>

////////////////////////////////////////////////////////////////////////
//Window function support:
//https://en.wikipedia.org/wiki/Window_function
////////////////////////////////////////////////////////////////////////
class POTHOS_WIDGET_UTILS_EXPORT WindowFunction
{
public:

    WindowFunction(void);

    //! Set the window function type (use common window function names)
    void setType(const std::string &name);

    /*!
     * Get the power of the window function
     */
    double power(void) const
    {
        return _power;
    }

    /*!
     * Get the window values (only update if length changed).
     */
    const std::valarray<float> &window(void) const
    {
        return _window;
    }

    /*!
     * Set to a new window size.
     */
    void setSize(const size_t length);

private:
    std::function<double(const size_t, const size_t)> _calc;
    double _power;
    std::valarray<float> _window;
    void reload(void);
};

////////////////////////////////////////////////////////////////////////
//FFT code can be found at:
//http://rosettacode.org/wiki/Fast_Fourier_transform
////////////////////////////////////////////////////////////////////////
typedef std::complex<float> Complex;
typedef std::valarray<Complex> CArray;

// Cooley–Tukey FFT (in-place)
inline void fft(CArray& x)
{
    const size_t N = x.size();
    if (N <= 1) return;

    // divide
    CArray even = x[std::slice(0, N/2, 2)];
    CArray  odd = x[std::slice(1, N/2, 2)];

    // conquer
    fft(even);
    fft(odd);

    // combine
    for (size_t k = 0; k < N/2; ++k)
    {
        Complex t = std::polar(1.0f, -2 * float(M_PI) * k / N) * odd[k];
        x[k    ] = even[k] + t;
        x[k+N/2] = even[k] - t;
    }
}

// inverse fft (in-place)
inline void ifft(CArray& x)
{
    // conjugate the complex numbers
    x = x.apply(std::conj);

    // forward fft
    fft( x );

    // conjugate the complex numbers again
    x = x.apply(std::conj);

    // scale the numbers
    x /= x.size();
}

////////////////////////////////////////////////////////////////////////
//FFT Power spectrum
////////////////////////////////////////////////////////////////////////
inline std::valarray<float> fftPowerSpectrum(CArray &fftBins, WindowFunction &windowFunction)
{
    //windowing
    const auto &window = windowFunction.window();
    assert(window.size() == fftBins.size());
    for (size_t n = 0; n < fftBins.size(); n++) fftBins[n] *= window[n];

    //take fft
    fft(fftBins);

    //power calculation
    std::valarray<float> powerBins(fftBins.size());
    for (size_t i = 0; i < fftBins.size(); i++)
    {
        powerBins[i] = std::norm(fftBins[i]);
        powerBins[i] = 10*std::log10(powerBins[i]);
        powerBins[i] -= 20*std::log10(fftBins.size());
        powerBins[i] -= 20*std::log10(windowFunction.power());
    }

    //bin reorder
    for (size_t i = 0; i < powerBins.size()/2; i++)
    {
        std::swap(powerBins[i], powerBins[i+powerBins.size()/2]);
    }

    return powerBins;
}
