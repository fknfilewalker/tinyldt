# Header only C++11 LDT loader.
A simple header only loader for ldt files. Parses the file and writes its content to a struct. 

## Usage
```c++
#include <tiny_ldt.hpp>

tiny_ldt::light ldt;
std::string err;
std::string warn;
if (!tiny_ldt::load_ldt(filepath, err, warn, ldt)) {
	// print loading failed
}
if (!err.empty()) // print error
if (!warn.empty()) // print warning
```

[License (MIT)](https://github.com/fknfilewalker/tinyies/blob/main/LICENSE)