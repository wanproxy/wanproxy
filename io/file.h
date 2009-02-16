#ifndef	FILE_H
#define	FILE_H

#include <io/file_descriptor.h>

class File : public FileDescriptor {
	File(int);
	~File();
public:
	static FileDescriptor *open(const std::string&, bool, bool);
};

#endif /* !FILE_H */
