# Commander

Commander is a command line tool interface library.

## Quick start

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

### Dependencies

`commander` require several dependencies such as `cmake`, `conan` and a C++ compiler.

> **NOTE:** [just](https://just.systems/) can be used instead of calling the following commands.

First, you will need to install conan and cmake using `pip`.

```bash
python -m pip install conan==1.59 cmake --user
```

> **NOTE:** `conan` 2 is out but we did not used it yet.

Then you need to create the default profile and add the cosmic repository that own commander. Simply run the following script:

```bash
tools/configure_conan.sh
```

### Deployment

Run:

```bash
conan create .
```

It will create `commander` package in your local conan cache.

### Usage

Create a `conanfile.py` or `conanfile.txt` and add `commander/0.1.0` dependency. See [conan documentation](https://docs.conan.io/en/latest/using_packages.html).

And run the command:

```bash
conan install . -if build
```

This command will to your build system `commander` recipe by generating files to let cmake know how to use it.

> **NOTE:** You can take a look at the `examples` directory to see how to use `commander` in a project.

## Build example

To build the example, you first need to deploy `commander` in your local cache. See [deployment](#deployment) section.

Then run:

```bash
cd examples
conan install . -if build
conan build . -bf build
```

## Build documentation

To build the documentation, you need to install `doxygen`, `breathe`, `Sphinx`, `sphinx-rtd-theme`, `sphinx-rtd-dark-mode`.

To install doxygen, refer to your OS package manager. For the rest, run:

```bash
pip install -r requirements.txt
```

To build the documentation, run:

```bash
cd docs
make html
```

It will produce the documentation in `build/docs/sphinx/html`.
