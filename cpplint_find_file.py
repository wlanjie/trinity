import os, fnmatch, sys

def all_files(root, patterns = '*', single_level = False, yield_folders=False):
    patterns = patterns.split(';')
    for path, subdirs, files in os.walk(root):
        if "stb" in path or "json" in path:
            continue
        if yield_folders:
            files.extend(subdirs)
        files.sort()
        for name in files:
            if "main" in name:
                continue
            for pattern in patterns:
                if fnmatch.fnmatch(name, pattern):
                    yield os.path.join(path,name)
                    break
        if single_level:
            break

if __name__ == '__main__':
    # if len(sys.argv) < 2:
    #     print 'Please set the absolute path as the first parameter for parse.'
    #     sys.exit()
    # print sys.argv[1]
    for path in all_files(sys.argv[1],'*.cc;*.h'):
        # print(path)
        os.system("python cpplint.py --linelength=120 --verbose=3 --filter=-build/header_guard,-build/include %s"%(path))  
