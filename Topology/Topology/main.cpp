#include "Reader.hpp"
#include "Placer.hpp"
#include "Writer.hpp"
#include <iostream>
#include <chrono>

int main()
{
    FileData data;

    if (!read("input.txt", data)) {
        std::cerr << "The reading ended with an error\n";
        return 1;
    }

    auto start = std::chrono::steady_clock::now();

    const PlacementResult result = place(data);
    
    auto end = std::chrono::steady_clock::now();
    auto elasped = std::chrono::duration<double>(end - start);

    write(result, data, elasped);
    return 0;
}


