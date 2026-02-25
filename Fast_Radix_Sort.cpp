#include <bits/stdc++.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <chrono>
using namespace std;

// 2-pass radix sort for signed 32-bit int (16 bits + 16 bits)
static inline void radix_sort_int32_2pass(int* arr, int n, uint32_t* tmp)
{
    if (n <= 1) return;

    constexpr uint32_t SIGN = 0x80000000u;
    constexpr uint32_t RAD  = 1u << 16; // 65536
    constexpr uint32_t MASK = RAD - 1u;

    alignas(64) static uint32_t cnt[RAD];

    // PASS 0: histogram low 16
    memset(cnt, 0, sizeof(cnt));
    for (int i = 0; i < n; ++i) {
        uint32_t v = (uint32_t)arr[i] ^ SIGN;
        ++cnt[v & MASK];
    }

    // prefix sum
    uint32_t sum = 0;
    for (uint32_t k = 0; k < RAD; ++k) {
        uint32_t c = cnt[k];
        cnt[k] = sum;
        sum += c;
    }

    // scatter to tmp (store sign-flipped)
    for (int i = 0; i < n; ++i) {
        uint32_t v = (uint32_t)arr[i] ^ SIGN;
        tmp[cnt[v & MASK]++] = v;
    }

    // PASS 1: histogram high 16
    memset(cnt, 0, sizeof(cnt));
    for (int i = 0; i < n; ++i) ++cnt[tmp[i] >> 16];

    // prefix sum
    sum = 0;
    for (uint32_t k = 0; k < RAD; ++k) {
        uint32_t c = cnt[k];
        cnt[k] = sum;
        sum += c;
    }

    // scatter back to arr (restore sign)
    for (int i = 0; i < n; ++i) {
        uint32_t v = tmp[i];
        arr[cnt[v >> 16]++] = (int)(v ^ SIGN);
    }
}

int main() {
    srand(time(0));

    // int n = 100;
    // int n = 1000;
    // int n = 10000;
    // int n = 100000;
    // int n = 1000000;
    // int n = 10000000;
    int n = 100000000;

    int* arr = new int[n];
    uint32_t* tmp = new uint32_t[n];

    mt19937 rng(123456u);
    uniform_int_distribution<int> dist(numeric_limits<int>::min(), numeric_limits<int>::max());
    for (int i = n / 2 + 1; i < n; ++i) arr[i] = dist(rng);

    cout << "\nSorting " << n << " elements..." << endl;

    auto start = chrono::high_resolution_clock::now();

    ios_base::sync_with_stdio(false);
    radix_sort_int32_2pass(arr, n, tmp);

    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> time_taken = end - start;

    cout << fixed << setprecision(5);
    cout << "Time taken: " << time_taken.count() << " seconds" << endl;

    long long bytes_arr = (long long)n * sizeof(int);
    long long bytes_tmp = (long long)n * sizeof(uint32_t);
    double totalMB_arr = bytes_arr / (1024.0 * 1024.0);
    double totalMB_all = (bytes_arr + bytes_tmp) / (1024.0 * 1024.0);

    cout << "Memory used (arr only): " << totalMB_arr << " MB" << endl;
    cout << "Memory used (arr + tmp): " << totalMB_all << " MB" << endl;

    cout << endl;

    delete[] tmp;
    delete[] arr;

    return 0;
}