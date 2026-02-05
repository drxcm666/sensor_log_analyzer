#include "csv_split.hpp"
#include "util.hpp"


namespace sla{

SplitStatus split_csv (
    std::string_view s, 
    std::array<std::string_view, 4> &out, 
    std::size_t &actual_columns)
{
    actual_columns = 0;
    std::size_t start = 0;

    while (start <= s.size())
    {
        std::size_t pos = s.find(',', start);
        if (pos == std::string_view::npos)
            pos = s.size();
        
        std::string_view part =  trim(s.substr(start, pos - start));

        if (actual_columns < out.size())
        {
            out[actual_columns] = part;
        }
        
        start = pos + 1;
        actual_columns++;

        if (pos == s.size())
        {
            break;
        }
    }

    if (actual_columns < out.size()) return SplitStatus::TooFew;
    if (actual_columns > out.size()) return SplitStatus::TooMany;

    return SplitStatus::Ok;
} 

}