BINARYS=hermes.gcc hermes.release.gcc hermes.clang

all:${BINARYS}

.PHONY: hermes.gcc
hermes.gcc:
	mkdir -p build.hermes.gcc
	cd build.hermes.gcc && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j 4
	ln -fs build.hermes.gcc/hermes hermes.gcc

.PHONY: hermes.clang
hermes.clang:
	mkdir -p build.hermes.clang
	cd build.hermes.clang && export CXX=clang++-8; export CC=clang; cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j 4
	ln -fs build.hermes.clang/hermes hermes.clang

.PHONY: hermes.release.gcc
hermes.release.gcc:
	mkdir -p build.hermes.release.gcc
	cd build.hermes.release.gcc && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j 4
	ln -fs build.hermes.release.gcc/hermes hermes.release.gcc

.PHONY: scanbuild
scanbuild:
	rm -rf scanbuild
	mkdir -p scanbuild
	cd scanbuild && scan-build cmake -DCMAKE_BUILD_TYPE=Debug .. && scan-build make -j 4

lcov: hermes.gcc
	@./build.hermes.gcc/hermes.test
	@mkdir -p lcov
	@lcov -c -d . -o lcov/lcov.1
	@lcov -r lcov/lcov.1  /usr/include/\* \*libs/\* \*lua/\* \*test/\* \*src/entt/\* \*src/experimental/\* \*.test.cpp \*/build.hermes.gcc/\* \*.test.h -o lcov/lcov.2
	@genhtml -o lcov/lcov-html lcov/lcov.2

clean:
	rm -rf $(BINARYS) hermes.gcc hermes.clang hermes.release.gcc lcov build.hermes.gcc build.hermes.clang build.hermes.release.gcc scanbuild
