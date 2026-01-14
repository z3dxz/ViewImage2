#include "headers/leftrightlogic.hpp"

#include <shlwapi.h>
#include <vector>
#include <cctype>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <optional>

std::vector<std::string> kvector;
int currentIndex;
std::string indexedDir = "";

void clear_kvector(){
	kvector.clear();
}

void to_lower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

std::vector<std::string> SortFileVector(const std::string& referencePath) {

    const std::vector<std::string> extensions = {
        ".svg",
        ".m45",
        ".irbo",
		".sfbb",
		".jpg",
		".png",
		".bmp",
		".gif"
    };

    std::vector<std::string> result;

    std::filesystem::path refPath(referencePath);
    std::filesystem::path directory = refPath.parent_path();
    if (directory.empty())
		return {};

    for (const std::filesystem::directory_entry& entry :  std::filesystem::directory_iterator(directory))
    {
        if (!entry.is_regular_file())
            continue;

        std::string ext = entry.path().extension().string();
		to_lower(ext);

		// check right extension
        if (std::find(extensions.begin(), extensions.end(),  ext) != extensions.end()) {
            result.push_back(entry.path().string());
        }
    }

    std::sort(result.begin(), result.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) {
        return a.filename().string() < b.filename().string();
    });

    return result;
}

std::size_t FindIndexFromPath(const std::vector<std::string>& files, const std::string& referencePath) {
    const std::filesystem::path ref(referencePath);
    const std::filesystem::path refFilename = ref.filename();

    int index = 0;

    for (const std::string& file : files)
    {
        if (std::filesystem::path(file).filename() == refFilename)
            return index;

        index++;
    }

    return -1;
}

bool index(GlobalParams* m){
	std::string refpath = m->fpath;

	kvector = SortFileVector(refpath);
	if(kvector.empty()) {
		return false;
	}

	currentIndex = FindIndexFromPath(kvector, refpath);
	if(currentIndex == -1) {
		return false;
	}
	
    std::filesystem::path r(refpath); std::filesystem::path directory = r.parent_path();
	indexedDir = directory.string();
	return true;
}

void process_on(GlobalParams *m) {
	m->halt = true;
	m->loading = true;
	RedrawSurface(m);
}

void process_off(GlobalParams *m){
	m->halt = false;
	m->loading = false;
	RedrawSurface(m);
}

enum Result {
	NotIndexed,
	NotReady,
	EndOfArray,
	ImageFailed,
	Success
};

Result Inc(GlobalParams *m, int incremental){
	if(indexedDir == "") { return NotIndexed; }

	if (m->loading || m->halt || m->fpath == "") {
		return NotReady;
	}

	int want = currentIndex+incremental;

	if(want < 0 || want >= kvector.size()) {
		return EndOfArray;
	}
	
	process_on(m);
	LoadImageResult loadimg = OpenImageFromPath(m, kvector[want], true);
	process_off(m);

	if(loadimg == LI_Failed) {
		currentIndex+= incremental;
		return ImageFailed;
	}
	if(loadimg == LI_Success) {
		currentIndex+= incremental;
	} else {
		return NotReady;
	}

	return Success;
}

enum IndexCheck {
	IC_Fail,
	IC_Success,
	IC_None,
};

IndexCheck indexcheck(GlobalParams* m){
	std::filesystem::path p(m->fpath);
	std::filesystem::path directory = p.parent_path();

	std::string str = indexedDir;
	std::string str2 = directory.string();

	if(str != str2 || str2 == "" || kvector.size()<1) {
		std::cout << "indexing...\n";
		return index(m) ? IC_Success : IC_Fail;
	} else {
		std::cout << "Indexing not needed : " << str << " : " << str2 << "\n";

	}
	return IC_None;
}

void Move(GlobalParams *m, int increment){

	if (m->fpath == "Untitled") {
		return;
	}

	if(indexcheck(m) == IC_Fail) {
		MessageBox(m->hwnd, "Directory failed to index", "Error", MB_ICONERROR);
		indexedDir = "";
		return;
	} 
	
	Result test = Inc(m, increment);

	switch(test) {
		case NotReady:
			return;
		case ImageFailed:
			return Move(m, increment);
		case NotIndexed:
			MessageBox(m->hwnd, "Directory is not indexed", "Error", MB_ICONERROR);
			break;
		case EndOfArray:
			Beep(590, 50);
			break;
		default:
			break;
	}
}

void GoLeft(GlobalParams *m) {
	Move(m, -1);
}

void GoRight(GlobalParams *m) {
	Move(m, 1);
}

/*

std::string GetPrevFilePath() {
	
	if (kvector.size() < 1) {
		return "No";
	}
	std::string k = kvector[kvector.size() - 1];
	kvector.pop_back();
	return k;
}
*/

// come back to fix the issue here with the file paths and crap

/*

std::string GetNextFilePath(const char* file_Path) {

	 std::string imagePath = std::string(file_Path);
	 std::string folderPath = imagePath.substr(0, imagePath.find_last_of("\\/"));
	 std::string currentFileName = imagePath.substr(imagePath.find_last_of("\\/") + 1);
	 bool foundCurrentFile = false;

	 WIN32_FIND_DATAA fileData;
	 HANDLE hFind;

	 std::string searchPath = folderPath + "\\*.*";
	 hFind = FindFirstFileA(searchPath.c_str(), &fileData);

	 if (hFind != INVALID_HANDLE_VALUE)
	 {
		 do
		 {
			 if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			 {
				 std::string fileName = fileData.cFileName;
				 std::string extension = fileName.substr(fileName.find_last_of(".") + 1);
				 std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c){ return std::tolower(c); });

				 if (extension == "svg" || extension == "m45" || extension == "irbo" || extension == "sfbb" || extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "bmp" || extension == "gif")
				 {
					 if (foundCurrentFile)
					 {
						 std::wstring currentFileNameW = std::wstring(currentFileName.begin(), currentFileName.end());
						 std::wstring fileNameW = std::wstring(fileName.begin(), fileName.end());
						 if (StrCmpLogicalW(fileNameW.c_str(), currentFileNameW.c_str()) > 0)
						 {
							 std::string nextImagePath = folderPath + "\\" + fileName;
							 FindClose(hFind);

							 kvector.push_back(std::string(file_Path));
							 return nextImagePath;
						 }
					 }
					 if (fileName == currentFileName)
					 {
						 foundCurrentFile = true;
					 }
				 }
			 }
			 
		 } while (FindNextFileA(hFind, &fileData) != 0);

		 FindClose(hFind);
	 }
	 return "No";
}
*/