
# Default command: configure and build project.
default: configure build

# Install deps and configure project in build folder.
configure *args:
	mkdir -p build
	conan install . -if build --build=missing -pr:h=default -pr:b=default {{args}}
	conan build -c . -bf build

# Build project.
build:
	cmake --build build

# Deploy package in conan cache.
deploy:
    conan create .

# Install docs dependencies.
configure-docs:
    pip install -r requirements.txt

# Build docs.
docs:
    make -C docs html

# Clean
clean:
	rm -rf build/*
