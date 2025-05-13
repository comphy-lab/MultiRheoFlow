#!/bin/zsh
# MacOS only. modify using http://basilisk.fr/src/INSTALL 
# ensures that we are always using the latest version of basilisk

rm -rf basilisk
rm -rf .project_config

darcs clone http://basilisk.fr/basilisk
cd basilisk/src

if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Using MacOS"
    ln -s config.osx config
else
    echo "Using Linux"
    ln -s config.gcc config
fi
make -k
make

echo "export BASILISK=$PWD" >> ../../.project_config
echo "export PATH=\$PATH:\$BASILISK" >> ../../.project_config

source ../../.project_config 

