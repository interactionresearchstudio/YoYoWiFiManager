#ifndef YoYoFS_h
#define YoYoFS_h

#include <FS.h>

class YoYoFS : public fs::FS {
    public:
        //FS(FSImplPtr impl) : _impl(impl) { }

        File open(const char* path, const char* mode = FILE_READ, const bool create = false);
        File open(const String& path, const char* mode = FILE_READ, const bool create = false);

        bool exists(const char* path);
        bool exists(const String& path);

        bool remove(const char* path);
        bool remove(const String& path);

        bool rename(const char* pathFrom, const char* pathTo);
        bool rename(const String& pathFrom, const String& pathTo);

        bool mkdir(const char *path);
        bool mkdir(const String &path);

        bool rmdir(const char *path);
        bool rmdir(const String &path);
    private:
};

#endif