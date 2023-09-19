#!/bin/bash

cat .gitignore | xargs rm -rf
pushd src
    cat ../.gitignore | xargs rm -rf
popd
