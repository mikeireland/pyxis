# Commander

Commander is a command line tool interface library.

## Simple usage

Let's start by creating command bindings for an extremely simple function, which
adds two numbers and returns their result:

```c++
int add(int i, int j) {
    return i + j;
}
```

For simplicity we'll put both this function and the binding code into
a file named `example.cpp` with the following contents:

```cpp
#include <commander/commander.h>

int add(int i, int j) {
    return i + j;
}

COMMANDER_REGISTER(m) {
    m.def("add", &add, "A function that adds two numbers");
}
```

Now, all you need is to compile the source file and run the executable with for instance the
`commander::Server`:

```cpp

int main(int argc, char* argv[]) {
    commander::Server(argc, argv).run();
}
```

and use it as follow:

```bash
./example --command add [2,3]
5
```

## Installation

### Quick installation

Commander is available as a conan package.

First, you will need to install conan using `pip`.

```bash
python -m pip install conan --user
```

Then you need to create the default profile and add the cosmic repository that own commander. Simply run the following script:

```bash
tools/configure_conan.sh
```

Then create a `conanfile.py` or `conanfile.txt` and add `commander/0.1.0` dependency. See [conan documentation](https://docs.conan.io/en/latest/using_packages.html).

And run the command:

```bash
conan install . -if build
```

This command will download commander, build it into the conan cache if missing and will generate files to let cmake know how to use it.

You can find a consumer example in `examples` directory. You can build it doing:

```bash
cd examples
./configure.sh
./build.sh
```

`configure.sh` will download and build dependencies if needed into your local conan cache and generate cmake configuration files.

### Create package from source.

Run:

```bash
conan create .
```

It will create `commander` package in your local conan cache.

