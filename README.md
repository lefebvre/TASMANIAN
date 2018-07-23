# TASMANIAN

The Toolkit for Adaptive Stochastic Modeling and Non-Intrusive ApproximatioN is a collection of robust libraries for high dimensional integration and interpolation as well as parameter calibration. The code consists of several modules that can be used individually or conjointly.

Visit us at: [http://tasmanian.ornl.gov/](http://tasmanian.ornl.gov/)

Sparse Grids
--------------

Sparse Grids is a family of algorithms for constructing multidimensional quadrature and interpolation rules from tensor products of one dimensional such rules. Tasmanian Sparse Grids Module implements a wide variety of one dimensional rules based on global and local function basis.

DREAM
--------------

DiffeRential Evolution Adaptive Metropolis (DREAM) is an algorithm for sampling from a general probability density when only the probability density function is known. The method can be applied to problems of Bayesian inference and optimization, including custom defined models as well as surrogate models constructed with the Tasmanian Sparse Grids module.

### Please cite us
If you use TASMANIAN for your research, pleace cite the Manual and our work on global and locally adaptive grids.

[http://tasmanian.ornl.gov/documents/Tasmanian.bib](http://tasmanian.ornl.gov/documents/Tasmanian.bib)

Quick Install
--------------

* the basic way: using GNU Make, `g++` and optionally `gfortran` and `/usr/bin/python`
```
  make
  make test
  make matlab   (optional: sets matlab work folder to ./tsgMatlabWorkFolder/)
  make python3  (optional: sets #!/usr/bin/env python3)
  make fortran  (optional: compile Fortran libraries)
  make examples
  make clean
```
* the easy way: using cmake and the `install` script
```
  ./install <install-path> <optional matlab work folder>
  ./install --help  (lists all options)
```
* the cmake way: see the top comment in CMakeLists.txt for available options
```
  mkdir Build
  cd Build
  cmake <options> <path-to-Tasmanian-source>
  make
  make test
  make install
  make test_install
```
