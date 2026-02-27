#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>
#include <iomanip>

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
    for (int i = 0; i < n; ++ i) {
        uint32_t v = (uint32_t)arr[i] ^ SIGN;
        ++ cnt[v & MASK];
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

static bool read_ints_from_file(const char* path, vector<int>& out)
{
    ifstream in(path);
    if (!in) return false;

    out.clear();
    out.reserve(1 << 20);

    int first;
    if (!(in >> first)) return false;

    vector<int> rest;
    rest.reserve(1 << 20);

    int x;
    while (in >> x) rest.push_back(x);

    // Supports both formats:
    // A) file contains: n then n numbers
    // B) file contains: numbers only
    if ((size_t)first == rest.size()) {
        out = std::move(rest); // treat 'first' as n
    } else {
        out.reserve(rest.size() + 1);
        out.push_back(first);              // treat 'first' as data
        out.insert(out.end(), rest.begin(), rest.end());
    }
    return true;
}

int main(int argc, char** argv)
{
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    if(argc < 2) {
        cerr << "Usage:\n"
             << "  ./sort in.txt\n"
             << "  ./sort in.txt stdout\n"
             << "  ./sort in.txt out.txt\n";
        return 1;
    }

    const char* input_path = argv[1];
    string out_dest = (argc >= 3) ? string(argv[2]) : "";

    vector<int> vec;
    if (!read_ints_from_file(input_path, vec)) {
        cerr << "Error: cannot read input file: " << input_path << "\n";
        return 1;
    }

    int n = (int)vec.size();
    int* arr = new int[n];
    uint32_t* tmp = new uint32_t[n];
    memcpy(arr, vec.data(), (size_t)n * sizeof(int));

    // 1) sorting time only
    auto t0 = chrono::high_resolution_clock::now();
    radix_sort_int32_2pass(arr, n, tmp);
    auto t1 = chrono::high_resolution_clock::now();

    // 2) sorting + output time (only if output requested)
    auto t2 = t1;
    if (!out_dest.empty()) {
        if (out_dest == "stdout") {
            for (int i = 0; i < n; ++i) cout << arr[i] << '\n';
            cout.flush();
        } else {
            ofstream out(out_dest);
            if (!out) {
                cerr << "Error: cannot open output file: " << out_dest << "\n";
                delete[] tmp;
                delete[] arr;
                return 1;
            }
            for (int i = 0; i < n; ++i) out << arr[i] << '\n';
            out.flush();
        }
        t2 = chrono::high_resolution_clock::now();
    }

    chrono::duration<double> sort_time = t1 - t0;
    chrono::duration<double> sort_plus_out_time = t2 - t0;

    cerr << fixed << setprecision(6);
    cerr << "Elements: " << n << "\n";
    cerr << "Sort time only: " << sort_time.count() << " s\n";
    cerr << "Sort + output time: " << sort_plus_out_time.count() << " s\n";

    long long bytes_arr = 1LL * n * (long long)sizeof(int);
    long long bytes_tmp = 1LL * n * (long long)sizeof(uint32_t);
    cerr << "Memory (arr): " << (bytes_arr / (1024.0 * 1024.0)) << " MB\n";
    cerr << "Memory (arr + tmp): " << ((bytes_arr + bytes_tmp) / (1024.0 * 1024.0)) << " MB\n";

    delete[] tmp;
    delete[] arr;
    return 0;
}