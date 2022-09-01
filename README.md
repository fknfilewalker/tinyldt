# Header only C++11 LDT loader.
A simple header only loader/writer for ldt files. Parses the file and writes its content to a struct or vice versa. 

## Usage
```c++
#include <tiny_ldt.hpp>

tiny_ldt<float>::light ldt;
// optional: use double precision 
// tiny_ldt<double>
std::string err;
std::string warn;
if (!tiny_ldt<float>::load_ldt(filepath, err, warn, ldt)) {
	// print loading failed
}
if (!err.empty()) // print error
if (!warn.empty()) // print warning

// write ltd to file
if (!tiny_ldt<float>::write_ldt("out.ldt", ldt, /*optional precision*/ 10)) {
	// print writing failed
}
```

![asd](image.png)

## Features
* [x] Load LDT
* [x] Save LDT
* [ ] Filter candela array data (e.g. resize)

[License (MIT)](https://github.com/fknfilewalker/tinyies/blob/main/LICENSE)