#include "FileUtility.h"
#include <fstream>

namespace Catherine
{
	size_t FileUtility::LoadFileContent(const char * param_Path, void ** param_Ptr)
	{
		std::ifstream tmp_stream(param_Path);

		if (tmp_stream.fail())
		{
			*param_Ptr = nullptr;
			return 0;
		}

		tmp_stream.seekg(0, tmp_stream.end);
		std::streamsize tmp_size = tmp_stream.tellg();
		tmp_stream.seekg(0, tmp_stream.beg);

		char * tmp_result = new char[tmp_size] { 0 };
		tmp_stream.read(tmp_result, tmp_size);
		tmp_stream.close();

		*param_Ptr = tmp_result;
		return static_cast<size_t>(tmp_size);
	}

	std::string FileUtility::GetDictionary(const std::string & path)
	{
		return path.substr(0, path.find_last_of('/'));
	}
}