# Cold c++ library

## Setup project repository

```sh
git clone https://github.com/gviolator/libcold.git ./libcold
cd libcold
git submodule update --init
```

## Development setup

Stand alone build.

Run 
```sh
cd build
pkg_configure_x86_64-Debug.cmd
```

to setup Debug configuration build or

```sh
cd build
pkg_configure_x86_64-Release.cmd
```

to configure Release (RelWithDebInfo) build.

Visual studio target projects will be located at 'build/.target/v142-Debug' or 'build/.target/v142-Release' directory.

## Create conan package in local cache.

```sh
cd build
pkg_create.cmd
```

## Package editable mode

At development time there is need to work with libraried at source level without creating actuall package.
For that purposes package must be enabled for **editable** mode.

**To verify current package version** always check:
```sh
build/pkg_version.cmd
```

```sh
@set PKG_NAME=libcold
@set PKG_VERSION=0.0.1
@set PKG_CHANNEL=XYZ/local
```

To enable package for editable mode (for version specified in pkg_version) run:
```sh
cd build
pkg_editable_enable.cmd
```

To disable package editable mode (for version in pkg_version specified) run:
```sh
cd build
pkg_disable_disable.cmd
```

## Setup samples projects.
To configure debug version of the sample projects run:
```sh
samples/configure_x86_64-Debug.cmd
```

To configure release version of the sample projects run:
```sh
samples/configure_x86_64-Release.cmd
```

Visual studio target projects will be located within 'samples/.target/v142-Debug' or 'samples/.target/v142-Release' directory.