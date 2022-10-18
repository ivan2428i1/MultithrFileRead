#pragma once
#include <regex>
#include <future>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

struct EntryWithLine
{
    EntryWithLine(const std::string& e, const int num)
    {
        this->entry = e;
        this->relative_l_num = num;
    }
    std::string entry; // entry will be used in final output
    int relative_l_num; // line number for corresponding thread (relative, not absolute)
};


class Scanner
{
    const std::string _filename;
    const std::regex _mask;
    int _total_lines_scanned; // how many lines have this scanned scanned totally

    void ScanOneLine(const int n, const std::string& line,
                     std::vector<EntryWithLine>* pV);
    std::vector<EntryWithLine> MultiLineScan(const std::string& filename, int start, int stop);
public:
    Scanner(const std::string& fname, const std::string& mask);
    [[nodiscard]]
    std::future<std::vector<EntryWithLine>> Run(int start_byte, int end_byte);
    int TotalLinesScanned() const;
};
