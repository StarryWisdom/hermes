mkdir build
cd build
conan install -s build_type=Release ..
cd ../libs
git clone https://github.com/google/googletest.git
git submodule init
git submodule update
