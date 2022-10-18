#include "scanner.h"

void Scanner::ScanOneLine(const int n, const std::string &line, std::vector<EntryWithLine> *pV)
{
    std::smatch m;
    auto& v= *pV; // alias
    int ind_in_line {1}; // we need to output match's index from the start of the line, even if there are several matches
    auto searchStart( line.cbegin() );
    while (std::regex_search(searchStart, line.cend(), m, _mask))
    { // Need to create a descriptive string:
        ind_in_line += m.position(0);
        v.emplace_back(std::to_string(ind_in_line) + " " + // position
                       m[0].str(), //  // match case
                n /* relative line number*/);
        searchStart = m.suffix().first;
        ind_in_line += m[0].str().size();
    }
}

std::vector<EntryWithLine> Scanner::MultiLineScan(const std::string &filename, int start, int stop)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Unable to open file " << filename << std::endl;
        return std::vector<EntryWithLine>();
    }
    std::vector<EntryWithLine> v;
    file.seekg(start, std::ifstream::beg);
    std::string line;
    int line_num {0};
    if (start != 0)
        std::getline(file, line); // we've already scanned this line before
    while (std::getline(file, line))
    {
        _total_lines_scanned++;
        ScanOneLine(++line_num, line, &v);
        if (file.tellg() >= stop)
            break;
    }
    file.close();

    return v;
}

Scanner::Scanner(const std::string &fname, const std::string &mask):
    _filename(fname),
    _mask(mask),
    _total_lines_scanned(0)
{
}

std::future<std::vector<EntryWithLine> > Scanner::Run(int start_byte, int end_byte)
{
    return std::async(std::launch::async, &Scanner::MultiLineScan, this, _filename, start_byte, end_byte);
}

int Scanner::TotalLinesScanned() const { return _total_lines_scanned; }
