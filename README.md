simple_deep
===========

A simple deep image format that is intended to be fast, small and simple to use.

This is still very much a work in progress. 
Contact me, vilhelmo at gmail.com if you're interested in participating in this project.


- How to build?
Run 'scons' or 'scons mode=debug' to build the library and test in release or debug mode.

- How to run?
First set your LD_LIBRARY_PATH to include the library, either in the release or debug directory.
An example on ubuntu if I've built the library from the folder ~/workspace/simple_deep
'export LD_LIBRARY_PATH='~/workspace/simple_deep/release/deep':$LD_LIBRARY_PATH
Then you can run the test application by running [release,debug]/deep_test/deep_test

